# may or may not be inspired by plutoo's ctrrpc
from argparse import ArgumentParser
import binascii
import errno
import os
import socket
import struct
import sys
import time

def buffer(size):
    return bytearray([0x00] * size)

def copy_string(buffer, s, offset):
    s += "\0"
    buffer[offset : (offset + len(s))] = bytearray(s, "ascii")

def copy_word(buffer, w, offset):
    buffer[offset : (offset + 4)] = struct.pack(">I", w)

def copy_u64(buffer, w, offset):
    buffer[offset : (offset + 8)] = struct.pack(">Q", w)

def get_string(buffer, offset):
    s = buffer[offset:]
    if 0x00 in s:
        return s[:s.index(0x00)].decode("utf-8")
    else:
        return s.decode("utf-8")

class wupclient:
    s=None

    def __init__(self, ip="192.168.50.19", port=1337):
        self.s=socket.socket()
        self.s.connect((ip, port))
        self.fsa_handle = None
        self.cwd = "/vol/storage_mlc01"

    def __del__(self):
        if self.fsa_handle != None:
            self.close(self.fsa_handle)
            self.fsa_handle = None

    # fundamental comms
    def send(self, command, data):
        request = struct.pack('>I', command) + data

        self.s.send(request)
        response = self.s.recv(0x600)

        ret = struct.unpack(">I", response[:4])[0]
        return (ret, response[4:])

    # core commands

    def read(self, addr, length):
        # read cmd
        request = struct.pack(">III", 1, addr, length)

        self.s.send(request)

        offset = 0
        data = bytearray()
        while offset < length:
            curdata = self.s.recv(length - offset)
            offset += len(curdata)
            data += curdata

        response = self.s.recv(0x600)

        ret = struct.unpack(">I", response[:4])[0]
        if ret == 0:
            return data
        else:
            print("read error : %08X" % ret)
            return None
        

    def write(self, addr, data):
        length = len(data)
        if length == 0:
            return 0
        request = struct.pack(">III", 0, addr, length)
        self.s.send(request)

        offset = 0
        while offset < length:
            offset += self.s.send(data[offset:])
        
        response = self.s.recv(0x600)

        ret = struct.unpack(">I", response[:4])[0]
        if ret == 0:
            return 0
        else:
            print("write error : %08X" % ret)
            return None

    def svc(self, svc_id, arguments):
        data = struct.pack(">I", svc_id)
        for a in arguments:
            data += struct.pack(">I", a)
        ret, data = self.send(2, data)
        if ret == 0:
            return struct.unpack(">I", data)[0]
        else:
            print("svc error : %08X" % ret)
            return None

    def kill(self):
        ret, _ = self.send(3, bytearray())
        return ret

    def memcpy(self, dst, src, len):
        data = struct.pack(">III", dst, src, len)
        ret, data = self.send(4, data)
        if ret == 0:
            return ret
        else:
            print("memcpy error : %08X" % ret)
            return None

    def repeatwrite(self, dst, val, n):
        data = struct.pack(">III", dst, val, n)
        ret, data = self.send(5, data)
        if ret == 0:
            return ret
        else:
            print("repeatwrite error : %08X" % ret)
            return None

    # derivatives
    def alloc(self, size, align = None):
        if size == 0:
            return 0
        if align == None:
            return self.svc(0x27, [0x1, size])
        else:
            return self.svc(0x28, [0x1, size, align])

    def free(self, address):
        if address == 0:
            return 0
        return self.svc(0x29, [0x1, address])

    def load_buffer(self, b, align = None):
        if len(b) == 0:
            return 0
        address = self.alloc(len(b), align)
        self.write(address, b)
        return address

    def load_string(self, s, align = None):
        return self.load_buffer(bytearray(s + "\0", "ascii"), align)

    def open(self, device, mode):
        address = self.load_string(device)
        handle = self.svc(0x33, [address, mode])
        self.free(address)
        return handle

    def close(self, handle):
        return self.svc(0x34, [handle])

    def ioctl(self, handle, cmd, inbuf, outbuf_size):
        in_address = self.load_buffer(inbuf)
        out_data = None
        if outbuf_size > 0:
            out_address = self.alloc(outbuf_size)
            ret = self.svc(0x38, [handle, cmd, in_address, len(inbuf), out_address, outbuf_size])
            out_data = self.read(out_address, outbuf_size)
            self.free(out_address)
        else:
            ret = self.svc(0x38, [handle, cmd, in_address, len(inbuf), 0, 0])
        self.free(in_address)
        return (ret, out_data)
        
    def otpread(self, index, size):
        out_data = None
        out_address = self.alloc(size)
        self.write(out_address, bytearray([0x00] * size))
        ret = self.svc(0x22, [index, out_address, size])
        out_data = self.read(out_address, size)
        self.free(out_address)
        return (ret, out_data)

    def iovec(self, vecs):
        data = bytearray()
        for (a, s) in vecs:
            data += struct.pack(">III", a, s, 0)
        return self.load_buffer(data)

    def ioctlv(self, handle, cmd, inbufs, outbuf_sizes, inbufs_ptr = [], outbufs_ptr = []):
        inbufs = [(self.load_buffer(b, 0x40), len(b)) for b in inbufs]
        outbufs = [(self.alloc(s, 0x40), s) for s in outbuf_sizes]
        iovecs = self.iovec(inbufs + inbufs_ptr + outbufs_ptr + outbufs)
        out_data = []
        ret = self.svc(0x39, [handle, cmd, len(inbufs + inbufs_ptr), len(outbufs + outbufs_ptr), iovecs])
        for (a, s) in outbufs:
            out_data += [self.read(a, s)]
        for (a, _) in (inbufs + outbufs):
            self.free(a)
        self.free(iovecs)
        return (ret, out_data)

    # fsa
    def FSA_Mount(self, handle, device_path, volume_path, flags):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, device_path, 0x0004)
        copy_string(inbuffer, volume_path, 0x0284)
        copy_word(inbuffer, flags, 0x0504)
        (ret, _) = self.ioctlv(handle, 0x01, [inbuffer, bytearray()], [0x293])
        return ret

    def FSA_RawOpen(self, handle, device):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, device, 0x4)
        (ret, data) = self.ioctl(handle, 0x6A, inbuffer, 0x293)
        return (ret, struct.unpack(">I", data[4:8])[0])

    def FSA_FlushVolume(self, handle, device):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, device, 0x4)
        (ret, data) = self.ioctl(handle, 0x1B, inbuffer, 0x293)
        return (ret, struct.unpack(">I", data[4:8])[0])

    def FSA_RawClose(self, handle, file_handle):
        inbuffer = buffer(0x520)
        copy_word(inbuffer, file_handle, 0x4)
        (ret, data) = self.ioctl(handle, 0x6D, inbuffer, 0x293)
        return ret

    def FSA_RawRead(self, handle, device_handle, size, cnt, offset):
        inbuffer = buffer(0x520)
        copy_u64(inbuffer, offset, 0x8)
        copy_word(inbuffer, cnt, 0x10)
        copy_word(inbuffer, size, 0x14)
        copy_word(inbuffer, device_handle, 0x18)
        (ret, data) = self.ioctlv(handle, 0x6B, [inbuffer], [size * cnt, 0x293])
        return (ret, data[0])

    def FSA_RawReadPtr(self, handle, device_handle, size, cnt, offset, ptr):
        inbuffer = buffer(0x520)
        copy_u64(inbuffer, offset, 0x8)
        copy_word(inbuffer, cnt, 0x10)
        copy_word(inbuffer, size, 0x14)
        copy_word(inbuffer, device_handle, 0x18)
        (ret, data) = self.ioctlv(handle, 0x6B, [inbuffer], [0x293], [], [(ptr, size*cnt)])
        return (ret, data[0])

    def FSA_GetDeviceInfo(self, handle, path):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, path, 0x4)
        copy_word(inbuffer, 4, 0x284); #GetDeviceInfo mode
        (ret, data) = self.ioctl(handle, 0x18, inbuffer, 0x293)
        return (ret, data[4:0x2C])

    def FSA_GetVolumeInfo(self, handle, path):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, path, 0x4)
        copy_word(inbuffer, 4, 0x284);
        (ret, data) = self.ioctl(handle, 3, inbuffer, 0x293, 0x1C0)
        return (ret, data[4:0x1C0])
    
    def FSA_OpenDir(self, handle, path):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, path, 0x4)
        (ret, data) = self.ioctl(handle, 0x0A, inbuffer, 0x293)
        return (ret, struct.unpack(">I", data[4:8])[0])

    def FSA_ReadDir(self, handle, dir_handle):
        inbuffer = buffer(0x520)
        copy_word(inbuffer, dir_handle, 0x4)
        (ret, data) = self.ioctl(handle, 0x0B, inbuffer, 0x293)
        data = data[4:]
        unk = data[:0x64]
        if ret == 0:
            return (ret, {"name" : get_string(data, 0x64), "is_file" : (unk[0] & 0x80) != 0x80, "unk" : unk})
        else:
            return (ret, None)

    def FSA_CloseDir(self, handle, dir_handle):
        inbuffer = buffer(0x520)
        copy_word(inbuffer, dir_handle, 0x4)
        (ret, data) = self.ioctl(handle, 0x0D, inbuffer, 0x293)
        return ret

    def FSA_OpenFile(self, handle, path, mode):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, path, 0x4)
        copy_string(inbuffer, mode, 0x284)
        (ret, data) = self.ioctl(handle, 0x0E, inbuffer, 0x293)
        return (ret, struct.unpack(">I", data[4:8])[0])
    
    def FSA_GetStat(self, handle, path):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, path, 0x4)
        copy_word(inbuffer, 0x5, 0x284)
        (ret, data) = self.ioctl(handle, 0x18, inbuffer, 0x293)
        return (ret, data[4:0x68])

    def FSA_Rename(self, handle, src, dst):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, src, 0x4)
        copy_string(inbuffer, dst, 0x284)
        (ret, _) = self.ioctl(handle, 0x09, inbuffer, 0x293)
        return ret
        
    def FSA_Remove(self, handle, path):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, path, 0x4)
        (ret, _) = self.ioctl(handle, 0x08, inbuffer, 0x293)
        return ret

    def FSA_MakeDir(self, handle, path, flags):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, path, 0x4)
        copy_word(inbuffer, flags, 0x284)
        (ret, _) = self.ioctl(handle, 0x07, inbuffer, 0x293)
        return ret

    def FSA_ReadFile(self, handle, file_handle, size, cnt):
        inbuffer = buffer(0x520)
        copy_word(inbuffer, size, 0x08)
        copy_word(inbuffer, cnt, 0x0C)
        copy_word(inbuffer, file_handle, 0x14)
        (ret, data) = self.ioctlv(handle, 0x0F, [inbuffer], [size * cnt, 0x293])
        return (ret, data[0])

    def FSA_WriteFile(self, handle, file_handle, data):
        inbuffer = buffer(0x520)
        copy_word(inbuffer, 1, 0x08) # size
        copy_word(inbuffer, len(data), 0x0C) # cnt
        copy_word(inbuffer, file_handle, 0x14)
        (ret, data) = self.ioctlv(handle, 0x10, [inbuffer, data], [0x293])
        return (ret)

    def FSA_ReadFilePtr(self, handle, file_handle, size, cnt, ptr):
        inbuffer = buffer(0x520)
        copy_word(inbuffer, size, 0x08)
        copy_word(inbuffer, cnt, 0x0C)
        copy_word(inbuffer, file_handle, 0x14)
        (ret, data) = self.ioctlv(handle, 0x0F, [inbuffer], [0x293], [], [(ptr, size*cnt)])
        return (ret, data[0])

    def FSA_WriteFilePtr(self, handle, file_handle, size, cnt, ptr):
        inbuffer = buffer(0x520)
        copy_word(inbuffer, size, 0x08)
        copy_word(inbuffer, cnt, 0x0C)
        copy_word(inbuffer, file_handle, 0x14)
        (ret, data) = self.ioctlv(handle, 0x10, [inbuffer], [0x293], [(ptr, size*cnt)], [])
        return (ret)

    def FSA_CloseFile(self, handle, file_handle):
        inbuffer = buffer(0x520)
        copy_word(inbuffer, file_handle, 0x4)
        (ret, data) = self.ioctl(handle, 0x15, inbuffer, 0x293)
        return ret

    def FSA_ChangeMode(self, handle, path, permission):
        if len(path) > 0x27F:
            print(" Path is too long!");
            return -1;
        inbuffer = buffer(0x520)
        copy_string(inbuffer, path, 4)
        copy_word(inbuffer, permission, 0x284)
        copy_word(inbuffer, 0x666, 0x288)
        (ret, _) = self.ioctl(handle, 0x20, inbuffer, 0x293)
        return ret

    def FSA_MapDir(self, handle, src_dir, dst_dir, flags, arg1, arg2, arg3, arg4):
        inbuffer = buffer(0x520)
        copy_string(inbuffer, src_dir, 4)
        copy_string(inbuffer, dst_dir, 0x284)
        copy_word(inbuffer, flags, 0x504)
        copy_word(inbuffer, arg1, 0x508)
        copy_word(inbuffer, arg2, 0x50C)
        copy_word(inbuffer, arg3, 0x510)
        copy_word(inbuffer, arg4, 0x514)
        copy_word(inbuffer, 0, 0x518)

        (ret, data) = self.ioctlv(handle, 0x67, [inbuffer], [0x293], [(0, 0)])
        return (ret)

    # pm
    def PM_Restart(self, handle):
        (ret, _) = w.ioctl(handle, 0xE3, [], 0)
        return ret

    # mcp
    def MCP_InstallGetInfo(self, handle, path):
        inbuffer = buffer(0x27F)
        copy_string(inbuffer, path, 0x0)
        (ret, data) = self.ioctlv(handle, 0x80, [inbuffer], [0x16])
        return (ret, struct.unpack(">IIIIIH", data[0]))

    # mcp
    def BSP_Read(self, handle, entityName, instance, attributeName, out_size):
        inbuffer = buffer(0x48)
        copy_string(inbuffer, entityName, 0x0)
        copy_word(inbuffer, instance, 0x8*4)
        copy_string(inbuffer, attributeName, 0x9*4)
        copy_word(inbuffer, out_size, 17*4)
        (ret, data) = self.ioctl(handle, 0x05, inbuffer, out_size)
        return (ret, data)

    # mcp
    def BSP_Write(self, handle, entityName, instance, attributeName, data):
        inbuffer = buffer(0x48) + bytearray(data)
        copy_string(inbuffer, entityName, 0x0)
        copy_word(inbuffer, instance, 0x8*4)
        copy_string(inbuffer, attributeName, 0x9*4)
        copy_word(inbuffer, len(data), 17*4)
        (ret, data) = self.ioctl(handle, 0x06, inbuffer, 0)
        return (ret, data)

    def MCP_Install(self, handle, path):
        inbuffer = buffer(0x27F)
        copy_string(inbuffer, path, 0x0)
        (ret, _) = self.ioctlv(handle, 0x81, [inbuffer], [])
        return ret

    def MCP_Delete(self, handle, path, uninstall):
        inbuffer = buffer(0x38)
        copy_string(inbuffer, path, 0x0)
        inbuffer2 = buffer(0x4)
        copy_word(inbuffer2, uninstall, 0)
        (ret, _) = self.ioctlv(handle, 0x83, [inbuffer, inbuffer2], [], [], [])
        return ret

    def MCP_Uninstall(self, handle, path):
        inbuffer = buffer(0x38)
        copy_string(inbuffer, path, 0x0)
        (ret, _) = self.ioctlv(handle, 0x84, [inbuffer], [], [], [])
        return ret

    def MCP_CopyTitle(self, handle, path, dst_device_id, flush):
        inbuffer = buffer(0x27F)
        copy_string(inbuffer, path, 0x0)
        inbuffer2 = buffer(0x4)
        copy_word(inbuffer2, dst_device_id, 0x0)
        inbuffer3 = buffer(0x4)
        copy_word(inbuffer3, flush, 0x0)
        (ret, _) = self.ioctlv(handle, 0x85, [inbuffer, inbuffer2, inbuffer3], [])
        return ret

    def MCP_InstallGetProgress(self, handle):
        (ret, data) = self.ioctl(handle, 0x82, [], 0x24)
        return (ret, struct.unpack(">IIIIIIIII", data))
    
    def MCP_InstallSetTargetDevice(self, handle, device):
        inbuffer = buffer(4)
        copy_word(inbuffer, device, 0)
        (ret, data) = self.ioctl(handle, 0x8D, inbuffer, 0)
        return ret
    
    def MCP_InstallSetTargetUsb(self, handle, device):
        inbuffer = buffer(4)
        copy_word(inbuffer, device, 0)
        (ret, data) = self.ioctl(handle, 0xF1, inbuffer, 0)
        return ret

    def MCP_GetDefaultTitleId(self, handle):
        (ret, data) = self.ioctl(handle, 0x65, [], 0x8)
        return ret, data

    def MCP_SetDefaultTitleId(self, handle, tid):
        inbuffer = buffer(8)
        copy_u64(inbuffer, tid, 0)
        (ret, data) = self.ioctl(handle, 0x62, inbuffer, 0)
        return ret

    # bsp
    def BSPRead(self, handle, device, offset, operation, length):
        inbuffer = buffer(0x48)
        copy_string(inbuffer, device, 0)
        copy_word(inbuffer, offset, 0x20)
        copy_string(inbuffer, operation, 0x24)
        copy_word(inbuffer, length, 0x44)
        (ret, data) = self.ioctl(handle, 0x5, inbuffer, length)
        return ret, data


    # syslog (tmp)
    def dump_syslog(self):
        syslog_address = struct.unpack(">I", self.read(0x05095ECC, 4))[0]
        
        log_size = struct.unpack(">I", w.read(syslog_address, 4))[0]
        print("Dumping log of size " + str(log_size))
        syslog_address += 0x10
        
        data = self.read(syslog_address, log_size)
        f = open("syslog.txt", "wb")
        f.write(data)
        #print(data.decode("ascii")) #print

    # file management
    def get_fsa_handle(self):
        if self.fsa_handle == None:
            self.fsa_handle = self.open("/dev/fsa", 0)
        return self.fsa_handle

    def mkdir(self, path, flags):
        fsa_handle = self.get_fsa_handle()
        ret = w.FSA_MakeDir(fsa_handle, path, flags)
        if ret == 0:
            return 0
        if ret == 0xFFFCFFEA:
            print("mkdir: dir already exists")
            return 0
        else:
            print("mkdir error (%s, %08X)" % (path, ret))
            return ret

    def chmod(self, path, mode):
        if path[0] != "/" and self.cwd[0] == "/":
            return self.chmod(self.cwd + "/" + path, mode)
        fsa_handle = self.get_fsa_handle()
        ret = self.FSA_ChangeMode(fsa_handle, path, mode)
        if ret == 0:
            return 0
        else:
            print("chmod error (%s, %08X)" % (path, ret))
            return ret
        
    def chmod_r(self, path, mode):
        if path[0] != "/" and self.cwd[0] == "/":
            return self.chmod_r(self.cwd + "/" + path)

        self.chmod(path, mode)
        entries = self.ls(path, True)
        for e in entries:
            print(path + "/" + e["name"])
            if e["is_file"]:
                self.chmod(path + "/" + e["name"], mode)
            else:
                self.chmod_r(path + "/" + e["name"], mode)

    def cd(self, path):
        if path[0] != "/" and self.cwd[0] == "/":
            return self.cd(self.cwd + "/" + path)
        fsa_handle = self.get_fsa_handle()
        ret, dir_handle = self.FSA_OpenDir(fsa_handle, path if path != None else self.cwd)
        if ret == 0:
            self.cwd = path
            self.FSA_CloseDir(fsa_handle, dir_handle)
            return 0
        else:
            print("cd error : path does not exist (%s)" % (path))
            return -1

    def rm(self, path):
        fs_handle = self.get_fsa_handle()
        ret = self.FSA_Remove(fs_handle, path)
        if ret == 0:
            return 0
        else:
            print("rm error : " + hex(ret))
            return -1

    def rmdir(self, path):
        entries = self.ls(path, True)
        for e in entries:
            cur_dir = path + "/" + e["name"]
            print(cur_dir)
            if e["is_file"] == False:
                self.rmdir(cur_dir)
            self.rm(cur_dir)
        self.rm(path)

    def ls(self, path = None, return_data = False):
        fsa_handle = self.get_fsa_handle()
        ret, dir_handle = self.FSA_OpenDir(fsa_handle, path if path != None else self.cwd)
        if ret != 0x0:
            print("opendir error : " + hex(ret))
            return [] if return_data else None
        entries = []
        while True:
            ret, data = self.FSA_ReadDir(fsa_handle, dir_handle)
            if ret != 0:
                break
            if not(return_data):
                if data["is_file"]:
                    print("     %s" % data["name"])
                else:
                    print("     %s/" % data["name"])
            else:
                entries += [data]
        ret = self.FSA_CloseDir(fsa_handle, dir_handle)
        return entries if return_data else None

    def dldir(self, path):
        entries = self.ls(path, True)
        for e in entries:
            print(path + "/" + e["name"])
            if e["is_file"]:
                self.dl(path + "/" + e["name"])
            else:
                mkdir_p(e["name"])
                os.chdir(e["name"])
                self.dldir(path + "/" + e["name"])
                os.chdir("..")

    def cpdir(self, srcpath, dstpath):
        entries = self.ls(srcpath, True)
        q = [(srcpath, dstpath, e) for e in entries]
        while len(q) > 0:
            _srcpath, _dstpath, e = q.pop()
            _srcpath += "/" + e["name"]
            _dstpath += "/" + e["name"]
            if e["is_file"]:
                print(e["name"])
                self.cp(_srcpath, _dstpath)
            else:
                self.mkdir(_dstpath, 0x666)
                entries = self.ls(_srcpath, True)
                q += [(_srcpath, _dstpath, e) for e in entries]

    def pwd(self):
        return self.cwd

    def cp(self, filename_in, filename_out):
        fsa_handle = self.get_fsa_handle()
        ret, in_file_handle = self.FSA_OpenFile(fsa_handle, filename_in, "r")
        if ret != 0x0:
            print("cp error : could not open " + filename_in + "with ret" + hex(ret))
            return
        ret, out_file_handle = self.FSA_OpenFile(fsa_handle, filename_out, "w")
        if ret != 0x0:
            print("cp error : could not open " + filename_out + "with ret" + hex(ret))
            return
        block_size = 0x100000
        buffer = self.alloc(block_size, 0x40)
        k = 0
        while True:
            ret, _ = self.FSA_ReadFilePtr(fsa_handle, in_file_handle, 0x1, block_size, buffer)
            k += ret
            ret = self.FSA_WriteFilePtr(fsa_handle, out_file_handle, 0x1, ret, buffer)
            sys.stdout.write(hex(k) + "\r"); sys.stdout.flush();
            if ret < 1:
                break
        self.free(buffer)
        ret = self.FSA_CloseFile(fsa_handle, out_file_handle)
        ret = self.FSA_CloseFile(fsa_handle, in_file_handle)
        self.chmod(filename_out, 0x666)
        self.FSA_FlushVolume(fsa_handle, '/vol/storage_mlc01')
        self.FSA_FlushVolume(fsa_handle, '/vol/storage_usb01')
        self.FSA_FlushVolume(fsa_handle, '/vol/storage_slc01')

    def df(self, filename_out, src, size):
        fsa_handle = self.get_fsa_handle()
        ret, out_file_handle = self.FSA_OpenFile(fsa_handle, filename_out, "w")
        if ret != 0x0:
            print("df error : could not open " + filename_out)
            return
        block_size = 0x10000
        buffer = self.alloc(block_size, 0x40)
        k = 0
        while k < size:
            cur_size = min(size - k, block_size)
            self.memcpy(buffer, src + k, cur_size)
            k += cur_size
            ret = self.FSA_WriteFilePtr(fsa_handle, out_file_handle, 0x1, cur_size, buffer)
            sys.stdout.write(hex(k) + " (%f) " % (float(k * 100) / size) + "\r"); sys.stdout.flush();
        self.free(buffer)
        ret = self.FSA_CloseFile(fsa_handle, out_file_handle)


    def dl(self, filename, local_filename = None):
        fsa_handle = self.get_fsa_handle()
        if filename[0] != "/":
            filename = self.cwd + "/" + filename
        if local_filename == None:
            if "/" in filename:
                local_filename = filename[[i for i, x in enumerate(filename) if x == "/"][-1]+1:]
            else:
                local_filename = filename
        ret, file_handle = self.FSA_OpenFile(fsa_handle, filename, "r")
        if ret != 0x0:
            print("dl error : could not open " + filename)
            return

        # create our remote buffer
        block_size = 0x40000
        chunk_buf = w.alloc(block_size, 0x40)
        if chunk_buf == 0:
            print("failed to alloc chunk buffer!")
            return

        ret = 0
        offs = 0
        f = open(local_filename, "wb")
        while 1:
            offs += block_size
            ret, data = self.FSA_ReadFilePtr(fsa_handle, file_handle, 0x1, block_size, chunk_buf)
            if ret == block_size:
                print(hex(offs))
            
            #read 0 bytes, or error
            if ret < 1:
                break
            
            f.write(w.read(chunk_buf, ret))

        ret = self.FSA_CloseFile(fsa_handle, file_handle)
        f.close()
        self.free(chunk_buf)


    def fr(self, filename, offset, size):
        fsa_handle = self.get_fsa_handle()
        if filename[0] != "/":
            filename = self.cwd + "/" + filename
        ret, file_handle = self.FSA_OpenFile(fsa_handle, filename, "r")
        if ret != 0x0:
            print("fr error : could not open " + filename)
            return
        buffer = bytearray()
        block_size = 0x400
        while True:
            ret, data = self.FSA_ReadFile(fsa_handle, file_handle, 0x1, block_size if (block_size < size) else size)
            buffer += data[:ret]
            sys.stdout.write(hex(len(buffer)) + "\r"); sys.stdout.flush();
            if len(buffer) >= size:
                break
        ret = self.FSA_CloseFile(fsa_handle, file_handle)
        return buffer

    def fw(self, filename, offset, buffer):
        fsa_handle = self.get_fsa_handle()
        if filename[0] != "/":
            filename = self.cwd + "/" + filename
        ret, file_handle = self.FSA_OpenFile(fsa_handle, filename, "r+")
        if ret != 0x0:
            print("fw error : could not open " + filename)
            return
        block_size = 0x400
        k = 0
        while True:
            cur_size = min(len(buffer) - k, block_size)
            if cur_size <= 0:
                break
            sys.stdout.write(hex(k) + "\r"); sys.stdout.flush();
            ret = self.FSA_WriteFile(fsa_handle, file_handle, buffer[k:(k+cur_size)])
            k += cur_size
        ret = self.FSA_CloseFile(fsa_handle, file_handle)

    def up(self, local_filename, filename = None):
        fsa_handle = self.get_fsa_handle()
        if filename == None:
            if "/" in local_filename:
                filename = local_filename[[i for i, x in enumerate(local_filename) if x == "/"][-1]+1:]
            else:
                filename = local_filename
        if filename[0] != "/":
            filename = self.cwd + "/" + filename
        f = open(local_filename, "rb")
        ret, file_handle = self.FSA_OpenFile(fsa_handle, filename, "w")
        if ret != 0x0:
            print("up error : could not open " + filename)
            return
        progress = 0
        block_size = 0x40000
        while True:
            data = f.read(block_size)
            ret = self.FSA_WriteFile(fsa_handle, file_handle, data)
            progress += len(data)
            sys.stdout.write(hex(progress) + "\r"); sys.stdout.flush();
            if len(data) < block_size:
                break
        ret = self.FSA_CloseFile(fsa_handle, file_handle)

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise

