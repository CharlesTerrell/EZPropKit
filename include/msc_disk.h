#pragma once

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

bool msc_disk_init(void);
void msc_disk_task(void);
bool msc_disk_check_reload(void);
bool msc_disk_is_reload_pending(void);
bool msc_disk_is_writing(void);
bool msc_disk_find_script(char* out_path, size_t max_len);
void msc_disk_factory_reset(void);

#ifdef __cplusplus
}
#endif
