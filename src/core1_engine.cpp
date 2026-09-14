#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

#include "core1_engine.h"
#include "board_config.h"
#include "peripheral_mgr.h"
#include <I2S.h>
#include <FatFS.h>
#include <I2S.h>
#include <FatFS.h>
#include <BackgroundAudioMP3.h>
#include <BackgroundAudioWAV.h>
#include <math.h>

// Audio subsystem objects
static I2S i2s_out(OUTPUT, PIN_I2S_BCLK, PIN_I2S_DATA);
static volatile bool is_playing = false;
static bool loop_current = false;
static char current_track[MAX_AUDIO_PATH_LEN] = {0};
static File audio_file;

// Track volume (0..100)
static uint8_t master_volume = 100;

// Type of current audio decoder
typedef enum {
    DEC_NONE,
    DEC_WAV,
    DEC_MP3
} audio_dec_type_t;

static audio_dec_type_t current_dec = DEC_NONE;

// Decoders from BackgroundAudio
static BackgroundAudioWAV audio_wav;
static BackgroundAudioMP3 audio_mp3;

static void setup_i2s(void) {
    i2s_out.setBCLK(PIN_I2S_BCLK); // 17 (LRCLK will be 18)
    i2s_out.setDATA(PIN_I2S_DATA); // 16
    i2s_out.onTransmit((void(*)(void *))nullptr, nullptr);
    i2s_out.onTransmit((void(*)(void))nullptr);
}

static void stop_audio(void) {
    if (is_playing) {
        if (current_dec == DEC_WAV) {
            audio_wav.flush();
            audio_wav.end();
        } else if (current_dec == DEC_MP3) {
            audio_mp3.flush();
            audio_mp3.end();
        }

        if (audio_file) {
            audio_file.close();
        }
        is_playing = false;
        current_dec = DEC_NONE;

        // Force I2S lines LOW to ensure MAX98357A enters hardware shutdown immediately
        pinMode(PIN_I2S_BCLK, OUTPUT);
        digitalWrite(PIN_I2S_BCLK, LOW);
        pinMode(PIN_I2S_DATA, OUTPUT);
        digitalWrite(PIN_I2S_DATA, LOW);
        pinMode(PIN_I2S_LRCLK, OUTPUT);
        digitalWrite(PIN_I2S_LRCLK, LOW);
    }
}

static void play_tone(uint16_t freq_hz, uint16_t duration_ms) {
    stop_audio();
    power_rail_enable();
    setup_i2s();

    i2s_out.setBitsPerSample(16);
    i2s_out.setBuffers(4, 256);
    if (!i2s_out.begin(44100)) {
        return;
    }
    is_playing = true;

    core_event_t evt_start = { EVT_AUDIO_STARTED, 4 /* TONE */ };
    ipc_send_event(&evt_start);

    uint32_t total_samples = (44100UL * (uint32_t)duration_ms) / 1000UL;
    float phase_step = 2.0f * (float)M_PI * (float)freq_hz / 44100.0f;
    float phase = 0.0f;
    int16_t amplitude = (int16_t)(16000.0f * ((float)master_volume / 100.0f));

    for (uint32_t i = 0; i < total_samples; i++) {
        if ((i & 0xFF) == 0) {
            core_command_t check_cmd;
            if (ipc_receive_command(&check_cmd)) {
                if (check_cmd.type == CMD_AUDIO_STOP || check_cmd.type == CMD_RESET_PERIPHERALS) {
                    break;
                }
            }
        }
        int16_t sample = (int16_t)(sinf(phase) * (float)amplitude);
        phase += phase_step;
        if (phase >= 2.0f * (float)M_PI) {
            phase -= 2.0f * (float)M_PI;
        }
        i2s_out.write16(sample, sample);
    }

    i2s_out.flush();
    delay(20); // Allow DMA to finish playing the last buffer before end()
    i2s_out.end();

    pinMode(PIN_I2S_BCLK, OUTPUT);
    digitalWrite(PIN_I2S_BCLK, LOW);
    pinMode(PIN_I2S_DATA, OUTPUT);
    digitalWrite(PIN_I2S_DATA, LOW);
    pinMode(PIN_I2S_LRCLK, OUTPUT);
    digitalWrite(PIN_I2S_LRCLK, LOW);

    is_playing = false;
    core_event_t evt_done = { EVT_AUDIO_FINISHED, 0 };
    ipc_send_event(&evt_done);
}