def mount_sd():
    handle = w.open("/dev/fsa", 0)
    print(hex(handle))

    ret = w.FSA_Mount(handle, "/dev/sdcard01", "/vol/storage_sdcard", 2)
    print(hex(ret))

    ret = w.close(handle)
    print(hex(ret))

def mount_usbfat():
    handle = w.open("/dev/fsa", 0)
    print(hex(handle))

    ret = w.FSA_Mount(handle, "/dev/usb01", "/vol/storage_usb01", 2)
    print(hex(ret))

    ret = w.close(handle)
    print(hex(ret))

def mount_usbfat_ext():
    handle = w.open("/dev/fsa", 0)
    print(hex(handle))

    ret = w.FSA_Mount(handle, "/dev/ext02", "/vol/storage_usb02", 2)
    print(hex(ret))

    ret = w.close(handle)
    print(hex(ret))

def mount_slccmpt():
    handle = w.open("/dev/fsa", 0)
    print(hex(handle))

    ret = w.FSA_Mount(handle, "/dev/slccmpt01", "/vol/storage_slccmpt", 2)
    print(hex(ret))

    ret = w.close(handle)
    print(hex(ret))

def mount_odd():
    handle = w.open("/dev/fsa", 0)
    print(hex(handle))

    ret = w.FSA_Mount(handle, "/dev/odd04", "/vol/storage_test", 2)
    print(hex(ret))

    ret = w.close(handle)
    print(hex(ret))

