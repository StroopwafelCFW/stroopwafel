import sys
import struct
import hashlib
import base64
from Crypto.Cipher import AES

class elf32_ehdr:
	def __init__(self, file, offset):
		file.seek(offset)
		self.e_ident = file.read(16)
		self.e_type = struct.unpack(">H", file.read(2))[0]
		self.e_machine = struct.unpack(">H", file.read(2))[0]
		self.e_version = struct.unpack(">I", file.read(4))[0]
		self.e_entry = struct.unpack(">I", file.read(4))[0]
		self.e_phoff = struct.unpack(">I", file.read(4))[0]
		self.e_shoff = struct.unpack(">I", file.read(4))[0]
		self.e_flags = struct.unpack(">I", file.read(4))[0]
		self.e_ehsize = struct.unpack(">H", file.read(2))[0]
		self.e_phentsize = struct.unpack(">H", file.read(2))[0]
		self.e_phnum = struct.unpack(">H", file.read(2))[0]
		self.e_shentsize = struct.unpack(">H", file.read(2))[0]
		self.e_shnum = struct.unpack(">H", file.read(2))[0]
		self.e_shstrndx = struct.unpack(">H", file.read(2))[0]

	def write(self, file, offset):
		file.seek(offset)
		file.write(self.e_ident)
		file.write(struct.pack(">HHIIIIIHHHHHH", self.e_type, self.e_machine, self.e_version, self.e_entry, self.e_phoff, self.e_shoff, self.e_flags, self.e_ehsize, self.e_phentsize, self.e_phnum, self.e_shentsize, self.e_shnum, self.e_shstrndx))

	def _print(self):
		# print("e_ident : " + (self.e_ident))
		print("e_type : " + hex(self.e_type))
		print("e_machine : " + hex(self.e_machine))
		print("e_version : " + hex(self.e_version))
		print("e_entry : " + hex(self.e_entry))
		print("e_phoff : " + hex(self.e_phoff))
		print("e_shoff : " + hex(self.e_shoff))
		print("e_flags : " + hex(self.e_flags))
		print("e_ehsize : " + hex(self.e_ehsize))
		print("e_phentsize : " + hex(self.e_phentsize))
		print("e_phnum : " + hex(self.e_phnum))
		print("e_shentsize : " + hex(self.e_shentsize))
		print("e_shnum : " + hex(self.e_shnum))
		print("e_shstrndx : " + hex(self.e_shstrndx))

class elf32_phdr:
	def __init__(self, file, offset, hdr, id):
		if id >= hdr.e_phnum:
			raise ValueError("id is invalid yo")
		self.id = id
		if file is not None:
			file.seek(offset + hdr.e_phoff + hdr.e_phentsize * id)
			self.p_type = struct.unpack(">I", file.read(4))[0]
			self.p_offset = struct.unpack(">I", file.read(4))[0]
			self.p_vaddr = struct.unpack(">I", file.read(4))[0]
			self.p_paddr = struct.unpack(">I", file.read(4))[0]
			self.p_filesz = struct.unpack(">I", file.read(4))[0]
			self.p_memsz = struct.unpack(">I", file.read(4))[0]
			self.p_flags = struct.unpack(">I", file.read(4))[0]
			self.p_align = struct.unpack(">I", file.read(4))[0]
			file.seek(offset + self.p_offset)
			self.content = file.read(self.p_filesz)
		else:
			self.content = b''
			self.p_type = 0
			self.p_offset = 0
			self.p_vaddr = 0
			self.p_paddr = 0
			self.p_filesz = 0
			self.p_memsz = 0
			self.p_flags = 0
			self.p_align = 0

	def replace_content(self, file):
		file.seek(0)
		self.content = file.read()
		# should check that it's not too big
		self.p_filesz = len(self.content)
		self.p_memsz = self.p_filesz

	def write(self, file, offset, hdr, id, write_content = True):
		file.seek(offset + hdr.e_phoff + hdr.e_phentsize * id)
		file.write(struct.pack(">IIIIIIII", self.p_type, self.p_offset, self.p_vaddr, self.p_paddr, self.p_filesz, self.p_memsz, self.p_flags, self.p_align))
		if write_content:
			file.seek(offset + self.p_offset)
			file.write(self.content)
			return len(self.content)
		return 0

	def _print(self):
		print("p_type : " + hex(self.p_type))
		print("p_offset : " + hex(self.p_offset))
		print("p_vaddr : " + hex(self.p_vaddr))
		print("p_paddr : " + hex(self.p_paddr))
		print("p_filesz : " + hex(self.p_filesz))
		print("p_memsz : " + hex(self.p_memsz))
		print("p_flags : " + hex(self.p_flags))
		print("p_align : " + hex(self.p_align))