static bool start_audio(const char* path, bool loop) {
    ipc_log("start_audio: '%s', loop=%d", path, loop ? 1 : 0);
    stop_audio();

    // Ensure power rail is enabled for MAX98357A amplifier
    power_rail_enable();
    setup_i2s();

    // Normalize path to always have a leading slash for FatFS
    char full_path[MAX_AUDIO_PATH_LEN];
    if (path[0] == '/') {
        strncpy(full_path, path, sizeof(full_path) - 1);
        full_path[sizeof(full_path) - 1] = '\0';
    } else {
        snprintf(full_path, sizeof(full_path), "/%s", path);
    }

    // Determine codec from file extension
    size_t len = strlen(full_path);
    if (len < 4) {
        ipc_log("ERR: path too short '%s'", full_path);
        core_event_t evt = { EVT_AUDIO_ERROR, -1, "" };
        ipc_send_event(&evt);
        return false;
    }
    const char* ext = full_path + (len - 4);

    ipc_log("FatFS.open('%s')...", full_path);
    audio_file = FatFS.open(full_path, "r");
    if (!audio_file) {
        ipc_log("FatFS.open FAILED for '%s'!", full_path);
        core_event_t evt = { EVT_AUDIO_ERROR, -1, "" };
        ipc_send_event(&evt);
        return false;
    }
    ipc_log("FatFS.open OK, size=%lu", (unsigned long)audio_file.size());

    strncpy(current_track, full_path, sizeof(current_track) - 1);
    loop_current = loop;

    float gain = (float)master_volume / 100.0f;
    bool begin_ok = false;

    if (strcasecmp(ext, ".wav") == 0) {
        current_dec = DEC_WAV;
        audio_wav.setDevice(&i2s_out);
        audio_wav.setGain(gain);
        ipc_log("Starting audio_wav.begin()...");
        begin_ok = audio_wav.begin();
    } else if (strcasecmp(ext, ".mp3") == 0) {
        current_dec = DEC_MP3;
        audio_mp3.setDevice(&i2s_out);
        audio_mp3.setGain(gain);
        ipc_log("Starting audio_mp3.begin()...");
        begin_ok = audio_mp3.begin();
    } else {
        ipc_log("ERR: unknown ext '%s'", ext);
        audio_file.close();
        current_dec = DEC_NONE;
        core_event_t evt = { EVT_AUDIO_ERROR, -2, "" };
        ipc_send_event(&evt);
        return false;
    }

    if (!begin_ok) {
        ipc_log("ERR: decoder begin() failed!");
        audio_file.close();
        current_dec = DEC_NONE;
        core_event_t evt = { EVT_AUDIO_ERROR, -3, "" };
        ipc_send_event(&evt);
        return false;
    }
    ipc_log("Decoder begin() OK!");

    // Pre-fill decoder buffer with initial data so pump() has samples immediately
    size_t prefilled = 0;
    while (audio_file.available()) {
        size_t avail = 0;
        if (current_dec == DEC_WAV) avail = audio_wav.availableForWrite();
        else if (current_dec == DEC_MP3) avail = audio_mp3.availableForWrite();

        if (avail < 512) break;

        uint8_t buf[512];
        size_t to_read = (avail > sizeof(buf)) ? sizeof(buf) : avail;
        size_t bytes_read = audio_file.read(buf, to_read);
        if (bytes_read > 0) {
            if (current_dec == DEC_WAV) audio_wav.write(buf, bytes_read);
            else if (current_dec == DEC_MP3) audio_mp3.write(buf, bytes_read);
            prefilled += bytes_read;
        } else {
            break;
        }
    }
    ipc_log("Prefilled %u bytes into decoder", (unsigned int)prefilled);

    is_playing = true;
    core_event_t evt = { EVT_AUDIO_STARTED, (int32_t)current_dec, "" };
    ipc_send_event(&evt);
    return true;
}

void core1_start(void) {
    // Initial state on Core 1: Hold I2S lines LOW so amplifier stays in shutdown
    is_playing = false;
    pinMode(PIN_I2S_BCLK, OUTPUT);
    digitalWrite(PIN_I2S_BCLK, LOW);
    pinMode(PIN_I2S_DATA, OUTPUT);
    digitalWrite(PIN_I2S_DATA, LOW);
    pinMode(PIN_I2S_LRCLK, OUTPUT);
    digitalWrite(PIN_I2S_LRCLK, LOW);
}