def install_title(path):
    mcp_handle = w.open("/dev/mcp", 0)
    print(hex(mcp_handle))

    ret, data = w.MCP_InstallGetInfo(mcp_handle, "/vol/storage_sdcard/"+path)
    print("install info : " + hex(ret), [hex(v) for v in data])

    ret = w.MCP_Install(mcp_handle, "/vol/storage_sdcard/"+path)
    print("install : " + hex(ret))

    ret = w.close(mcp_handle)
    print(hex(ret))


def dldev(device, local_filename = None):
        fsa_handle = w.get_fsa_handle()
            
        ret, dev_handle = w.FSA_RawOpen(fsa_handle, device)
        if ret != 0x0:
            print("dldev: could not open " + device)
            return

        # create our remote buffer
        block_size = 0x8000
        chunk_buf = w.alloc(block_size, 0x40)
        if chunk_buf == 0:
            print("dldev: failed to alloc chunk buffer!")
            return

        if local_filename == None:
            local_filename = device # this is bad
        f = open(local_filename, "wb")

        ret = 0
        offs = 0
        while 1:
            ret, data = w.FSA_RawReadPtr(fsa_handle, dev_handle, 0x1, block_size, offs, chunk_buf)
            
            print(hex(ret))
            #read 0 bytes, or error
            if ret < 0:
                print("err %d, closing file" % ret)
                break
            #if == 0:
                #break
            
            f.write(w.read(chunk_buf, block_size))
            offs += block_size

        ret = w.FSA_RawClose(fsa_handle, dev_handle)
        f.close()
        w.free(chunk_buf)

