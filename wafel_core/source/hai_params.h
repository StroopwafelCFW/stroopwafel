#include "types.h"
#include "addrs_55x.h"

#pragma once


typedef struct {
    u32 magic;
    u16 total_size;
    u16 parameter_count;
    u8 entries[];
} PACKED hai_params_header;

typedef struct {
    u16 id;
    u16 size;
    u8 data[];
} PACKED hai_params_entry;

static inline hai_params_header* hai_get_params(void){
    return *((hai_params_header**)HAI_PARAM_POINTER);
}

static inline hai_params_entry* hai_param_next(hai_params_entry *entry){
    return (hai_params_entry*)((void*)entry + entry->size + offsetof(hai_params_entry, data));
}

void hai_params_print(void);