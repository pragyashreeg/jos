#ifndef JOS_INC_ELF_H
#define JOS_INC_ELF_H

#define ELF_MAGIC 0x464C457FU	/* "\x7FELF" in little endian */

struct Elf {
	uint32_t e_magic;	// must equal ELF_MAGIC
	uint8_t e_elf[12];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
};

struct Proghdr {
	uint32_t p_type;
    uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_va;
	uint64_t p_pa;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;
};

struct Secthdr {
	uint32_t sh_name;
	uint32_t sh_type;
	uint64_t sh_flags;
	uint64_t sh_addr;
	uint64_t sh_offset;
	uint64_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint64_t sh_addralign;
	uint64_t sh_entsize;
};

struct Symtab {
	uint32_t st_name;
	unsigned char st_info;
	unsigned char st_other;
	uint16_t st_shndx;
	uint64_t st_value;
	uint64_t st_size;
};

// Values for Proghdr::p_type
#define ELF_PROG_LOAD		1

// Flag bits for Proghdr::p_flags
#define ELF_PROG_FLAG_EXEC	1
#define ELF_PROG_FLAG_WRITE	2
#define ELF_PROG_FLAG_READ	4

// Values for Secthdr::sh_type
#define ELF_SHT_NULL		0
#define ELF_SHT_PROGBITS	1
#define ELF_SHT_SYMTAB		2
#define ELF_SHT_STRTAB		3
#define ELF_SHT_RELA        4
#define ELF_SHT_HASH        5
#define ELF_SHT_DYNAMIC     6
#define ELF_SHT_NOTE        7
#define ELF_SHT_NOBITS      8
#define ELF_SHT_REL         9
#define ELF_SHT_SHLIB       10
#define ELF_SHT_DYNSYM      11
#define ELF_SHT_NUM         12
#define ELF_SHT_LOPROC      0x70000000
#define ELF_SHT_HIPROC      0x7fffffff
#define ELF_SHT_LOUSER      0x80000000
#define ELF_SHT_HIUSER      0xffffffff

// Values for Secthdr::sh_name
#define ELF_SHN_UNDEF		0

#endif /* !JOS_INC_ELF_H */
