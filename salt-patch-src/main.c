#define _GNU_SOURCE

#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "utils.h"
#include "sha1.h"

size_t get_fsize(FILE* f)
{
    fseeko(f, 0, SEEK_END);
    size_t fsize = ftello(f);
    rewind(f);
    return fsize;
}

u32 virt_to_phys(u32 addr)
{
    switch(addr)
    {
        case 0x04000000: return 0x08280000;
        case 0x04020000: return 0x082A0000;
        case 0x04024000: return 0x082A4000;
        case 0x04025000: return 0x082A5000;
        case 0x05000000: return 0x081C0000;
        case 0x05060000: return 0x08220000;
        case 0x05070000: return 0x08230000;
        case 0x05074000: return 0x08234000;
        case 0x05100000: return 0x13D80000;
        case 0xe0000000: return 0x12900000;
        case 0xe0100000: return 0x12a00000;
        case 0xe0121000: return 0x12a21000;
        case 0xe0122000: return 0x12a22000;
        case 0xe0123000: return 0x12a23000;
        case 0xe1000000: return 0x12bc0000;
        case 0xe10c0000: return 0x12c80000;
        case 0xe10e2000: return 0x12ca2000;
        case 0xe10e4000: return 0x12ca4000;
        case 0xe2000000: return 0x12ec0000;
        case 0xe2280000: return 0x13140000;
        case 0xe22c9000: return 0x13189000;
        case 0xe22ca000: return 0x1318a000;
        case 0xe22cb000: return 0x1318b000;
        case 0xe3000000: return 0x13640000;
        case 0xe3180000: return 0x137c0000;
        case 0xe31ad000: return 0x137ed000;
        case 0xe31ae000: return 0x137ee000;
        case 0xe31af000: return 0x137ef000;
        case 0xe4000000: return 0x13a40000;
        case 0xe4040000: return 0x13a80000;
        case 0xe4046000: return 0x13a86000;
        case 0xe4047000: return 0x13a87000;
        case 0xe5000000: return 0x13c00000;
        case 0xe5040000: return 0x13c40000;
        case 0xe5044000: return 0x13c44000;
        case 0xe5045000: return 0x13c45000;
        case 0xe6000000: return 0x13cc0000;
        case 0xe6040000: return 0x13d00000;
        case 0xe6042000: return 0x13d02000;
        case 0xe6047000: return 0x13d07000;
        case 0xe7000000: return 0x082c0000;
        case 0xeff00000: return 0xfff00000;
        default: return addr;
    }
}

typedef struct {
    char magic[8]; // 'SALTPTCH'
    char version[4];
    char sha1[20];  // checksum of entire file with this zeroed
} patchfile_header;

typedef struct {
    char type[4]; // 0 = data (followed by data[len]), 1 = memset
    char addr[4];
    char len[4];
} generic_chunk;

#define PATCHFILE_VERSION 1
void write_patch_header(FILE* f_patch)
{
    patchfile_header hdr;
    memcpy(&hdr.magic, "SALTPTCH", 8);
    putbe32(&hdr.version, PATCHFILE_VERSION);
    memset(hdr.sha1, 0, 20);
    
    fwrite(&hdr, 1, sizeof(patchfile_header), f_patch);
}

void write_patch_footer_and_hash(FILE* f_patch)
{
    // write footer
    u8 footer[4];
    putbe32(footer, 0xFF);
    fwrite(footer, 1, 4, f_patch);
    
    // update hash
    patchfile_header hdr;
    sha1_ctx sha;
    sha1_begin(&sha);
    void* filebuf = malloc(0x2000);
    size_t size = get_fsize(f_patch);
    for(size_t i = 0; i < size; i += 0x2000)
    {
        size_t cur_size = (size - i < 0x2000) ? size - i : 0x2000;
        fread(filebuf, 1, cur_size, f_patch);
        sha1_hash(filebuf, cur_size, &sha);
    }
    free(filebuf);
    
    u32 hash[5];
    sha1_end((void*)hash, &sha);
    
    // seek to sha hash (zeroed) in file
    fseek(f_patch, 12, SEEK_SET);
    fwrite(hash, 1, 20, f_patch);
}

