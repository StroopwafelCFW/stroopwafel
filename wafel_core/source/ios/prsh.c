#include "ios/prsh.h"
#include "utils.h"

#include <string.h>

static prst_entry* prst = NULL;
static prsh_header* header = NULL;
static bool initialized = false;

void prsh_dump_entry(char* name) {
    void* data = NULL;
    size_t size;
    if (!prsh_get_entry(name, &data, &size)) {
        if (size > 0x200) {
            size = 0x200;
        }
        for (size_t i = 0; i < size; i++) {
            if (i && i % 16 == 0) {
                debug_printf("\n        ");
            }
            else if (!i) {
                debug_printf("        ");
            }
            debug_printf("%02x ", *(u8*)((u32)data + i));
        }
        debug_printf("\n");
    }
}

void prsh_reset(void)
{
    prst = NULL;
    header = NULL;
    initialized = false;
}

void prsh_init(void)
{
    // corrupt
    if (header && header->total_entries > 0x100) {
        debug_printf("prsh: Detected corruption, %u total entries.\n", header->total_entries);
        initialized = false;
    }

    if(initialized) return;

    void* buffer = (void*)0x10000400;
    size_t size = 0x7C00;
    while(size) {
        if(!memcmp(buffer, "PRSH", sizeof(u32))) break;
        buffer += sizeof(u32);
        size -= sizeof(u32);

        //printf("%08x: %08x\n", buffer, *(u32*)buffer);
    }

    if (!size) {
        debug_printf("prsh: No header found!!\n");
        header = NULL;
        prst = NULL;
    }
    else {
        header = buffer - sizeof(u32);
        prst = (prst_entry*)&header->entry[header->total_entries];

        //header->entries = 0x4;
    }

    initialized = true;

    for (int i = 0; i < header->entries; i++)
    {
        prsh_entry* entry = &header->entry[i];
        debug_printf("%02x: %s\n", i, entry->name);
    }

    // TODO: verify checksums
}

int prsh_get_entry(const char* name, void** data, size_t* size)
{
    prsh_init();
    if(!name) return -1;
    if(!header) return -2;
    if (header->total_entries > 0x100) return -1; // corrupt 

    for(int i = 0; i < header->entries; i++) {
        prsh_entry* entry = &header->entry[i];

        if(!strncmp(name, entry->name, sizeof(entry->name))) {
            if(data) *data = entry->data;
            if(size) *size = entry->size;
            return 0;
        }
    }

    return -2;
}

#if 0
void prsh_recompute_checksum()
{
    prsh_init();
    // Re-calculate PRSH checksum
    u32 checksum = 0;
    u32 word_counter = 0;
    
    void* checksum_start = (void*)(&header->magic);
    while (word_counter < ((header->entries * sizeof(prsh_entry))>>2))
    {
        checksum ^= *(u32 *)(checksum_start + word_counter * 0x04);
        word_counter++;
    }
    
    printf("%08x %08x\n", header->checksum, checksum);
    
    header->checksum = checksum;

    checksum = 0;
    word_counter = 0;
    void* prst_checksum_start = (void*)(&prst->size);
    while (word_counter < (sizeof(prst_entry)/sizeof(u32)-1))
    {
        u32 val = *(u32 *)(prst_checksum_start + word_counter * 0x04);
        //printf("%08x\n", val);
        checksum ^= val;
        word_counter++;
    }
    
    printf("%08x %08x\n", prst->checksum, checksum);
    
    prst->checksum = checksum;
}
#endif