def cpdev(device, dst_filename, dump_size = 0):        
        fsa_handle = w.get_fsa_handle()
        
        ret, dev_handle = w.FSA_RawOpen(fsa_handle, device)
        if ret != 0x0:
            print("dldev: could not open " + device)
            return

        # create our remote buffer
        block_size = 0x200000 #2mb
        chunk_buf = w.alloc(block_size, 0x40)
        if chunk_buf == 0:
            print("dldev: failed to alloc chunk buffer!")
            return

        ret, data = w.FSA_GetDeviceInfo(fsa_handle, device)
        if ret & 0x80000000:
            print("Error %s getting device info!" % hex(ret))
            free(chunk_buf)
            w.FSA_RawClose(fsa_handle, dev_handle)
            return

        # hack to handle specific devices which will crash if over-dumped
        max_size = 0
        if device.startswith("/dev/odd"):
            max_size = 0x5D3A00000
        if dump_size != 0:
            max_size = dump_size
        
        time_start = time.time()
        
        ret = 0
        f = 0
        offs = 0
        offs_old = 0
        time_old = 0
        time_new = 0
        part = 0
        # copy until we error out
        while 1:
            # Open a new file every 2GB
            if offs % 0x80000000 == 0:
                part += 1
                print("Opening new file %s.%02d",dst_filename, part)
                w.FSA_CloseFile(fsa_handle, f);
                ret, f = w.FSA_OpenFile(fsa_handle, dst_filename + ".%02d" % part , "wb")
                if ret & 0x80000000:
                    print("Error %s opening output file" % hex(ret))
                    w.free(chunk_buf)
                    return
                
            ret, data = w.FSA_RawReadPtr(fsa_handle, dev_handle, 0x1, block_size, offs, chunk_buf)
            if ret & 0x80000000:
                print("err %d, closing file" % ret)
                break
            
            w.FSA_WriteFilePtr(fsa_handle, f, 1, block_size, chunk_buf)
            offs += block_size
            mb_total = offs / 0x100000

            # stop dumping at max_size if one is set
            if max_size != 0 and offs >= max_size:
                break

            # print progress every 4mb
            if offs % 0x400000 == 0:
                num_mbs = (offs - offs_old) / 0x100000 
                offs_old = offs
                time_new = time.time()
                seconds = time_new - time_old
                mb_per_sec = num_mbs/seconds
                time_old = time_new
                
                print("%d MB dumped, %.2f MB/s..." % (mb_total, mb_per_sec))

            
        ret = w.FSA_RawClose(fsa_handle, dev_handle)
        w.FSA_CloseFile(fsa_handle, f);
        w.free(chunk_buf)
        print("Done! Took %d seconds." % time.time() - time_start)
        
