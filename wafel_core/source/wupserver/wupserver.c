#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "imports.h"
#include "services/net_ifmgr_ncl.h"
#include "socket.h"
#include "services/fsa.h"
#include "ios/svc.h"
#include "wupserver/text.h"
#include "addrs_55x.h"

void fsa_copydir(int fd, char* src, char* dst)
{
    int src_dir = 0;
    int ret = FSA_OpenDir(fd, src, &src_dir);
    if(ret)
    {
        print_log(0, 8, "Dir %s open failed: %X Aborting...\n", src, ret);
        return;
    }
    // create second dir
    ret = FSA_MakeDir(fd, dst, 0x666);
    if(ret && ret != 0xFFFCFFEA) // not created or existing
    {
        print_log(0, 8, "Destination dir create failed: %X Aborting...\n", ret);
        FSA_CloseDir(fd, src_dir);
        return;
    }
    print_log(0, 0, "Created %s\n", dst);
    
    directoryEntry_s *dir_entry = iosAlloc(0x00001, sizeof(directoryEntry_s));
    if(dir_entry == NULL)
    {
        print_log(0, 8, "Dir entry alloc failed, Aborting...\n", ret);
        FSA_CloseDir(fd, src_dir);
        return;
    }
    print_log(0, 0, "allocated direntry\n");
    
    while(!FSA_ReadDir(fd, src_dir, dir_entry))
    {
        print_log(0, 0, "read dir\n");
        // get new dst strs
        char *src_str = iosAlloc(0x00001,0x100);
        char *dst_str = iosAlloc(0x00001, 0x100);
        snprintf(src_str, 0x100, "%s/%s", src, dir_entry->name);
        snprintf(dst_str, 0x100, "%s/%s", dst, dir_entry->name);
        if(!src_str || !dst_str)
        {
            print_log(0, 8, "Failed to allocate copy stringnames!\n");
            FSA_CloseDir(fd, src_dir);
            iosFree(0x00001, src_str);
            iosFree(0x00001, dst_str);
            return;
        }
        
        // copy this dir
        if(dir_entry->dirStat.flags & 0x80000000)
        {
            fsa_copydir(fd, src_str, dst_str);
            iosFree(0x00001, src_str);
            iosFree(0x00001, dst_str);
        }
        // copy a file
        else
        {
            u32 block_size = 2*1024*1024; // 2MB
            u8* filebuf = iosAlloc(0x00001, block_size);
            if(!filebuf)
            {
                print_log(0, 8, "Failed to allocate filebuf!\n");
                FSA_CloseDir(fd, src_dir);
                return;
            }
            int f_dst = 0, f_src = 0;
            ret = FSA_OpenFile(fd, src_str, "r", &f_src);
            if(ret)
            {
                print_log(0, 8, "Open src file %s failed! %X\n", src_str, ret);
                iosFree(0x00001, filebuf);
                FSA_CloseDir(fd, src_dir);
                return;
            }
            ret = FSA_OpenFile(fd, dst_str, "w", &f_dst);
            if(ret)
            {
                print_log(0, 8, "Open dst file %s failed! %X\n", dst_str, ret);
                iosFree(0x00001, filebuf);
                FSA_CloseFile(fd, f_src);
                FSA_CloseDir(fd, src_dir);
                return;
            }
            
            // print filename and free all our strings
            size_t fsize = dir_entry->dirStat.size;
            print_log(0, 16, "%s size: 0x%X\n", dst_str, fsize);
            iosFree(0x00001, src_str);
            iosFree(0x00001, dst_str);
            
            // copy!!
            u32 cur_offset = 0;
            u32 i = 0;
            while(cur_offset < fsize)
            {
                
                u32 cur_size = (fsize - cur_offset < block_size) ? fsize - cur_offset : block_size;
                int read_sz = FSA_ReadFile(fd, filebuf, 1, cur_size, f_src, 0);
                if(read_sz != cur_size)
                {
                    print_log(0, 8, "Breaking copy loop at read with ret %X\n", read_sz);
                    break;
                }
                int write_sz = FSA_WriteFile(fd, filebuf, 1, cur_size, f_dst, 0);
                if(write_sz != cur_size)
                {
                    print_log(0, 8, "Failed to write during copy with ret %X\n", write_sz);
                    break;
                }
                cur_offset += cur_size;
                // 4*block_size = every 16mb, or on completion
                if((i++ %4 == 0) || cur_offset == fsize) print_log(0, 24, "0x%X of 0x%X bytes copied\n", cur_offset, fsize);
            }
            FSA_CloseFile(fd, f_dst);
            FSA_CloseFile(fd, f_src);
            iosFree(0x00001, filebuf);
            ret = FSA_ChangeMode(fd, dst, 0x666);
            if(ret) print_log(0, 8, "FSA_ChangeMode returned %d\n", ret);
        }
    }
    iosFree(0x00001, dir_entry);
    FSA_CloseDir(fd, src_dir);
}

