#include "msc_disk.h"
#include "board_config.h"
#include "core1_engine.h"
#include "ipc_protocol.h"
#include <Adafruit_TinyUSB.h>
#include <FatFS.h>
#include "diskio.h"
#include <hardware/flash.h>
#include <strings.h>

// USB Mass Storage object
static Adafruit_USBD_MSC usb_msc;

static volatile uint32_t last_write_ms = 0;
static volatile bool write_active = false;
static bool reload_requested = false;

// Deferred debug buffer for LBA 27 (to avoid any blocking Serial calls in USB callbacks)
static uint8_t debug_lba27_buf[128];
static volatile bool debug_has_lba27 = false;

// Callbacks for MSC
static int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize) {
    uint32_t count = bufsize / 512;
    return (fatfs::disk_read(0, (uint8_t*)buffer, lba, count) == fatfs::RES_OK) ? bufsize : -1;
}

static int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
    if (!write_active) {
        core_command_t cmd = { CMD_AUDIO_STOP, {} };
        ipc_send_command(&cmd);
        write_active = true;
    }
    last_write_ms = millis();
    write_active = true;
    if (lba == 27 && bufsize >= 128) {
        memcpy(debug_lba27_buf, buffer, 128);
        debug_has_lba27 = true;
    }
    uint32_t count = bufsize / 512;
    int32_t ret = (fatfs::disk_write(0, (const uint8_t*)buffer, lba, count) == fatfs::RES_OK) ? bufsize : -1;
    return ret;
}

static void msc_flush_cb(void) {
    last_write_ms = millis();
    write_active = true;
}

extern uint8_t _FS_start;
extern uint8_t _FS_end;

static bool format_fatfs_clean(void) {
    Serial.println("[MSC] Performing clean FAT16 format with 2KB clusters...");
    fatfs::disk_initialize(0);
    fatfs::BYTE work[4096];
    fatfs::MKFS_PARM opt = {
        .fmt = FM_FAT | FM_SFD,
        .n_fat = 1,
        .align = 0,
        .n_root = 512,
        .au_size = 2048
    };
    fatfs::FRESULT res = fatfs::f_mkfs("", &opt, work, sizeof(work));
    Serial.printf("[MSC] f_mkfs result: %d\r\n", res);
    return (res == fatfs::FR_OK);
}

void msc_disk_factory_reset(void) {
    Serial.println("[MSC] Erasing entire filesystem flash partition...");
    FatFS.end();

    uint32_t fs_start_offset = (uint32_t)&_FS_start - (uint32_t)XIP_BASE;
    uint32_t fs_size = (uint32_t)&_FS_end - (uint32_t)&_FS_start;
    fs_size = (fs_size / 4096) * 4096;

    rp2040.idleOtherCore();
    flash_range_erase(fs_start_offset, fs_size);
    rp2040.resumeOtherCore();

    format_fatfs_clean();
    FatFS.begin();
    fatfs::f_setlabel(USB_DRIVE_LABEL);

    File f = FatFS.open("/README.txt", "w");
    if (f) {
        f.println("Welcome to EZPROPKIT!");
        f.println("Drop code.lua or main.lua here to run your prop script.");
        f.println("Supported audio files: .wav, .mp3");
        f.close();
        fatfs::disk_ioctl(0, CTRL_SYNC, nullptr);
    }
    Serial.println("[MSC] Factory reset complete.");
}

bool msc_disk_init(void) {
    Serial.println("[MSC] Initializing SPI Flash Filesystem (FatFS)...");

    // Check if user is holding button on boot for factory reset
    pinMode(PIN_BOOT_BUTTON, INPUT_PULLUP);
    pinMode(PIN_EXTERNAL_BUTTON, INPUT_PULLUP);
    if (digitalRead(PIN_BOOT_BUTTON) == LOW || digitalRead(PIN_EXTERNAL_BUTTON) == LOW) {
        delay(50);
        if (digitalRead(PIN_BOOT_BUTTON) == LOW || digitalRead(PIN_EXTERNAL_BUTTON) == LOW) {
            Serial.println("[MSC] Button held at boot: Factory Reset triggered!");
            msc_disk_factory_reset();
        }
    }

    // Initialize FAT filesystem on external SPI flash
    bool fat_ok = FatFS.begin();
    if (!fat_ok) {
        Serial.println("[MSC] Filesystem unformatted or corrupt. Formatting now...");
        format_fatfs_clean();
        fat_ok = FatFS.begin();
    }

    if (fat_ok) {
        Serial.println("[MSC] FatFS mounted successfully.");
        fatfs::f_setlabel(USB_DRIVE_LABEL);
    } else {
        Serial.println("[MSC] Warning: FatFS mount failed. Exposing raw block device over USB.");
    }

    uint32_t sector_count = 0;
    fatfs::disk_ioctl(0, GET_SECTOR_COUNT, &sector_count);
    if (sector_count == 0) {
        uint32_t fs_bytes = (uint32_t)(&_FS_end - &_FS_start);
        sector_count = (fs_bytes > 0) ? (fs_bytes / 512) : 13107;
    }
    Serial.printf("[MSC] Disk capacity: %lu sectors (%lu KB)\r\n",
                  (unsigned long)sector_count,
                  (unsigned long)(sector_count * 512 / 1024));

    // Configure USB Mass Storage endpoint
    usb_msc.setID("Adafruit", USB_DRIVE_LABEL, "1.0");
    usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
    usb_msc.setCapacity(sector_count, 512);
    usb_msc.setUnitReady(true);
    usb_msc.begin();

    // Re-enumerate USB so host discovers Mass Storage Class (MSC) endpoint
    TinyUSBDevice.detach();
    delay(100);
    TinyUSBDevice.attach();

    Serial.println("[MSC] USB Mass Storage initialized.");

    // Provide starter README if drive has no files
    if (fat_ok && !FatFS.exists("/code.lua") && !FatFS.exists("/main.lua") && !FatFS.exists("/README.txt")) {
        File f = FatFS.open("/README.txt", "w");
        if (f) {
            f.println("Welcome to EZPROPKIT!");
            f.println("Drop code.lua or main.lua here to run your prop script.");
            f.println("Supported audio files: .wav, .mp3");
            f.close();
            fatfs::disk_ioctl(0, CTRL_SYNC, nullptr);
        }
    }

    return fat_ok;
}