void patch_add_diff(FILE* f_patch, void* data, u32 len_words, u32 base)
{
    //printf("Adding 0x%08X len 0x%X\n", base, len_words*4);
    generic_chunk chnk;
    putbe32(chnk.type, 0);
    putbe32(chnk.addr, base);
    putbe32(chnk.len, len_words*4);
    fwrite(&chnk, 1, sizeof(generic_chunk), f_patch);
    fwrite(data, 1, len_words*4, f_patch);
}

void patch_add_memset(FILE* f_patch, u32 len_words, u32 base)
{
    //printf("Adding memset chunk of len %X, base %X\n", len_words*4, base);
    generic_chunk chnk;
    putbe32(chnk.type, 1);
    putbe32(chnk.addr, base);
    putbe32(chnk.len, len_words*4);
    fwrite(&chnk, 1, sizeof(generic_chunk), f_patch);
}

int patch_add_to_file(FILE *f_patched, FILE *f_patch, u32 base)
{
    size_t fsize = get_fsize(f_patched);
    size_t fsize_round = (fsize + 3) & ~0x03;
    
    u32* data = calloc(fsize_round, 1);
    if(!data)
    {
        printf("Failed to allocate memory for encode!\n");
        return -1;
    }
    if(fread(data, 1, fsize, f_patched) != fsize)
    {
        printf("Failed to fully read patched data file!\n");
        return -2;
    }
    
    size_t i = 0;
    while(i < fsize_round/4)
    {
        size_t j = i;
        if(data[i] == 0) // add memset
        {
            // after this loop, i - j = num words to memset
            while(data[i] == 0)
            {
                i++;
                if(i >= fsize_round/4) break;
            }
            patch_add_memset(f_patch, (i-j), base+(u64)&data[j] - (u64)data);
        }
        else // add data
        {
            // after this loop, i - j = num words to copy
            while(data[i] != 0)
            {
                i++;
                if(i >= fsize_round/4) break;
            }
            patch_add_diff(f_patch, &data[j], (i-j), base+(u64)&data[j] - (u64)data);
        }
    }
    
    
    return 0;
}

int patch_diff_to_file(FILE *f_unpatched, FILE *f_patched, FILE *f_patch, u32 base)
{
    size_t unpatched_size = get_fsize(f_unpatched);
    size_t patched_size = get_fsize(f_patched);
    size_t unpatched_size_round = (unpatched_size + 3) & ~0x03;
    size_t patched_size_round = (patched_size + 3) & ~0x03;
    
    u32* unpatched_data = calloc(unpatched_size_round, 1);
    u32* patched_data = calloc(patched_size_round, 1);
    if(!unpatched_data || !patched_data)
    {
        printf("Failed to allocate memory for diff!\n");
        free(patched_data);
        free(unpatched_data);
        return -1;
    }
    
    if(fread(unpatched_data, 1, unpatched_size, f_unpatched) != unpatched_size)
    {
        printf("Failed to fully read patched data file!\n");
        return -2;
    }
    if(fread(patched_data, 1, patched_size, f_patched) != patched_size)
    {
        printf("Failed to fully read patched data file!\n");
        return -3;
    }
    
    size_t i = 0;
    while(i < unpatched_size_round/4)
    {
        size_t j = i;
        if(unpatched_data[i] != patched_data[i])
        {
            // after this loop, i - j = num words to patch
            while(unpatched_data[i] != patched_data[i])
            {
                i++;
                if(i >= unpatched_size_round/4)
                    break;
            }
            patch_add_diff(f_patch, &patched_data[j], (i-j), base+(u64)&patched_data[j] - (u64)patched_data);
        }
        else
            i++;
    }
    
    // add any additional data in f_patched
    if(unpatched_size_round != patched_size_round)
        patch_add_diff(f_patch, &patched_data[unpatched_size_round/4], (patched_size_round-unpatched_size_round)/4, base+unpatched_size_round);
    
    free(unpatched_data);
    free(patched_data);
    return 0;
}