// TODO : errno stuff ?
bool serverKilled;

// overwrites command_buffer with response
// returns length of response (or 0 for no response, negative for error)
int serverCommandHandler(u32* command_buffer, u32 length, int sock)
{
    if(!command_buffer || !length) return -1;

    int out_length = 4;

    switch(command_buffer[0])
    {
        case 0:
            // write
            // [cmd_id][addr]
            {
                void* dst = (void*)command_buffer[1];
                int length = command_buffer[2];
                
                u32 offset = 0;
                while(offset < length)
                    offset += recv(sock, dst + offset, length - offset, 0);
            }
            break;
        case 1:
            // read
            // [cmd_id][addr][length]
            {
                void* src = (void*)command_buffer[1];
                length = command_buffer[2];
                
                u32 offset = 0;
                while(offset < length)
                    offset += send(sock, src + offset, length - offset, 0);
            }
            break;
        case 2:
            // svc
            // [cmd_id][svc_id]
            {
                int svc_id = command_buffer[1];
                int size_arguments = length - 8;

                u32 arguments[8];
                memset(arguments, 0x00, sizeof(arguments));
                memcpy(arguments, &command_buffer[2], (size_arguments < 8 * 4) ? size_arguments : (8 * 4));

                // return error code as data
                out_length = 8;
                command_buffer[1] = ((int (*const)(u32, u32, u32, u32, u32, u32, u32, u32))(wafel_SyscallTable[svc_id]))(arguments[0], arguments[1], arguments[2], arguments[3], arguments[4], arguments[5], arguments[6], arguments[7]);
            }
            break;
        case 3:
            // kill
            // [cmd_id]
            {
                serverKilled = true;
            }
            break;
        case 4:
            // memcpy
            // [dst][src][size]
            {
                void* dst = (void*)command_buffer[1];
                void* src = (void*)command_buffer[2];
                int size = command_buffer[3];

                memcpy(dst, src, size);
            }
            break;
        case 5:
            // repeated-write
            // [address][value][n]
            {
                u32* dst = (u32*)command_buffer[1];
                u32* cache_range = (u32*)(command_buffer[1] & ~0xFF);
                u32 value = command_buffer[2];
                u32 n = command_buffer[3];

                u32 old = *dst;
                int i;
                for(i = 0; i < n; i++)
                {
                    if(*dst != old)
                    {
                        if(*dst == 0x0) old = *dst;
                        else
                        {
                            *dst = value;
                            iosFlushDCache(cache_range, 0x100);
                            break;
                        }
                    }else
                    {
                        iosInvalidateDCache(cache_range, 0x100);
                        usleep(50);
                    }
                }
            }
            break;
        default:
            // unknown command
            return -2;
            break;
    }
    
    // no error !
    command_buffer[0] = 0x00000000;
    return out_length;
}

void serverClientHandler(int sock)
{
    u32 command_buffer[0x180];

    while(!serverKilled)
    {
        int ret = recv(sock, command_buffer, sizeof(command_buffer), 0);

        if(ret <= 0) break;

        ret = serverCommandHandler(command_buffer, ret, sock);

        if(ret > 0)
        {
            send(sock, command_buffer, ret, 0);
        }else if(ret < 0)
        {
            send(sock, &ret, sizeof(int), 0);
        }
    }

    closesocket(sock);
}