def print_keyslot(obj_id):
    # get keydata index
    kd_index = struct.unpack(">H", w.read(0x402BB2C + (obj_id * 0x10) + 2, 2))[0]

    data = bytearray()
    while kd_index:
        data += w.read(0x0402972C + (0x24*kd_index) + 1, 0x20)
        kd_index = struct.unpack(">H", w.read(0x0402972C + (0x24*kd_index) + 0x22, 2))[0]

    print(binascii.hexlify(data))
    
def get_disk_key():
    # get ODM crypto object's index
    crypto_obj_index = struct.unpack(">I", w.read(0x1167BDA0+0x409D4, 4))[0]
                                     
    # get keydata index
    kd_index = struct.unpack(">H", w.read(0x402BB2C + (crypto_obj_index * 0x10) + 2, 2))[0]
    
    # key at keydata + 0x1
    key = w.read(0x0402972C + (0x24*kd_index) + 1, 0x10)
    print(binascii.hexlify(key))
    
def install_target_usb01():
    mcp_handle = w.open("/dev/mcp", 0)
    print(hex(mcp_handle))

    ret = w.MCP_InstallSetTargetDevice(mcp_handle, 1)
    print(hex(ret))
    
    ret = w.MCP_InstallSetTargetUsb(mcp_handle, 1)
    print(hex(ret))

    ret = w.close(mcp_handle)
    print(hex(ret))