void patch_read(char* fn_patches)
{
    printf("PATCHREAD: Reading out %s\n", fn_patches);
    
    // open and get the size of our patches
    FILE* f_patches= fopen(fn_patches, "rb");
    if(!f_patches)
    {
        printf("Failed to open file!\n");
        return;
    }
    fseek(f_patches, 0, SEEK_END);
    size_t patches_size = ftell(f_patches);
    rewind(f_patches);
    
    // allocate memory for, then read, patch data
    u32* patch_data = malloc(patches_size);
    if(!patch_data)
    {
        printf("Failed to alloc %d bytes for patchdata!\n", (u32)patches_size);
        return;
    }
    size_t readsize = fread(patch_data, 1, patches_size, f_patches);
    if(readsize != patches_size)
    {
        printf("Failed to read out %s\n", fn_patches);
        return;
    }
    fclose(f_patches);
    // endian-flip the fucker
    for(u32 i = 2; i < (patches_size / 4); i++)
        patch_data[i] = getbe32(&patch_data[i]);
    
    // check magic and patch version
    if(memcmp(patch_data, "SALTPTCH", 8))
    {
        printf("Invalid magic! %08X%08X\n", patch_data[0], patch_data[1]);
        return;
    }
    
    if(patch_data[2] != 1)
    {
        printf("Unsupported patch version %d\n", getbe32(&patch_data[2]));
        return;
    }
    
    u32 cur_word = 8; // skip past header
    while(cur_word < (patches_size / 4))
    {
        switch(patch_data[cur_word])
        {
            case 0:
                printf("Data chunk: Offs 0x%08X len 0x%08X\n", patch_data[cur_word+1], patch_data[cur_word+2]);
                u32 num_data_words = patch_data[cur_word+2] / 4;
                cur_word += (3 + num_data_words);
                break;
            case 1:
                printf("Memset chunk: Offs 0x%08X len 0x%08X\n", patch_data[cur_word+1], patch_data[cur_word+2]);
                cur_word += 3;
                break;
            case 0xFF:
                printf("EOF chunk.\n");
                cur_word += 1;
                break;
            default:
                printf("Unrecognized chunk type %08X\n", patch_data[cur_word]);
                cur_word++;
                break;
        }
            
    }
    
    free(patch_data);
}