class elf32:
	def __init__(self, file, offset):
		self.hdr = elf32_ehdr(file, offset)
		self.phdrs = [elf32_phdr(file, offset, self.hdr, i) for i in range(self.hdr.e_phnum)]

	def write(self, file, offset):
		self.hdr.write(file, offset)
		data_offset = self.hdr.e_phoff + self.hdr.e_phentsize * self.hdr.e_phnum
		for i in range(self.hdr.e_phnum):
			special = (i == 0 or i == 2)
			if not(special):
				self.phdrs[i].p_offset = data_offset
			if i == 0:
				self.phdrs[i].p_filesz = self.hdr.e_phentsize * self.hdr.e_phnum
				self.phdrs[i].p_memsz = self.phdrs[i].p_filesz
			if i == 1:
				self.phdrs[i].p_vaddr = self.phdrs[0].p_vaddr + self.phdrs[0].p_filesz
				self.phdrs[i].p_paddr = self.phdrs[i].p_vaddr
			if i == 2:
				self.phdrs[i].p_filesz = (self.hdr.e_phentsize * self.hdr.e_phnum) + self.phdrs[i-1].p_filesz
			data_offset += self.phdrs[i].write(file, offset, self.hdr, i, not(special))

	def extract_sections(self, sections):
		for adr in sections:
			for i in range(self.hdr.e_phnum):
				if self.phdrs[i].p_vaddr == adr:
					open(sections[adr], "wb").write(self.phdrs[i].content)
					break

	def replace_sections(self, sections):
		for adr in sections:
			for i in range(self.hdr.e_phnum):
				if self.phdrs[i].p_vaddr == adr:
					self.phdrs[i].replace_content(open(sections[adr], "rb"))
					break

	def bss_sections(self, sections):
		for adr in sections:
			for i in range(self.hdr.e_phnum):
				if self.phdrs[i].p_vaddr == adr:
					f = open(sections[adr], "rb")
					f.seek(0, 2)
					self.phdrs[i].p_filesz = 0
					self.phdrs[i].p_memsz = f.tell()
					break

	def insert_phdr(self, addr):
		phdr_num = 0
		for i in range(0, self.hdr.e_phnum):
			if self.phdrs[i].p_vaddr > addr:
				if phdr_num == 0:
					phdr_num = i
				self.phdrs[i].id += 1
		self.hdr.e_phnum += 1
		phdr = elf32_phdr(None, 0, self.hdr, phdr_num)
		
		self.phdrs = self.phdrs[0:phdr_num] + [phdr] + self.phdrs[phdr_num:]
		return phdr_num

	def _print(self):
		self.hdr._print()
		for i in range(self.hdr.e_phnum):
			print("PHDR " + hex(i))
			self.phdrs[i]._print()