def install_target_mlc():
    mcp_handle = w.open("/dev/mcp", 0)
    print(hex(mcp_handle))

    ret = w.MCP_InstallSetTargetDevice(mcp_handle, 0)
    print(hex(ret))

    ret = w.close(mcp_handle)
    print(hex(ret))

def install_usb(path):
    install_target_usb01()
    install_title(path)

def print_devinfo(path):
    fsa_handle = w.get_fsa_handle()
    
    if fsa_handle & 0x80000000:
        print("Error %s opening FSA!" % hex(fsa_handle))
        return
    
    ret, data = w.FSA_GetDeviceInfo(fsa_handle, path)
    if ret & 0x80000000:
        print("Error %s getting device info!" % hex(ret))
        return

    name = data[0x1C:0x24].decode("ascii")
    sec_size = binascii.hexlify(data[0x10:0x14])
    num_sectors = binascii.hexlify(data[0x8:0x10])
    print("Name %s, sector size %s, %s sectors" % (name, sec_size, num_sectors))

    print(binascii.hexlify(data[0:0x10]))
    print(binascii.hexlify(data[0x10:0x20]))
    print(binascii.hexlify(data[0x20:0x28]))

def restart():
    pm_handle = w.open("/dev/pm", 0)
    print(hex(pm_handle))

    ret = w.PM_Restart(pm_handle)
    print(hex(ret))

    ret = w.close(pm_handle)
    print(hex(ret))