int main(int argc, char* argv[])
{    
    int i;
    
    printf("SALT ios patch generator v0.5\n"
            "(C) SALT 2016\n\n");
    if(argc >= 2)
    {
        if(!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))
        {
            printf("\nUsage: salt-patch [read-patchfile]"
            "\n-Supply a filename (e.g.'salt-patch ios.patches')to read"
            "\n   the contents of that file's patches.");
            exit(0);
        }
    }
    if(argc == 2)
    {
        patch_read(argv[1]);
        exit(0);
    }
    
    char* fname_patch = "ios.patch";
    FILE* f_patch = fopen(fname_patch, "wb+");
    if(!f_patch)
    {
        printf("Failed to create %s\n", fname_patch);
        return -1;
    }
    write_patch_header(f_patch);
    
    struct dirent *ent;
    DIR* d_sections = opendir("sections");
    if(d_sections == NULL)
    {
        printf("Failed to open /sections!\n");
        return -1;
    }
    
    DIR* d_patched = opendir("patched_sections");
    if(d_patched == NULL)
    {
        printf("Failed to open /patched_sections!\n");
        return -1;
    }
    
    while((ent = readdir(d_patched)))
    {
        char* fn_cur = ent->d_name;
        // skip `.`, `..`, `.DS_Store`, etc
        if(fn_cur[0] == '.')
            continue;
        
        // sanity-check fname format
        if(!strstr(fn_cur, ".bin") || !strstr(fn_cur, "0x"))
        {
            printf("File %s is not a valid bin! (format 0xFFFFFFFF.s)\n", fn_cur);
            continue;
        }
        
        // get u32 base_addr from fname
        u32 base_addr;
        sscanf(fn_cur, "%x.bin", &base_addr);
        base_addr = virt_to_phys(base_addr);
        
        // prepare our bin filenames and open them
        char *fn_unpatched, *fn_patched;
        asprintf(&fn_unpatched, "sections/%s", fn_cur);
        asprintf(&fn_patched, "patched_sections/%s", fn_cur);
       
        FILE* f_patched = fopen(fn_patched, "rb");
        if(!f_patched)
        {
            printf("Failed to open %s\n", fn_patched);
            return -1;
        }
        
        FILE* f_unpatched = fopen(fn_unpatched, "rb");
        // if there's no unpatched file, we should encode the entire file into our patches
        if(!f_unpatched)
        {
            printf("Encoding file %s...\n", fn_cur);
            if(patch_add_to_file(f_patched, f_patch, base_addr))
            {
                printf("Failed to encode file! Aborting.\n");
                return -1;
            }
        }
        // else just diff our two files
        else
        {
            printf("Diffing file %s...\n", fn_cur);
            if(patch_diff_to_file(f_unpatched, f_patched, f_patch, base_addr))
            {
                printf("Failed to diff files! Aborting.\n");
                return -1;
            }
            fclose(f_unpatched);
        }
        
        fclose(f_patched);
        free(fn_patched);
        free(fn_unpatched);
    }
    
    // now add the ELF header! memory is mapped based on it.
    FILE* f_img = fopen("ios.img", "rb");
    if(!f_img)
    {
        printf("Failed to open ios.img for elfhdr copy!\n");
        return -1;
    }
    
    // get phdr table size
    fseek(f_img, 0x804+0x2A, SEEK_SET);
    u16 ent_sz_num[2];
    fread(ent_sz_num, 2, 2, f_img);
    u32 tbl_size = getbe16(&ent_sz_num[1]) * getbe16(&ent_sz_num[0]);
    
    void* elf_phdrs = malloc(tbl_size);
    if(!elf_phdrs)
    {
        printf("Failed to alloc mem for elf phdrs copy!\n");
        return -1;
    }
    
    // read phdrs...
    fseek(f_img, 0x804+0x34, SEEK_SET);
    fread(elf_phdrs, 1, tbl_size, f_img);

    // Now check the notes size and read it
    u32 notes_size = getbe32((u8*)elf_phdrs + 0x30);
    void* elf_notes = malloc(notes_size);
    if(!elf_notes)
    {
        printf("Failed to alloc mem for elf notes copy!\n");
        return -1;
    }
    fread(elf_notes, 1, notes_size, f_img);

    fclose(f_img);
    
    // write phdrs and notes to file as a diff overwrite
    patch_add_diff(f_patch, elf_phdrs, tbl_size/4, 0x1D000000);
    patch_add_diff(f_patch, elf_notes, notes_size/4, 0x1D000000+tbl_size);

    // Add in ios_process (TODO: do this in minute instead?)
    #define CARVEOUT_SZ (0x400000)

    void* ios_proc_elf = malloc(CARVEOUT_SZ);
    FILE* f_ios_proc_elf = fopen("ios_process/ios_process.elf", "rb");
    size_t ios_proc_sz = fread(ios_proc_elf, 1, CARVEOUT_SZ, f_ios_proc_elf);
    patch_add_diff(f_patch, ios_proc_elf, ios_proc_sz/4, 0x28000000-CARVEOUT_SZ);
    fclose(f_ios_proc_elf);

    // TODO: make a trampoline at 0x27FFFFF0 instead?
    u32 jump_addr;
    putbe32((u8*)&jump_addr, (0x28000000-CARVEOUT_SZ) + getbe32((u8*)ios_proc_elf+0x18));

    // TODO: verify this is consistent
    patch_add_diff(f_patch, &jump_addr, 1, 0xFFFF0020);
    
    write_patch_footer_and_hash(f_patch);
    fclose(f_patch);
    printf("Done!\n");
    
    return 0;
}