void core1_loop_process(void) {
    // 1. Process incoming commands from Core 0
    core_command_t cmd;
    while (ipc_receive_command(&cmd)) {
        switch (cmd.type) {
            case CMD_AUDIO_PLAY:
                start_audio(cmd.params.audio_play.path, cmd.params.audio_play.loop);
                break;
            case CMD_AUDIO_STOP:
                stop_audio();
                break;
            case CMD_AUDIO_PAUSE:
                // BackgroundAudio pauses when not fed data
                break;
            case CMD_AUDIO_RESUME:
                break;
            case CMD_AUDIO_SET_VOLUME:
                master_volume = cmd.params.audio_volume.volume;
                {
                    float gain = (float)master_volume / 100.0f;
                    if (current_dec == DEC_WAV) audio_wav.setGain(gain);
                    else if (current_dec == DEC_MP3) audio_mp3.setGain(gain);
                }
                break;
            case CMD_AUDIO_TONE:
                play_tone(cmd.params.audio_tone.freq_hz, cmd.params.audio_tone.duration_ms);
                break;
            case CMD_SERVO_SET_US:
                servo_write_us(cmd.params.servo_set.pulse_us);
                break;
            case CMD_SERVO_DETACH:
                servo_detach();
                break;
            case CMD_RESET_PERIPHERALS:
                stop_audio();
                break;
            default:
                break;
        }
    }

    // 2. Feed data into the active decoder buffer
    if (is_playing && audio_file) {
        while (audio_file.available()) {
            size_t avail = 0;
            if (current_dec == DEC_WAV) avail = audio_wav.availableForWrite();
            else if (current_dec == DEC_MP3) avail = audio_mp3.availableForWrite();

            if (avail < 512) {
                break; // Buffer is sufficiently topped up
            }

            uint8_t buf[512];
            size_t to_read = (avail > sizeof(buf)) ? sizeof(buf) : avail;
            size_t bytes_read = audio_file.read(buf, to_read);
            if (bytes_read > 0) {
                if (current_dec == DEC_WAV) audio_wav.write(buf, bytes_read);
                else if (current_dec == DEC_MP3) audio_mp3.write(buf, bytes_read);
            } else {
                break;
            }
        }

        if (!audio_file.available()) {
            // Check if buffer has finished draining
            bool finished = false;
            if (current_dec == DEC_WAV) finished = audio_wav.done();
            else if (current_dec == DEC_MP3) finished = audio_mp3.done();

            if (finished) {
                if (loop_current) {
                    ipc_log("Looping track '%s'...", current_track);
                    audio_file.seek(0);
                    if (current_dec == DEC_WAV) audio_wav.flush();
                    else if (current_dec == DEC_MP3) audio_mp3.flush();
                } else {
                    ipc_log("Playback finished for '%s'", current_track);
                    stop_audio();
                    core_event_t evt = { EVT_AUDIO_FINISHED, 0, "" };
                    ipc_send_event(&evt);
                }
            }
        }

        static uint32_t last_stat_ms = 0;
        if (millis() - last_stat_ms >= 1000) {
            last_stat_ms = millis();
            if (current_dec == DEC_WAV) {
                ipc_log("WAV stats: f=%lu u=%lu err=%lu dmp=%lu pos=%lu/%lu",
                        (unsigned long)audio_wav.frames(),
                        (unsigned long)audio_wav.underflows(),
                        (unsigned long)audio_wav.errors(),
                        (unsigned long)audio_wav.dumps(),
                        (unsigned long)audio_file.position(),
                        (unsigned long)audio_file.size());
            } else if (current_dec == DEC_MP3) {
                ipc_log("MP3 stats: f=%lu u=%lu err=%lu dmp=%lu pos=%lu/%lu",
                        (unsigned long)audio_mp3.frames(),
                        (unsigned long)audio_mp3.underflows(),
                        (unsigned long)audio_mp3.errors(),
                        (unsigned long)audio_mp3.dumps(),
                        (unsigned long)audio_file.position(),
                        (unsigned long)audio_file.size());
            }
        }
    }
}

// Earle's Philhower core multicore hooks
void setup1() {
    core1_start();
}

void loop1() {
    core1_loop_process();
}

bool core1_audio_is_playing(void) {
    return is_playing;
}