class ancast:
	def __init__(self, file, no_crypto):
		file.seek(0)
		self.header = file.read(0x1000)
		self.elf_offs = 0
		for i in range(0, 0x1000, 4):
			if self.header[i:i+4] == b'\x7FELF':
				self.elf_offs = i
				break
		self.header = self.header[0:self.elf_offs]
		self.elf = elf32(file, self.elf_offs)
		self.no_crypto = no_crypto
		# self.elf._print()

	def write(self, file):
		file.seek(0)
		file.write(self.header)
		self.elf.write(file, len(self.header))
		file.seek(0, 2)
		size = file.tell() - 0x200
		total_size = (size + 0xfff) & ~0xfff
		file.write(bytearray([0x00] * (total_size - size)))
		hash = self.encrypt(file, 0x200, self.no_crypto)
		if no_crypto:
			file.seek(0x1A0)
			file.write(struct.pack(">H", 0x0001))
		file.seek(0x1AC)
		file.write(struct.pack(">I", total_size))
		file.write(hash)

	def write_elf(self, file):
		self.elf.write(file, 0)

	def extract_sections(self, sections):
		self.elf.extract_sections(sections)

	def replace_sections(self, sections):
		self.elf.replace_sections(sections)

	def bss_sections(self, sections):
		self.elf.bss_sections(sections)

	def elf_proc(self, addr, fpath):
		f = open(fpath, "rb")
		elf_dat = f.read()
		f.close()

		carveout_sz = 0x100000
		phdr_num = self.elf.insert_phdr(addr)
		self.elf.phdrs[phdr_num].content = elf_dat
		self.elf.phdrs[phdr_num].p_type = 1   # LOAD
		self.elf.phdrs[phdr_num].p_offset = 0 # filled in
		self.elf.phdrs[phdr_num].p_vaddr = addr
		self.elf.phdrs[phdr_num].p_paddr = addr # ramdisk is consistent so we can do this.
		self.elf.phdrs[phdr_num].p_filesz = carveout_sz#len(elf_dat)
		self.elf.phdrs[phdr_num].p_memsz = carveout_sz#len(elf_dat)
		self.elf.phdrs[phdr_num].p_flags = 7 | (0x1<<20) # RWX, MCP
		self.elf.phdrs[phdr_num].p_align = 1
		self.elf.phdrs[phdr_num-1].p_memsz -= 0x100000

		# Add mirror at 0x05800000 for trampolines
		phdr_num = self.elf.insert_phdr(addr)
		self.elf.phdrs[phdr_num].content = elf_dat
		self.elf.phdrs[phdr_num].p_type = 1   # LOAD
		self.elf.phdrs[phdr_num].p_offset = 0 # filled in
		self.elf.phdrs[phdr_num].p_vaddr = 0x05200000
		self.elf.phdrs[phdr_num].p_paddr = addr # ramdisk is consistent so we can do this.
		self.elf.phdrs[phdr_num].p_filesz = carveout_sz#len(elf_dat)
		self.elf.phdrs[phdr_num].p_memsz = carveout_sz#len(elf_dat)
		self.elf.phdrs[phdr_num].p_flags = 7 | (0x1<<20) # RWX, MCP
		self.elf.phdrs[phdr_num].p_align = 1
		
	def encrypt(self, file, offset, no_crypto):
		key = base64.b16decode(b'00000000000000000000000000000000') # fill in if you need crypted images...
		iv = base64.b16decode(b'00000000000000000000000000000000')  # ""
		file.seek(offset)
		buffer = bytearray()
		hash = hashlib.sha1()
		while True:
			cipher = AES.new(key, AES.MODE_CBC, iv)
			dec = file.read(0x4000)
			if len(dec) < 0x10:
				break
			if no_crypto:
				enc = dec
			else:
				enc = cipher.encrypt(dec)
			hash.update(enc)
			buffer += enc
			iv = enc[-0x10:]
		file.seek(offset)
		file.write(buffer)
		return hash.digest()

input_fn = None
output_fn = None
no_crypto = False
replace_sections = {}
extract_sections = {}
bss_sections = {}
proc_addr = 0x0
proc_elf = ""

i = 1
while i < len(sys.argv):
	option = sys.argv[i]
	if option[0] == '-':
		if option[1:] == "in":
			input_fn = sys.argv[i+1]
			i += 1
		elif option[1:] == "out":
			output_fn = sys.argv[i+1]
			i += 1
		elif option[1:] == "nc":
			no_crypto = True
		elif option[1:] == "b":
			param = sys.argv[i+1]
			i += 1
			param = param.split(",")
			bss_sections[int(param[0], 0)] = param[1]
		elif option[1:] == "proc":
			param = sys.argv[i+1]
			i += 1
			param = param.split(",")
			proc_addr = int(param[0], 0)
			proc_elf = param[1]
		elif option[1:] == "r":
			param = sys.argv[i+1]
			i += 1
			param = param.split(",")
			replace_sections[int(param[0], 0)] = param[1]
		elif option[1:] == "e":
			param = sys.argv[i+1]
			i += 1
			param = param.split(",")
			extract_sections[int(param[0], 0)] = param[1]
		else:
			print("	unknown option " + option)
	i += 1

if input_fn != None:
	fw = ancast(open(input_fn, "rb"), no_crypto)
	fw.extract_sections(extract_sections)
	fw.replace_sections(replace_sections)
	fw.bss_sections(bss_sections)
	if proc_elf != "":
		fw.elf_proc(proc_addr, proc_elf)
	if output_fn != None:
		fw.write(open(output_fn, "w+b"))
	if output_fn != None:
		fw.write_elf(open(output_fn+".elf", "w+b"))
	#fw.elf._print()
