#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _UINT_DEFINED
typedef unsigned int uint;
#define _UINT_DEFINED
#endif

typedef struct {
    uint8_t* buffer;
    uint element_size;
    uint element_count;
    uint head;
    uint tail;
    uint count;
} queue_t;

void queue_init(queue_t *q, uint element_size, uint element_count);
bool queue_try_add(queue_t *q, const void *data);
bool queue_try_remove(queue_t *q, void *data);
void queue_free(queue_t *q);
uint queue_get_level(queue_t *q);

#ifdef __cplusplus
}
#endif
