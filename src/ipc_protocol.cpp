#include "ipc_protocol.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static queue_t cmd_queue;
static queue_t evt_queue;
static bool initialized = false;

void ipc_init(void) {
    if (!initialized) {
        queue_init(&cmd_queue, sizeof(core_command_t), IPC_QUEUE_LENGTH);
        queue_init(&evt_queue, sizeof(core_event_t), IPC_QUEUE_LENGTH);
        initialized = true;
    }
}

bool ipc_send_command(const core_command_t* cmd) {
    if (!initialized) return false;
    return queue_try_add(&cmd_queue, cmd);
}

bool ipc_receive_command(core_command_t* cmd) {
    if (!initialized) return false;
    return queue_try_remove(&cmd_queue, cmd);
}

bool ipc_send_event(const core_event_t* evt) {
    if (!initialized) return false;
    return queue_try_add(&evt_queue, evt);
}

bool ipc_receive_event(core_event_t* evt) {
    if (!initialized) return false;
    return queue_try_remove(&evt_queue, evt);
}

void ipc_log(const char* fmt, ...) {
    if (!initialized) return;
    core_event_t evt;
    memset(&evt, 0, sizeof(evt));
    evt.type = EVT_DEBUG_LOG;
    va_list args;
    va_start(args, fmt);
    vsnprintf(evt.text, sizeof(evt.text), fmt, args);
    va_end(args);
    queue_try_add(&evt_queue, &evt);
}

