#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "pico/util/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_AUDIO_PATH_LEN 64
#define IPC_QUEUE_LENGTH   64

typedef enum {
    CMD_NONE = 0,
    CMD_AUDIO_PLAY,
    CMD_AUDIO_STOP,
    CMD_AUDIO_PAUSE,
    CMD_AUDIO_RESUME,
    CMD_AUDIO_SET_VOLUME,
    CMD_AUDIO_TONE,
    CMD_SERVO_SET_US,
    CMD_SERVO_DETACH,
    CMD_RESET_PERIPHERALS
} core_cmd_type_t;

typedef struct {
    core_cmd_type_t type;
    union {
        struct {
            char path[MAX_AUDIO_PATH_LEN];
            bool loop;
        } audio_play;
        struct {
            uint8_t volume; // 0..100
        } audio_volume;
        struct {
            uint16_t freq_hz;
            uint16_t duration_ms;
        } audio_tone;
        struct {
            uint16_t pulse_us;
        } servo_set;
    } params;
} core_command_t;

typedef enum {
    EVT_NONE = 0,
    EVT_AUDIO_STARTED,
    EVT_AUDIO_FINISHED,
    EVT_AUDIO_ERROR,
    EVT_DEBUG_LOG
} core_evt_type_t;

typedef struct {
    core_evt_type_t type;
    int32_t code;
    char text[80];
} core_event_t;

// Queue initialization and access
void ipc_init(void);
bool ipc_send_command(const core_command_t* cmd);
bool ipc_receive_command(core_command_t* cmd);
bool ipc_send_event(const core_event_t* evt);
bool ipc_receive_event(core_event_t* evt);
void ipc_log(const char* fmt, ...);

#ifdef __cplusplus
}
#endif
