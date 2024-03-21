#include "hai_params.h"
#include "utils.h"

void hai_params_print(void){
    hai_params_header* params_header = hai_get_params();
    debug_printf("HAI PARAMS: magic: 0x%X, count: %u\n", params_header->magic, params_header->parameter_count);

    hai_params_entry *entry = (hai_params_entry *)params_header->entries;
    for(int i = 0; i < params_header->parameter_count; i++){
        debug_printf("Entry %d, id 0x%X, size 0x%X", i, entry->id, entry->size);   
        for(size_t j=0; j < entry->size; j++){
            if(j%32 == 0)
                debug_printf("\n");
            debug_printf("%02X ", entry->data[j]);
        }
        debug_printf("\n%s", entry->data);
        debug_printf("\n");
        entry = hai_param_next(entry);
    }
}