def get_nim_status():
    nim_handle = w.open("/dev/nim", 0)
    print(hex(nim_handle))
    
    inbuffer = buffer(0x80)
    (ret, data) = w.ioctlv(nim_handle, 0x00, [inbuffer], [0x80])

    print(hex(ret), "".join("%02X" % v for v in data[0]))

    ret = w.close(nim_handle)
    print(hex(ret))

def read_and_print(adr, size):
    data = w.read(adr, size)
    data = struct.unpack(">%dI" % (len(data) // 4), data)
    for i in range(0, len(data), 4):
        print(" ".join("%08X"%v for v in data[i:i+4]))

def stat(path):
    if path[0] != "/" and self.cwd[0] == "/":
        return finfo(self.cwd + "/" + path)
    fsa_handle = w.get_fsa_handle()

    ret, stat = w.FSA_GetStat(fsa_handle, path)
    if ret != 0:
        print("Failed to stat %s with err %s" % (path, hex(ret)))
        return
    
    #print(binascii.hexlify(stat))
    flags = int.from_bytes(stat[0:4], byteorder='big')
    print("Flags:        0x%08X" % flags)
    print("Permissions:  %X" % int.from_bytes(stat[4:8], byteorder='big'))
    print("Owner:        %X" % int.from_bytes(stat[8:12], byteorder='big'))
    print("Group:        %X" % int.from_bytes(stat[12:16], byteorder='big'))
    print("Size:         0x%X" % int.from_bytes(stat[16:20], byteorder='big'))
    print("Size on disk: 0x%X" % int.from_bytes(stat[20:24], byteorder='big'))

def send_shell_cmd(command):
    inbuffer = buffer(len(command) + 1)
    copy_string(inbuffer, command, 0)
    sh_handle = w.open('/dev/iopsh', 0)
    w.ioctl(sh_handle, 4, inbuffer, 0)
    w.close(sh_handle)

def haxchi_install(install_coldboot = False):
    fsa_handle = w.get_fsa_handle()
    names = ["Kirby: Squeak Squad (U)", "Kirby: Mouse Attack (E)", "Star Fox Command (E)"]
    romzips = ["rom-kirby.squeak.squad-us.zip", "rom-kirby.squeak.squad-eu.zip", "rom-star.fox.command-eu.zip"]
    tids = ["101a5600", "101a5700", "101ac200"]
    active_tid = 0

    # get stats on installed hax games
    valid_tids = bytearray(len(tids))
    num_valid = 0
    i = 0
    for name, tid in zip(names, tids):
        ret, stat = w.FSA_GetStat(fsa_handle, '/vol/storage_mlc01/usr/title/00050000/'+tid+'/content/0010/rom.zip')
        if(ret == 0):
            print(str(i+1)+'.  '+name)
            valid_tids[i] = 1
            num_valid += 1
        i += 1

    title_index = int(input("Select a title to install to...: "), 10)-1
    if title_index+1 > num_valid or title_index < 0:
        print("Invalid title selection!")
        return
    active_tid = tids[title_index]

    w.up('../../uhshax/'+romzips[title_index], '/vol/storage_mlc01/usr/title/00050000/'+tids[title_index]+'/content/0010/rom.zip')

    if(install_coldboot == True):
        mh = w.open('/dev/mcp', 0)
        ret = w.MCP_SetDefaultTitleId(mh, 0x0005000000000000 | int(tids[title_index], 16))
        if(ret == 0):
            print("Coldboot installed successfully!")
        else:
            print("Coldboot install returned "+hex(ret))
        w.close(mh)
                    
        
        
    w.FSA_FlushVolume(fsa_handle, '/vol/storage_mlc01/')
    print("Done!")
    
def clock_test(val):
    bsp_handle = w.open("/dev/bsp", 0)
    print(hex(bsp_handle))

    ret, data = w.BSP_Read(bsp_handle, "GFX", 0, "spll", 0x36)
    print("read : " + hex(ret), data)

    fastEn,dithEn,satEn,ssEn,bypOut,bypVco,operational,clkR,clkFMsb,clkFLsb,clkO0Div,clkO1Div,clkO2Div,clkS,clkVMsb,clkVLsb,bwAdj,options = struct.unpack(">IIIIIIIHHHHHHHHHII", data)
    clkFMsb = val
    write_data = struct.pack(">IIIIIIIHHHHHHHHHII", fastEn,dithEn,satEn,ssEn,bypOut,bypVco,operational,clkR,clkFMsb,clkFLsb,clkO0Div,clkO1Div,clkO2Div,clkS,clkVMsb,clkVLsb,bwAdj,options)

    # Calculate the frequency and show it in console
    clkF = (clkFMsb << 16) | (clkFLsb << 1)
    clkO = clkO0Div
    freqMhz = 27 * (clkF/0x10000) / (clkR+1) / (clkO/2)
    print("Setting clocks to: " + str(freqMhz) + " MHz")

    #print (data)
    ret, data = w.BSP_Write(bsp_handle, "GFX", 0, "spll", write_data)
    print("write : " + hex(ret), data)

    ret, new_data = w.BSP_Read(bsp_handle, "GFX", 0, "spll", 0x36)
    print("read : " + hex(ret), new_data)
    print("applied:", write_data == new_data)

    ret = w.close(bsp_handle)
    print(hex(ret))

def clock_test_loop():
    while True:
        clock_test(0x28)
        time.sleep(5)
        clock_test(0x29)
        time.sleep(5)
        clock_test(0x2A)
        time.sleep(5)
        clock_test(0x2B)
        time.sleep(5)
        clock_test(0x2C)
        time.sleep(5)
        clock_test(0x2D)
        time.sleep(5)
        clock_test(0x2E)
        time.sleep(5)
        clock_test(0x2F)
        time.sleep(5)
        clock_test(0x30)
        time.sleep(5)
        clock_test(0x32)
        time.sleep(5)
        clock_test(0x33)
        time.sleep(5)
        clock_test(0x34)
        time.sleep(5)


if __name__ == '__main__':
    parser = ArgumentParser(prog="wupclient", usage="%(prog)s [ip] (port)")
    parser.add_argument("ip")
    parser.add_argument("port", default=1337, nargs='?', type=int)
    args = parser.parse_args()
    w = wupclient(args.ip, args.port)