void serverListenClients()
{
    serverKilled = false;

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in server;

    memset(&server, 0x00, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = 1337;
    server.sin_addr.s_addr = 0;

    int ret = bind(sock, (struct sockaddr *)&server, sizeof(server));

    while(!serverKilled)
    {
        ret = listen(sock, 1);
        
        if(ret >= 0)
        {
            int csock = accept(sock, NULL, NULL);
            
            // TODO : threading
            serverClientHandler(csock);
        }else usleep(1000);
    }
}

void mount_sd(int fd, char* path)
{
    // Mount sd to /vol/sdcard
    int ret = FSA_Mount(fd, "/dev/sdcard01", path, 0, NULL, 0);;
    int i = 1;
    while(ret < 0)
    {
        ret = FSA_Mount(fd, "/dev/sdcard01", path, 0, NULL, 0);
        print(0, 8, "Mount SD attempt %d, %X\n", i++, ret);
        usleep(1000);

        if (ret == 0xFFFCFFEA || ret == 0xFFFCFFE6 || i >= 0x100) break;
    }
    print_log(0, 0, "Mounted SD...\n");
}

void mount_mlc(int fd, char* path)
{
    // Mount mlc to path
    int ret = FSA_Mount(fd, "/dev/mlc01", path, 0, NULL, 0);;
    int i = 1;
    while(ret < 0)
    {
        ret = FSA_Mount(fd, "/dev/mlc01", path, 0, NULL, 0);
        print(0, 8, "Mount MLC attempt %d, %X", i++, ret);
        usleep(1000);
    }
    print_log(0, 0, "Mounted MLC...\n");
}

void wait_storages_ready(int fd)
{
    int dir = 0;
    int i = 1;
    int ret = FSA_OpenDir(fd, "/vol/sdcard/", &dir);
    while(ret < 0)
    {
        usleep(1000);
        ret = FSA_OpenDir(fd, "/vol/sdcard/", &dir);
        print(0, 8, "SD open attempt %d %X\n", i++, ret);
    }
    FSA_CloseDir(fd, dir);
    print_log(0, 0, "SD ready!\n");
    
    i = 1;
    ret = FSA_OpenDir(fd, "/vol/storage_mlc01/", &dir);
    while(ret < 0)
    {
        usleep(1000);
        ret = FSA_OpenDir(fd, "/vol/storage_mlc01/", &dir);
        print(0, 8, "MLC open attempt %d %X\n", i++, ret);
    }
    FSA_CloseDir(fd, dir);
    print_log(0, 0, "MLC ready!\n");
}

//#define EMERGENCY_INSTALL

u32 wupserver_main(void* arg)
{
    //clearScreen(0xFF0000FF);
    /*
    // Get FSA ready.
    int fd = FSA_Open();
    int i = 1;
    while(fd < 0)
    {
        usleep(1000);
        fd = FSA_Open();
        print(0, 8, "FSA open attempt %d %X\n", i++, fd);
    }
    clearScreen(0x00FF00FF); // green on fsa open
    
    mount_sd(fd, "/vol/sdcard/");
    //mount_mlc(fd, "/vol/storage_mlc01/");
    wait_storages_ready(fd);
    
    clearScreen(0xBB00FFFF); // cute purple on copy ready
    //fsa_copydir(fd, "/vol/storage_mlc01/", "/vol/sdcard/mlc01bak/"); // backup
    fsa_copydir(fd, "/vol/sdcard/mlc01/usr", "/vol/storage_mlc01/usr"); // restore
    fsa_copydir(fd, "/vol/sdcard/mlc01/sys", "/vol/storage_mlc01/sys"); // restore
    
    clearScreen(0xFFBB00FF); // amber after copy
    int ret = FSA_FlushVolume(fd, "/vol/storage_mlc01");
    print_log(0, 0, "Flush MLC returned %X\n", ret);
    ret = FSA_FlushVolume(fd, "/vol/system_slc");
    print_log(0, 8, "Flush SLC returned %X\n", ret);
    while(1) ;
    */

    usleep(5*1000*1000);

#ifdef EMERGENCY_INSTALL
    // Get FSA ready.
    int fsa_fd = FSA_Open();

    int i = 1;
    while(fsa_fd < 0)
    {
        usleep(1000*1000);
        fsa_fd = FSA_Open();
        print(0, 8, "FSA open attempt %d %X\n", i++, fsa_fd);
    }
    clearScreen(0x00FF00FF); // green on fsa open

    while (1)
    {
        for (int j = 0; j < 15; j++)
        {
            print(0, 8, "sleeping...\n");
            usleep(1*1000*1000);
        }

        mount_sd(fsa_fd, "/vol/sdcard/");
        wait_storages_ready(fsa_fd);

        int mcp_handle = iosOpen("/dev/mcp", 0);
        print(0,8,"%08x", mcp_handle);

        int ret = MCP_InstallTarget(mcp_handle, 0);
        print(0,16,"installtarget : %08x", ret);

        ret = MCP_InstallGetInfo(mcp_handle, "/vol/sdcard/test_install");
        print(0,16,"installinfo : %08x", ret);

        ret = MCP_Install(mcp_handle, "/vol/sdcard/test_install");
        print(0,16,"install : %08x", ret);

        iosClose(mcp_handle);
        //iosClose(fd);
    }
#endif
    
    
    clearScreen(0xBB00FFFF);
    while(ifmgrnclInit() <= 0)
    {
        print(0, 0, "opening /dev/net/ifmgr/ncl...");
        usleep(5*1000*1000);
    }

    while(true)
    {
        u16 out0, out1;

        int ret0 = IFMGRNCL_GetInterfaceStatus(0, &out0);
        if(!ret0 && out0 == 1) break;

        int ret1 = IFMGRNCL_GetInterfaceStatus(1, &out1);
        if(!ret1 && out1 == 1) break;
        
        print(0, 0, "initializing /dev/net/ifmgr/ncl... %08X %08X %08X %08X ", ret0, ret1, out0, out1);

        usleep(5*1000*1000);
    }

    while(socketInit() <= 0)
    {
        print(0, 0, "opening /dev/socket...");
        usleep(1000*1000);
    }

    print(0, 0, "opened /dev/socket !");
    usleep(5*1000*1000);
    print(0, 10, "attempting sockets !");

    serverListenClients();

    while(1)
    {
        usleep(1000*1000);
    }

    return 0;
}