void msc_disk_task(void) {
    // Check if write activity occurred and has settled
    if (write_active) {
        if (millis() - last_write_ms >= MSC_WRITE_DEBOUNCE_MS) {
            write_active = false;
            // Persist cached FTL metadata once writes have settled
            fatfs::disk_ioctl(0, CTRL_SYNC, nullptr);
            reload_requested = true;
        }
    }
}

bool msc_disk_check_reload(void) {
    if (reload_requested) {
        reload_requested = false;
        return true;
    }
    return false;
}

bool msc_disk_is_reload_pending(void) {
    return reload_requested;
}

bool msc_disk_find_script(char* out_path, size_t max_len) {
    fatfs::DIR d;
    fatfs::FILINFO info;
    char matched_file[64] = {0};

    Serial.println("[SUPERVISOR] Scanning FatFs directory for prop script...");
    if (fatfs::f_opendir(&d, "") == fatfs::FR_OK) {
        // Direct dump of first root directory sector for diagnostic verification
        uint8_t root_sec[512];
        if (fatfs::disk_read(0, root_sec, d.obj.fs->dirbase, 1) == fatfs::RES_OK) {
            Serial.printf("[SUPERVISOR] Root dir LBA %lu (cluster_size=%u bytes):\r\n",
                          (unsigned long)d.obj.fs->dirbase, (unsigned int)(d.obj.fs->csize * 512));
            for (int e = 0; e < 6; e++) {
                Serial.printf("  [%02d] ", e);
                for (int b = 0; b < 32; b++) {
                    Serial.printf("%02X ", root_sec[e * 32 + b]);
                }
                Serial.print(" | ");
                for (int b = 0; b < 32; b++) {
                    char c = root_sec[e * 32 + b];
                    Serial.print((c >= 32 && c < 127) ? c : '.');
                }
                Serial.println();
            }
        }

        while (fatfs::f_readdir(&d, &info) == fatfs::FR_OK && info.fname[0]) {
            Serial.printf("  [ENTRY] fname='%s' altname='%s' size=%lu attr=0x%02X\r\n",
                          info.fname, info.altname, (unsigned long)info.fsize, info.fattrib);

            if (info.fattrib & (AM_DIR | 0x08)) continue;

            const char* fn = info.fname;
            const char* alt = info.altname;

            // Match code.lua (case-insensitive on both LFN and SFN)
            if (strcasecmp(fn, "code.lua") == 0 || (alt[0] && strcasecmp(alt, "CODE.LUA") == 0)) {
                strncpy(matched_file, fn, sizeof(matched_file) - 1);
                break;
            }
            // Match main.lua
            if (strcasecmp(fn, "main.lua") == 0 || (alt[0] && strcasecmp(alt, "MAIN.LUA") == 0)) {
                if (matched_file[0] == '\0') {
                    strncpy(matched_file, fn, sizeof(matched_file) - 1);
                }
            }
        }
        fatfs::f_closedir(&d);
    } else {
        Serial.println("  f_opendir failed!");
    }

    if (matched_file[0] != '\0') {
        snprintf(out_path, max_len, "/%s", matched_file);
        return true;
    }

    // Direct stat fallback checks
    if (FatFS.exists("/code.lua")) {
        strncpy(out_path, "/code.lua", max_len - 1);
        out_path[max_len - 1] = '\0';
        return true;
    }
    if (FatFS.exists("/main.lua")) {
        strncpy(out_path, "/main.lua", max_len - 1);
        out_path[max_len - 1] = '\0';
        return true;
    }

    return false;
}

bool msc_disk_is_writing(void) {
    return write_active;
}
