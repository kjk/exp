/*
 * WinLoader - Windows PE Memory Loader Implementation
 * Ported from jchv/go-winloader
 *
 * A library for loading Windows PE modules directly from memory
 * without requiring temporary files on disk.
 *
 * This file consolidates all implementation: PE parsing, virtual memory
 * management, and the main loader functionality.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "winloader.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
/* Stub for non-Windows platforms */
typedef void *HMODULE;
#endif

/*============================================================================
 * PE FORMAT STRUCTURES AND CONSTANTS
 *============================================================================*/

/* PE Signatures */
#define MZ_SIGNATURE 0x5A4D
#define PE_SIGNATURE 0x00004550

/* Optional Header Magic */
#define IMAGE_NT_OPTIONAL_HDR32_MAGIC 0x010b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x020b

/* Machine Types */
#define IMAGE_FILE_MACHINE_UNKNOWN   0x0000
#define IMAGE_FILE_MACHINE_AM33      0x01d3
#define IMAGE_FILE_MACHINE_AMD64     0x8664
#define IMAGE_FILE_MACHINE_ARM       0x01c0
#define IMAGE_FILE_MACHINE_ARM64     0xaa64
#define IMAGE_FILE_MACHINE_ARMNT     0x01c4
#define IMAGE_FILE_MACHINE_EBC       0x0ebc
#define IMAGE_FILE_MACHINE_I386      0x014c
#define IMAGE_FILE_MACHINE_IA64      0x0200
#define IMAGE_FILE_MACHINE_M32R      0x9041
#define IMAGE_FILE_MACHINE_MIPS16    0x0266
#define IMAGE_FILE_MACHINE_MIPSFPU   0x0366
#define IMAGE_FILE_MACHINE_MIPSFPU16 0x0466
#define IMAGE_FILE_MACHINE_POWERPC   0x01f0
#define IMAGE_FILE_MACHINE_POWERPCFP 0x01f1
#define IMAGE_FILE_MACHINE_R4000     0x0166
#define IMAGE_FILE_MACHINE_RISCV32   0x5032
#define IMAGE_FILE_MACHINE_RISCV64   0x5064
#define IMAGE_FILE_MACHINE_RISCV128  0x5128
#define IMAGE_FILE_MACHINE_SH3       0x01a2
#define IMAGE_FILE_MACHINE_SH3DSP    0x01a3
#define IMAGE_FILE_MACHINE_SH4       0x01a6
#define IMAGE_FILE_MACHINE_SH5       0x01a8
#define IMAGE_FILE_MACHINE_THUMB     0x01c2
#define IMAGE_FILE_MACHINE_WCEMIPSV2 0x0169

/* Image Characteristics */
#define IMAGE_FILE_RELOCS_STRIPPED         0x0001
#define IMAGE_FILE_EXECUTABLE_IMAGE        0x0002
#define IMAGE_FILE_LINE_NUMS_STRIPPED      0x0004
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED     0x0008
#define IMAGE_FILE_AGGRESSIVE_WS_TRIM      0x0010
#define IMAGE_FILE_LARGE_ADDRESS_AWARE     0x0020
#define IMAGE_FILE_BYTES_REVERSED_LO       0x0080
#define IMAGE_FILE_32BIT_MACHINE           0x0100
#define IMAGE_FILE_DEBUG_STRIPPED          0x0200
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP 0x0400
#define IMAGE_FILE_NET_RUN_FROM_SWAP       0x0800
#define IMAGE_FILE_SYSTEM                  0x1000
#define IMAGE_FILE_DLL                     0x2000
#define IMAGE_FILE_UP_SYSTEM_ONLY          0x4000
#define IMAGE_FILE_BYTES_REVERSED_HI       0x8000

/* Data Directory Indices */
#define IMAGE_DIRECTORY_ENTRY_EXPORT         0
#define IMAGE_DIRECTORY_ENTRY_IMPORT         1
#define IMAGE_DIRECTORY_ENTRY_RESOURCE       2
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION      3
#define IMAGE_DIRECTORY_ENTRY_SECURITY       4
#define IMAGE_DIRECTORY_ENTRY_BASERELOC      5
#define IMAGE_DIRECTORY_ENTRY_DEBUG          6
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE   7
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR      8
#define IMAGE_DIRECTORY_ENTRY_TLS            9
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11
#define IMAGE_DIRECTORY_ENTRY_IAT            12
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

/* Section Characteristics */
#define IMAGE_SCN_TYPE_NO_PAD            0x00000008
#define IMAGE_SCN_CNT_CODE               0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA   0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 0x00000080
#define IMAGE_SCN_LNK_OTHER              0x00000100
#define IMAGE_SCN_LNK_INFO               0x00000200
#define IMAGE_SCN_LNK_REMOVE             0x00000800
#define IMAGE_SCN_LNK_COMDAT             0x00001000
#define IMAGE_SCN_GPREL                  0x00008000
#define IMAGE_SCN_MEM_PURGEABLE          0x00020000
#define IMAGE_SCN_MEM_16BIT              0x00020000
#define IMAGE_SCN_MEM_LOCKED             0x00040000
#define IMAGE_SCN_MEM_PRELOAD            0x00080000
#define IMAGE_SCN_ALIGN_1BYTES           0x00100000
#define IMAGE_SCN_ALIGN_2BYTES           0x00200000
#define IMAGE_SCN_ALIGN_4BYTES           0x00300000
#define IMAGE_SCN_ALIGN_8BYTES           0x00400000
#define IMAGE_SCN_ALIGN_16BYTES          0x00500000
#define IMAGE_SCN_ALIGN_32BYTES          0x00600000
#define IMAGE_SCN_ALIGN_64BYTES          0x00700000
#define IMAGE_SCN_ALIGN_128BYTES         0x00800000
#define IMAGE_SCN_ALIGN_256BYTES         0x00900000
#define IMAGE_SCN_ALIGN_512BYTES         0x00A00000
#define IMAGE_SCN_ALIGN_1024BYTES        0x00B00000
#define IMAGE_SCN_ALIGN_2048BYTES        0x00C00000
#define IMAGE_SCN_ALIGN_4096BYTES        0x00D00000
#define IMAGE_SCN_ALIGN_8192BYTES        0x00E00000
#define IMAGE_SCN_LNK_NRELOC_OVFL        0x01000000
#define IMAGE_SCN_MEM_DISCARDABLE        0x02000000
#define IMAGE_SCN_MEM_NOT_CACHED         0x04000000
#define IMAGE_SCN_MEM_NOT_PAGED          0x08000000
#define IMAGE_SCN_MEM_SHARED             0x10000000
#define IMAGE_SCN_MEM_EXECUTE            0x20000000
#define IMAGE_SCN_MEM_READ               0x40000000
#define IMAGE_SCN_MEM_WRITE              0x80000000

/* Relocation Types - Base */
#define IMAGE_REL_BASED_ABSOLUTE       0
#define IMAGE_REL_BASED_HIGH           1
#define IMAGE_REL_BASED_LOW            2
#define IMAGE_REL_BASED_HIGHLOW        3
#define IMAGE_REL_BASED_HIGHADJ        4
#define IMAGE_REL_BASED_DIR64          10

/* Relocation Types - Architecture specific */
#define IMAGE_REL_BASED_MIPS_JMPADDR   5
#define IMAGE_REL_BASED_ARM_MOV32      5
#define IMAGE_REL_BASED_RISCV_HIGH20   5
#define IMAGE_REL_BASED_THUMB_MOV32    7
#define IMAGE_REL_BASED_RISCV_LOW12I   7
#define IMAGE_REL_BASED_RISCV_LOW12S   8
#define IMAGE_REL_BASED_MIPS_JMPADDR16 9
#define IMAGE_REL_BASED_IA64_IMM64     9

/* Maximum number of sections */
#define MAX_SECTIONS 96

/* Offset from NT header to optional header */
#define OFFSET_OF_OPTIONAL_HEADER_FROM_NT_HEADER 24

/*============================================================================
 * VIRTUAL MEMORY CONSTANTS
 *============================================================================*/

/* Memory Allocation Types */
#define MEM_COMMIT      0x00001000
#define MEM_RESERVE     0x00002000
#define MEM_RESET       0x00080000
#define MEM_RESET_UNDO  0x01000000
#define MEM_DECOMMIT    0x00004000
#define MEM_RELEASE     0x00008000

/* Page Protection Constants */
#define PAGE_NOACCESS          0x01
#define PAGE_READONLY          0x02
#define PAGE_READWRITE         0x04
#define PAGE_WRITECOPY         0x08
#define PAGE_EXECUTE           0x10
#define PAGE_EXECUTE_READ      0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_EXECUTE_WRITECOPY 0x80
#define PAGE_GUARD             0x100
#define PAGE_NOCACHE           0x200
#define PAGE_WRITECOMBINE      0x400

/*============================================================================
 * PE FORMAT STRUCTURES
 *============================================================================*/

#pragma pack(push, 1)

/* DOS Header */
typedef struct _IMAGE_DOS_HEADER {
    uint16_t e_magic;      /* Magic number (MZ) */
    uint16_t e_cblp;       /* Bytes on last page of file */
    uint16_t e_cp;         /* Pages in file */
    uint16_t e_crlc;       /* Relocations */
    uint16_t e_cparhdr;    /* Size of header in paragraphs */
    uint16_t e_minalloc;   /* Minimum extra paragraphs needed */
    uint16_t e_maxalloc;   /* Maximum extra paragraphs needed */
    uint16_t e_ss;         /* Initial (relative) SS value */
    uint16_t e_sp;         /* Initial SP value */
    uint16_t e_csum;       /* Checksum */
    uint16_t e_ip;         /* Initial IP value */
    uint16_t e_cs;         /* Initial (relative) CS value */
    uint16_t e_lfarlc;     /* File address of relocation table */
    uint16_t e_ovno;       /* Overlay number */
    uint16_t e_res[4];     /* Reserved words */
    uint16_t e_oemid;      /* OEM identifier */
    uint16_t e_oeminfo;    /* OEM information */
    uint16_t e_res2[10];   /* Reserved words */
    uint32_t e_lfanew;     /* File address of new exe header */
} IMAGE_DOS_HEADER;

/* File Header */
typedef struct _IMAGE_FILE_HEADER {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} IMAGE_FILE_HEADER;

/* Data Directory */
typedef struct _IMAGE_DATA_DIRECTORY {
    uint32_t VirtualAddress;
    uint32_t Size;
} IMAGE_DATA_DIRECTORY;

/* Optional Header (32-bit) */
typedef struct _IMAGE_OPTIONAL_HEADER32 {
    uint16_t Magic;
    uint8_t  MajorLinkerVersion;
    uint8_t  MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint32_t BaseOfData;
    uint32_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint32_t SizeOfStackReserve;
    uint32_t SizeOfStackCommit;
    uint32_t SizeOfHeapReserve;
    uint32_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32;

/* Optional Header (64-bit) */
typedef struct _IMAGE_OPTIONAL_HEADER64 {
    uint16_t Magic;
    uint8_t  MajorLinkerVersion;
    uint8_t  MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint64_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint64_t SizeOfStackReserve;
    uint64_t SizeOfStackCommit;
    uint64_t SizeOfHeapReserve;
    uint64_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64;

/* NT Headers (32-bit) */
typedef struct _IMAGE_NT_HEADERS32 {
    uint32_t Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32;

/* NT Headers (64-bit) */
typedef struct _IMAGE_NT_HEADERS64 {
    uint32_t Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64;

/* Section Header */
typedef struct _IMAGE_SECTION_HEADER {
    uint8_t  Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} IMAGE_SECTION_HEADER;

/* Import Descriptor */
typedef struct _IMAGE_IMPORT_DESCRIPTOR {
    uint32_t OriginalFirstThunk;
    uint32_t TimeDateStamp;
    uint32_t ForwarderChain;
    uint32_t Name;
    uint32_t FirstThunk;
} IMAGE_IMPORT_DESCRIPTOR;

/* Export Directory */
typedef struct _IMAGE_EXPORT_DIRECTORY {
    uint32_t Characteristics;
    uint32_t TimeDateStamp;
    uint16_t MajorVersion;
    uint16_t MinorVersion;
    uint32_t Name;
    uint32_t Base;
    uint32_t NumberOfFunctions;
    uint32_t NumberOfNames;
    uint32_t AddressOfFunctions;
    uint32_t AddressOfNames;
    uint32_t AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY;

/* Base Relocation Block */
typedef struct _IMAGE_BASE_RELOCATION {
    uint32_t VirtualAddress;
    uint32_t SizeOfBlock;
} IMAGE_BASE_RELOCATION;

/* TLS Directory (32-bit) */
typedef struct _IMAGE_TLS_DIRECTORY32 {
    uint32_t StartAddressOfRawData;
    uint32_t EndAddressOfRawData;
    uint32_t AddressOfIndex;
    uint32_t AddressOfCallBacks;
    uint32_t SizeOfZeroFill;
    uint32_t Characteristics;
} IMAGE_TLS_DIRECTORY32;

/* TLS Directory (64-bit) */
typedef struct _IMAGE_TLS_DIRECTORY64 {
    uint64_t StartAddressOfRawData;
    uint64_t EndAddressOfRawData;
    uint64_t AddressOfIndex;
    uint64_t AddressOfCallBacks;
    uint32_t SizeOfZeroFill;
    uint32_t Characteristics;
} IMAGE_TLS_DIRECTORY64;

#pragma pack(pop)

/*============================================================================
 * INTERNAL STRUCTURES
 *============================================================================*/

/* Memory block structure */
typedef struct _VMEM_BLOCK {
    uint8_t *data;
    size_t size;
    size_t position;  /* Current read/write position */
} VMEM_BLOCK;

/* Parsed PE Module structure */
typedef struct _PE_MODULE {
    bool is_pe64;
    IMAGE_DOS_HEADER dos_header;
    /* We store the 64-bit version and convert 32-bit to it */
    IMAGE_NT_HEADERS64 nt_headers;
    IMAGE_SECTION_HEADER sections[MAX_SECTIONS];
    uint16_t num_sections;
} PE_MODULE;

/* Base relocation entry */
typedef struct _BASE_RELOC {
    uint32_t offset;
    uint16_t type;
} BASE_RELOC;

/* Export table */
typedef struct _EXPORT_TABLE {
    char **symbols;
    uint64_t *symbol_addrs;
    uint32_t num_symbols;
    uint64_t *ordinal_addrs;
    uint32_t num_ordinals;
    uint32_t ordinal_base;
} EXPORT_TABLE;

/* PE Error codes */
typedef enum _PE_ERROR {
    PE_SUCCESS = 0,
    PE_ERROR_INVALID_MZ_SIGNATURE,
    PE_ERROR_INVALID_PE_SIGNATURE,
    PE_ERROR_UNKNOWN_OPTIONAL_HEADER_MAGIC,
    PE_ERROR_TOO_MANY_SECTIONS,
    PE_ERROR_READ_FAILED,
    PE_ERROR_MEMORY_ALLOC,
    PE_ERROR_INVALID_RELOC_TYPE,
    PE_ERROR_SYMBOL_NOT_FOUND,
    PE_ERROR_IMPORT_FAILED
} PE_ERROR;

/* Module types */
typedef enum _MODULE_TYPE {
    MODULE_TYPE_MEMORY,   /* Loaded from memory */
    MODULE_TYPE_NATIVE    /* Loaded from file (native) */
} MODULE_TYPE;

/* Loaded module structure */
struct _LOADED_MODULE {
    MODULE_TYPE type;

    /* For memory-loaded modules */
    PE_MODULE pe_module;
    VMEM_BLOCK *memory;
    EXPORT_TABLE exports;
    uint64_t base_addr;
    bool initialized;

    /* For native-loaded modules */
#ifdef _WIN32
    HMODULE native_handle;
#else
    void *native_handle;
#endif
};

/* Module cache entry */
typedef struct _CACHE_ENTRY {
    char *name;
    LOADED_MODULE *module;
    struct _CACHE_ENTRY *next;
} CACHE_ENTRY;

/*============================================================================
 * GLOBAL STATE
 *============================================================================*/

static CACHE_ENTRY *s_cache_head = NULL;
static char s_error_message[256] = {0};

/*============================================================================
 * FORWARD DECLARATIONS
 *============================================================================*/

/* Virtual memory functions */
static VMEM_BLOCK *vmem_alloc(uint64_t addr, size_t size, uint32_t alloc_type, uint32_t protect);
static VMEM_BLOCK *vmem_get(uint64_t addr, size_t size);
static void vmem_free(VMEM_BLOCK *block);
static uint64_t vmem_addr(const VMEM_BLOCK *block);
static int64_t vmem_write_at(VMEM_BLOCK *block, const uint8_t *buffer, size_t size, size_t offset);
static void vmem_clear(VMEM_BLOCK *block);
static bool vmem_protect(VMEM_BLOCK *block, size_t offset, size_t size, uint32_t protect);
static void vmem_flush_instruction_cache(void *addr, size_t size);
static uint64_t vmem_round_up(uint64_t addr, uint64_t alignment);

/* PE parsing functions */
static PE_ERROR pe_parse_module(const uint8_t *data, size_t size, PE_MODULE *module);
static PE_ERROR pe_load_base_relocs(const PE_MODULE *module, const uint8_t *mem,
                                    BASE_RELOC **relocs, uint32_t *num_relocs);
static PE_ERROR pe_relocate(const PE_MODULE *module, uint8_t *mem,
                            const BASE_RELOC *relocs, uint32_t num_relocs,
                            int64_t delta);
static PE_ERROR pe_load_exports(const PE_MODULE *module, const uint8_t *mem,
                                uint64_t base, EXPORT_TABLE *exports);
static void pe_free_exports(EXPORT_TABLE *exports);
static uint64_t pe_get_proc(const EXPORT_TABLE *exports, const char *name);
static uint64_t pe_get_ordinal(const EXPORT_TABLE *exports, uint16_t ordinal);
static void pe_optional_header_32_to_64(const IMAGE_OPTIONAL_HEADER32 *h32,
                                        IMAGE_OPTIONAL_HEADER64 *h64);

/* Loader internal functions */
static WINLOADER_ERROR memloader_load(const uint8_t *data, size_t size, LOADED_MODULE **module);
static WINLOADER_ERROR resolve_imports(LOADED_MODULE *module);
static WINLOADER_ERROR execute_tls_callbacks(LOADED_MODULE *module, uint32_t reason);
static WINLOADER_ERROR call_entry_point(LOADED_MODULE *module, uint32_t reason);
static LOADED_MODULE *cache_find(const char *name);

/*============================================================================
 * HELPER FUNCTIONS
 *============================================================================*/

/* Set error message */
static void set_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(s_error_message, sizeof(s_error_message), fmt, args);
    va_end(args);
}

/* Lowercase string comparison */
static int stricmp_portable(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = (*a >= 'A' && *a <= 'Z') ? (*a + 32) : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? (*b + 32) : *b;
        if (ca != cb) return ca - cb;
        a++;
        b++;
    }
    return *a - *b;
}

/* Get protection flags from section characteristics */
static uint32_t get_section_protection(uint32_t characteristics)
{
    bool exec = (characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
    bool read = (characteristics & IMAGE_SCN_MEM_READ) != 0;
    bool write = (characteristics & IMAGE_SCN_MEM_WRITE) != 0;

    if (exec && write && read) return PAGE_EXECUTE_READWRITE;
    if (exec && read) return PAGE_EXECUTE_READ;
    if (exec) return PAGE_EXECUTE;
    if (write && read) return PAGE_READWRITE;
    if (read) return PAGE_READONLY;
    return PAGE_NOACCESS;
}

/* Read bytes from buffer at offset */
static bool read_bytes(const uint8_t *data, size_t data_size,
                       size_t offset, void *dest, size_t count)
{
    if (offset + count > data_size) {
        return false;
    }
    memcpy(dest, data + offset, count);
    return true;
}

/* Read null-terminated string from memory */
static char *read_string(const uint8_t *mem, size_t offset, size_t max_len)
{
    size_t len = 0;
    while (len < max_len && mem[offset + len] != 0) {
        len++;
    }

    char *str = (char *)malloc(len + 1);
    if (!str) return NULL;

    memcpy(str, mem + offset, len);
    str[len] = 0;
    return str;
}

/*============================================================================
 * VIRTUAL MEMORY IMPLEMENTATION
 *============================================================================*/

/* Allocate virtual memory */
static VMEM_BLOCK *vmem_alloc(uint64_t addr, size_t size, uint32_t alloc_type, uint32_t protect)
{
#ifdef _WIN32
    void *ptr = VirtualAlloc((void *)(uintptr_t)addr, size, alloc_type, protect);
    if (!ptr) {
        return NULL;
    }
    return vmem_get((uint64_t)(uintptr_t)ptr, size);
#else
    /* On non-Windows, map allocation types to mmap flags */
    (void)alloc_type;  /* Not used on non-Windows */
    int mmap_prot = 0;
    if (protect & PAGE_EXECUTE_READWRITE) {
        mmap_prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    } else if (protect & PAGE_EXECUTE_READ) {
        mmap_prot = PROT_READ | PROT_EXEC;
    } else if (protect & PAGE_EXECUTE) {
        mmap_prot = PROT_EXEC;
    } else if (protect & PAGE_READWRITE) {
        mmap_prot = PROT_READ | PROT_WRITE;
    } else if (protect & PAGE_READONLY) {
        mmap_prot = PROT_READ;
    }

    int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;
    if (addr != 0) {
        mmap_flags |= MAP_FIXED;
    }

    void *ptr = mmap((void *)(uintptr_t)addr, size, mmap_prot, mmap_flags, -1, 0);
    if (ptr == MAP_FAILED) {
        return NULL;
    }
    return vmem_get((uint64_t)(uintptr_t)ptr, size);
#endif
}

/* Get a memory block wrapper for existing memory */
static VMEM_BLOCK *vmem_get(uint64_t addr, size_t size)
{
    VMEM_BLOCK *block = (VMEM_BLOCK *)malloc(sizeof(VMEM_BLOCK));
    if (!block) {
        return NULL;
    }

    block->data = (uint8_t *)(uintptr_t)addr;
    block->size = size;
    block->position = 0;

    return block;
}

/* Free virtual memory */
static void vmem_free(VMEM_BLOCK *block)
{
    if (!block) return;

#ifdef _WIN32
    VirtualFree(block->data, 0, MEM_RELEASE);
#else
    munmap(block->data, block->size);
#endif

    free(block);
}

/* Get base address of memory block */
static uint64_t vmem_addr(const VMEM_BLOCK *block)
{
    if (!block) return 0;
    return (uint64_t)(uintptr_t)block->data;
}

/* Write to memory block at specific offset */
static int64_t vmem_write_at(VMEM_BLOCK *block, const uint8_t *buffer, size_t size, size_t offset)
{
    if (!block || !buffer) return -1;

    if (offset >= block->size) {
        return -1;  /* No space */
    }

    size_t available = block->size - offset;
    size_t to_write = (size < available) ? size : available;

    memcpy(block->data + offset, buffer, to_write);

    /* Flush instruction cache */
    vmem_flush_instruction_cache(block->data + offset, to_write);

    return (int64_t)to_write;
}

/* Clear memory block contents */
static void vmem_clear(VMEM_BLOCK *block)
{
    if (!block) return;

    memset(block->data, 0, block->size);
    vmem_flush_instruction_cache(block->data, block->size);
}

/* Change memory protection */
static bool vmem_protect(VMEM_BLOCK *block, size_t offset, size_t size, uint32_t protect)
{
    if (!block) return false;

#ifdef _WIN32
    DWORD old_protect;
    return VirtualProtect(block->data + offset, size, protect, &old_protect) != 0;
#else
    /* Convert Windows protection flags to mmap protection */
    int mmap_prot = 0;
    if (protect & PAGE_EXECUTE_READWRITE) {
        mmap_prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    } else if (protect & PAGE_EXECUTE_READ) {
        mmap_prot = PROT_READ | PROT_EXEC;
    } else if (protect & PAGE_EXECUTE) {
        mmap_prot = PROT_EXEC;
    } else if (protect & PAGE_READWRITE) {
        mmap_prot = PROT_READ | PROT_WRITE;
    } else if (protect & PAGE_READONLY) {
        mmap_prot = PROT_READ;
    } else if (protect & PAGE_NOACCESS) {
        mmap_prot = PROT_NONE;
    }

    return mprotect(block->data + offset, size, mmap_prot) == 0;
#endif
}

/* Flush instruction cache */
static void vmem_flush_instruction_cache(void *addr, size_t size)
{
#ifdef _WIN32
    FlushInstructionCache(GetCurrentProcess(), addr, size);
#else
    /* On some architectures this may be needed */
    /* GCC builtin for cache sync */
#ifdef __GNUC__
    __builtin___clear_cache((char *)addr, (char *)addr + size);
#endif
#endif
}

/* Round address up to alignment boundary */
static uint64_t vmem_round_up(uint64_t addr, uint64_t alignment)
{
    return (addr + alignment - 1) & ~(alignment - 1);
}

/*============================================================================
 * PE PARSING IMPLEMENTATION
 *============================================================================*/

/* Convert 32-bit optional header to 64-bit */
static void pe_optional_header_32_to_64(const IMAGE_OPTIONAL_HEADER32 *h32,
                                        IMAGE_OPTIONAL_HEADER64 *h64)
{
    h64->Magic = h32->Magic;
    h64->MajorLinkerVersion = h32->MajorLinkerVersion;
    h64->MinorLinkerVersion = h32->MinorLinkerVersion;
    h64->SizeOfCode = h32->SizeOfCode;
    h64->SizeOfInitializedData = h32->SizeOfInitializedData;
    h64->SizeOfUninitializedData = h32->SizeOfUninitializedData;
    h64->AddressOfEntryPoint = h32->AddressOfEntryPoint;
    h64->BaseOfCode = h32->BaseOfCode;
    h64->ImageBase = h32->ImageBase;
    h64->SectionAlignment = h32->SectionAlignment;
    h64->FileAlignment = h32->FileAlignment;
    h64->MajorOperatingSystemVersion = h32->MajorOperatingSystemVersion;
    h64->MinorOperatingSystemVersion = h32->MinorOperatingSystemVersion;
    h64->MajorImageVersion = h32->MajorImageVersion;
    h64->MinorImageVersion = h32->MinorImageVersion;
    h64->MajorSubsystemVersion = h32->MajorSubsystemVersion;
    h64->MinorSubsystemVersion = h32->MinorSubsystemVersion;
    h64->Win32VersionValue = h32->Win32VersionValue;
    h64->SizeOfImage = h32->SizeOfImage;
    h64->SizeOfHeaders = h32->SizeOfHeaders;
    h64->CheckSum = h32->CheckSum;
    h64->Subsystem = h32->Subsystem;
    h64->DllCharacteristics = h32->DllCharacteristics;
    h64->SizeOfStackReserve = h32->SizeOfStackReserve;
    h64->SizeOfStackCommit = h32->SizeOfStackCommit;
    h64->SizeOfHeapReserve = h32->SizeOfHeapReserve;
    h64->SizeOfHeapCommit = h32->SizeOfHeapCommit;
    h64->LoaderFlags = h32->LoaderFlags;
    h64->NumberOfRvaAndSizes = h32->NumberOfRvaAndSizes;

    for (int i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++) {
        h64->DataDirectory[i] = h32->DataDirectory[i];
    }
}

/* Parse a PE module from memory buffer */
static PE_ERROR pe_parse_module(const uint8_t *data, size_t size, PE_MODULE *module)
{
    if (!data || !module) {
        return PE_ERROR_READ_FAILED;
    }

    memset(module, 0, sizeof(PE_MODULE));

    /* Read DOS header */
    if (!read_bytes(data, size, 0, &module->dos_header, sizeof(IMAGE_DOS_HEADER))) {
        return PE_ERROR_READ_FAILED;
    }

    if (module->dos_header.e_magic != MZ_SIGNATURE) {
        return PE_ERROR_INVALID_MZ_SIGNATURE;
    }

    /* Read PE signature */
    uint32_t pe_sig;
    size_t pe_offset = module->dos_header.e_lfanew;
    if (!read_bytes(data, size, pe_offset, &pe_sig, sizeof(pe_sig))) {
        return PE_ERROR_READ_FAILED;
    }

    if (pe_sig != PE_SIGNATURE) {
        return PE_ERROR_INVALID_PE_SIGNATURE;
    }

    /* Read optional header magic to determine PE32 vs PE32+ */
    uint16_t opt_magic;
    size_t opt_magic_offset = pe_offset + OFFSET_OF_OPTIONAL_HEADER_FROM_NT_HEADER;
    if (!read_bytes(data, size, opt_magic_offset, &opt_magic, sizeof(opt_magic))) {
        return PE_ERROR_READ_FAILED;
    }

    /* Read NT headers based on architecture */
    if (opt_magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        module->is_pe64 = false;

        IMAGE_NT_HEADERS32 nt32;
        if (!read_bytes(data, size, pe_offset, &nt32, sizeof(nt32))) {
            return PE_ERROR_READ_FAILED;
        }

        /* Convert to 64-bit format for uniform handling */
        module->nt_headers.Signature = nt32.Signature;
        module->nt_headers.FileHeader = nt32.FileHeader;
        pe_optional_header_32_to_64(&nt32.OptionalHeader, &module->nt_headers.OptionalHeader);

    } else if (opt_magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        module->is_pe64 = true;

        if (!read_bytes(data, size, pe_offset, &module->nt_headers, sizeof(IMAGE_NT_HEADERS64))) {
            return PE_ERROR_READ_FAILED;
        }

    } else {
        return PE_ERROR_UNKNOWN_OPTIONAL_HEADER_MAGIC;
    }

    /* Read section headers */
    uint16_t num_sections = module->nt_headers.FileHeader.NumberOfSections;
    if (num_sections > MAX_SECTIONS) {
        return PE_ERROR_TOO_MANY_SECTIONS;
    }
    module->num_sections = num_sections;

    size_t sections_offset = pe_offset + OFFSET_OF_OPTIONAL_HEADER_FROM_NT_HEADER +
                            module->nt_headers.FileHeader.SizeOfOptionalHeader;

    for (uint16_t i = 0; i < num_sections; i++) {
        if (!read_bytes(data, size, sections_offset + i * sizeof(IMAGE_SECTION_HEADER),
                       &module->sections[i], sizeof(IMAGE_SECTION_HEADER))) {
            return PE_ERROR_READ_FAILED;
        }
    }

    return PE_SUCCESS;
}

/* Load base relocations from a PE module */
static PE_ERROR pe_load_base_relocs(const PE_MODULE *module, const uint8_t *mem,
                                    BASE_RELOC **relocs, uint32_t *num_relocs)
{
    if (!module || !mem || !relocs || !num_relocs) {
        return PE_ERROR_READ_FAILED;
    }

    *relocs = NULL;
    *num_relocs = 0;

    IMAGE_DATA_DIRECTORY dir = module->nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (dir.Size == 0) {
        return PE_SUCCESS;  /* No relocations */
    }

    /* First pass: count relocations */
    uint32_t count = 0;
    size_t offset = dir.VirtualAddress;
    size_t end = dir.VirtualAddress + dir.Size;

    while (offset < end) {
        IMAGE_BASE_RELOCATION block;
        memcpy(&block, mem + offset, sizeof(block));

        if (block.SizeOfBlock == 0) break;

        uint32_t num_entries = (block.SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(uint16_t);
        count += num_entries;
        offset += block.SizeOfBlock;
    }

    if (count == 0) {
        return PE_SUCCESS;
    }

    /* Allocate relocation array */
    *relocs = (BASE_RELOC *)malloc(count * sizeof(BASE_RELOC));
    if (!*relocs) {
        return PE_ERROR_MEMORY_ALLOC;
    }

    /* Second pass: read relocations */
    offset = dir.VirtualAddress;
    uint32_t idx = 0;

    while (offset < end && idx < count) {
        IMAGE_BASE_RELOCATION block;
        memcpy(&block, mem + offset, sizeof(block));

        if (block.SizeOfBlock == 0) break;

        uint32_t num_entries = (block.SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(uint16_t);
        const uint16_t *entries = (const uint16_t *)(mem + offset + sizeof(IMAGE_BASE_RELOCATION));

        for (uint32_t i = 0; i < num_entries && idx < count; i++) {
            uint16_t entry = entries[i];
            (*relocs)[idx].type = entry >> 12;
            (*relocs)[idx].offset = block.VirtualAddress + (entry & 0x0FFF);
            idx++;
        }

        offset += block.SizeOfBlock;
    }

    *num_relocs = idx;
    return PE_SUCCESS;
}

/* Apply relocations to a loaded module */
static PE_ERROR pe_relocate(const PE_MODULE *module, uint8_t *mem,
                            const BASE_RELOC *relocs, uint32_t num_relocs,
                            int64_t delta)
{
    if (!module || !mem) {
        return PE_ERROR_READ_FAILED;
    }

    if (delta == 0 || num_relocs == 0 || !relocs) {
        return PE_SUCCESS;  /* No relocation needed */
    }

    uint16_t machine = module->nt_headers.FileHeader.Machine;

    for (uint32_t i = 0; i < num_relocs; i++) {
        uint32_t offset = relocs[i].offset;
        uint16_t type = relocs[i].type;

        switch (type) {
        case IMAGE_REL_BASED_ABSOLUTE:
            /* No relocation needed */
            break;

        case IMAGE_REL_BASED_HIGH:
            {
                uint16_t *ptr = (uint16_t *)(mem + offset);
                *ptr = (uint16_t)((*ptr) + (uint16_t)((delta >> 16) & 0xFFFF));
            }
            break;

        case IMAGE_REL_BASED_LOW:
            {
                uint16_t *ptr = (uint16_t *)(mem + offset);
                *ptr = (uint16_t)((*ptr) + (uint16_t)(delta & 0xFFFF));
            }
            break;

        case IMAGE_REL_BASED_HIGHLOW:
            {
                uint32_t *ptr = (uint32_t *)(mem + offset);
                *ptr = (uint32_t)((*ptr) + (int32_t)delta);
            }
            break;

        case IMAGE_REL_BASED_DIR64:
            {
                uint64_t *ptr = (uint64_t *)(mem + offset);
                *ptr = (uint64_t)((*ptr) + delta);
            }
            break;

        case IMAGE_REL_BASED_MIPS_JMPADDR:
            /* IMAGE_REL_BASED_ARM_MOV32 and IMAGE_REL_BASED_RISCV_HIGH20 share this value */
            if (machine == IMAGE_FILE_MACHINE_MIPSFPU ||
                machine == IMAGE_FILE_MACHINE_MIPS16 ||
                machine == IMAGE_FILE_MACHINE_MIPSFPU16 ||
                machine == IMAGE_FILE_MACHINE_R4000 ||
                machine == IMAGE_FILE_MACHINE_WCEMIPSV2) {
                /* MIPS jump address */
                uint32_t *ptr = (uint32_t *)(mem + offset);
                uint32_t val = *ptr;
                uint32_t target = ((val & 0x03FFFFFF) << 2) | (val & 0xFC000000);
                target = (uint32_t)(target + delta);
                *ptr = (val & 0xFC000000) | ((target >> 2) & 0x03FFFFFF);
            } else if (machine == IMAGE_FILE_MACHINE_ARM ||
                       machine == IMAGE_FILE_MACHINE_ARMNT ||
                       machine == IMAGE_FILE_MACHINE_THUMB) {
                /* ARM MOV32 - complex, not fully implemented */
                return PE_ERROR_INVALID_RELOC_TYPE;
            } else if (machine == IMAGE_FILE_MACHINE_RISCV32 ||
                       machine == IMAGE_FILE_MACHINE_RISCV64 ||
                       machine == IMAGE_FILE_MACHINE_RISCV128) {
                /* RISC-V HIGH20 - not implemented */
                return PE_ERROR_INVALID_RELOC_TYPE;
            } else {
                return PE_ERROR_INVALID_RELOC_TYPE;
            }
            break;

        case IMAGE_REL_BASED_THUMB_MOV32:
            /* IMAGE_REL_BASED_RISCV_LOW12I shares this value */
            if (machine == IMAGE_FILE_MACHINE_ARM ||
                machine == IMAGE_FILE_MACHINE_ARMNT ||
                machine == IMAGE_FILE_MACHINE_THUMB) {
                /* THUMB MOV32 - not implemented */
                return PE_ERROR_INVALID_RELOC_TYPE;
            } else if (machine == IMAGE_FILE_MACHINE_RISCV32 ||
                       machine == IMAGE_FILE_MACHINE_RISCV64 ||
                       machine == IMAGE_FILE_MACHINE_RISCV128) {
                /* RISC-V LOW12I - not implemented */
                return PE_ERROR_INVALID_RELOC_TYPE;
            }
            break;

        case IMAGE_REL_BASED_RISCV_LOW12S:
            /* RISC-V LOW12S - not implemented */
            if (machine == IMAGE_FILE_MACHINE_RISCV32 ||
                machine == IMAGE_FILE_MACHINE_RISCV64 ||
                machine == IMAGE_FILE_MACHINE_RISCV128) {
                return PE_ERROR_INVALID_RELOC_TYPE;
            }
            break;

        case IMAGE_REL_BASED_MIPS_JMPADDR16:
            /* IMAGE_REL_BASED_IA64_IMM64 shares this value */
            if (machine == IMAGE_FILE_MACHINE_MIPSFPU ||
                machine == IMAGE_FILE_MACHINE_MIPS16 ||
                machine == IMAGE_FILE_MACHINE_MIPSFPU16) {
                /* MIPS JMPADDR16 - not implemented */
                return PE_ERROR_INVALID_RELOC_TYPE;
            } else if (machine == IMAGE_FILE_MACHINE_IA64) {
                /* IA64 IMM64 - not implemented */
                return PE_ERROR_INVALID_RELOC_TYPE;
            }
            break;

        default:
            return PE_ERROR_INVALID_RELOC_TYPE;
        }
    }

    return PE_SUCCESS;
}

/* Load export table from a PE module */
static PE_ERROR pe_load_exports(const PE_MODULE *module, const uint8_t *mem,
                                uint64_t base, EXPORT_TABLE *exports)
{
    if (!module || !mem || !exports) {
        return PE_ERROR_READ_FAILED;
    }

    memset(exports, 0, sizeof(EXPORT_TABLE));

    IMAGE_DATA_DIRECTORY dir = module->nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (dir.Size == 0) {
        return PE_SUCCESS;  /* No exports */
    }

    /* Read export directory */
    IMAGE_EXPORT_DIRECTORY export_dir;
    memcpy(&export_dir, mem + dir.VirtualAddress, sizeof(export_dir));

    exports->ordinal_base = export_dir.Base;
    exports->num_ordinals = export_dir.NumberOfFunctions;
    exports->num_symbols = export_dir.NumberOfNames;

    /* Load function addresses (ordinals) */
    if (exports->num_ordinals > 0) {
        exports->ordinal_addrs = (uint64_t *)calloc(exports->num_ordinals, sizeof(uint64_t));
        if (!exports->ordinal_addrs) {
            return PE_ERROR_MEMORY_ALLOC;
        }

        const uint32_t *func_addrs = (const uint32_t *)(mem + export_dir.AddressOfFunctions);
        for (uint32_t i = 0; i < exports->num_ordinals; i++) {
            exports->ordinal_addrs[i] = base + func_addrs[i];
        }
    }

    /* Load symbol names and their addresses */
    if (exports->num_symbols > 0) {
        exports->symbols = (char **)calloc(exports->num_symbols, sizeof(char *));
        exports->symbol_addrs = (uint64_t *)calloc(exports->num_symbols, sizeof(uint64_t));

        if (!exports->symbols || !exports->symbol_addrs) {
            pe_free_exports(exports);
            return PE_ERROR_MEMORY_ALLOC;
        }

        const uint16_t *name_ordinals = (const uint16_t *)(mem + export_dir.AddressOfNameOrdinals);
        const uint32_t *name_addrs = (const uint32_t *)(mem + export_dir.AddressOfNames);
        const uint32_t *func_addrs = (const uint32_t *)(mem + export_dir.AddressOfFunctions);

        for (uint32_t i = 0; i < exports->num_symbols; i++) {
            exports->symbols[i] = read_string(mem, name_addrs[i], 256);
            if (!exports->symbols[i]) {
                pe_free_exports(exports);
                return PE_ERROR_MEMORY_ALLOC;
            }
            exports->symbol_addrs[i] = base + func_addrs[name_ordinals[i]];
        }
    }

    return PE_SUCCESS;
}

/* Free export table resources */
static void pe_free_exports(EXPORT_TABLE *exports)
{
    if (!exports) return;

    if (exports->symbols) {
        for (uint32_t i = 0; i < exports->num_symbols; i++) {
            free(exports->symbols[i]);
        }
        free(exports->symbols);
    }

    free(exports->symbol_addrs);
    free(exports->ordinal_addrs);

    memset(exports, 0, sizeof(EXPORT_TABLE));
}

/* Find procedure by name in export table */
static uint64_t pe_get_proc(const EXPORT_TABLE *exports, const char *name)
{
    if (!exports || !name) return 0;

    for (uint32_t i = 0; i < exports->num_symbols; i++) {
        if (exports->symbols[i] && strcmp(exports->symbols[i], name) == 0) {
            return exports->symbol_addrs[i];
        }
    }

    return 0;
}

/* Find procedure by ordinal in export table */
static uint64_t pe_get_ordinal(const EXPORT_TABLE *exports, uint16_t ordinal)
{
    if (!exports) return 0;

    uint32_t index = ordinal - exports->ordinal_base;
    if (index >= exports->num_ordinals) {
        return 0;
    }

    return exports->ordinal_addrs[index];
}

/*============================================================================
 * WINLOADER IMPLEMENTATION
 *============================================================================*/

/* Get native machine architecture */
uint16_t winloader_get_native_architecture(void)
{
#if defined(_M_AMD64) || defined(__x86_64__)
    return IMAGE_FILE_MACHINE_AMD64;
#elif defined(_M_IX86) || defined(__i386__)
    return IMAGE_FILE_MACHINE_I386;
#elif defined(_M_ARM64) || defined(__aarch64__)
    return IMAGE_FILE_MACHINE_ARM64;
#elif defined(_M_ARM) || defined(__arm__)
    return IMAGE_FILE_MACHINE_ARMNT;
#else
    return IMAGE_FILE_MACHINE_UNKNOWN;
#endif
}

/* Check if architecture is supported */
bool winloader_is_architecture_supported(uint16_t machine)
{
    return machine == winloader_get_native_architecture();
}

/* Load a PE module from memory */
WINLOADER_ERROR winloader_load_from_memory(const uint8_t *data, size_t size,
                                           LOADED_MODULE **module)
{
    if (!data || !module) {
        set_error("Null pointer argument");
        return WINLOADER_ERROR_NULL_POINTER;
    }

    return memloader_load(data, size, module);
}

/* Load a PE module from file */
WINLOADER_ERROR winloader_load_from_file(const char *filename,
                                         LOADED_MODULE **module)
{
    if (!filename || !module) {
        set_error("Null pointer argument");
        return WINLOADER_ERROR_NULL_POINTER;
    }

#ifdef _WIN32
    HMODULE handle = LoadLibraryA(filename);
    if (!handle) {
        set_error("Failed to load library: %s (error %lu)", filename, GetLastError());
        return WINLOADER_ERROR_LOAD_FAILED;
    }

    LOADED_MODULE *m = (LOADED_MODULE *)calloc(1, sizeof(LOADED_MODULE));
    if (!m) {
        FreeLibrary(handle);
        set_error("Memory allocation failed");
        return WINLOADER_ERROR_MEMORY_ALLOC;
    }

    m->type = MODULE_TYPE_NATIVE;
    m->native_handle = handle;
    *module = m;

    return WINLOADER_SUCCESS;
#else
    set_error("Native loading not supported on this platform");
    return WINLOADER_ERROR_LOAD_FAILED;
#endif
}

/* Memory loader implementation */
static WINLOADER_ERROR memloader_load(const uint8_t *data, size_t size, LOADED_MODULE **module)
{
    WINLOADER_ERROR err;
    LOADED_MODULE *m = NULL;

    /* Allocate module structure */
    m = (LOADED_MODULE *)calloc(1, sizeof(LOADED_MODULE));
    if (!m) {
        set_error("Memory allocation failed for module structure");
        return WINLOADER_ERROR_MEMORY_ALLOC;
    }
    m->type = MODULE_TYPE_MEMORY;

    /* Parse PE headers */
    PE_ERROR pe_err = pe_parse_module(data, size, &m->pe_module);
    if (pe_err != PE_SUCCESS) {
        set_error("Failed to parse PE module (error %d)", pe_err);
        free(m);
        return WINLOADER_ERROR_INVALID_PE;
    }

    /* Check architecture */
    uint16_t machine = m->pe_module.nt_headers.FileHeader.Machine;
    if (!winloader_is_architecture_supported(machine)) {
        set_error("Architecture 0x%04x not supported", machine);
        free(m);
        return WINLOADER_ERROR_ARCHITECTURE_MISMATCH;
    }

    /* Get image parameters */
    uint64_t preferred_base = m->pe_module.nt_headers.OptionalHeader.ImageBase;
    uint32_t image_size = m->pe_module.nt_headers.OptionalHeader.SizeOfImage;
    uint32_t section_align = m->pe_module.nt_headers.OptionalHeader.SectionAlignment;
    uint32_t headers_size = m->pe_module.nt_headers.OptionalHeader.SizeOfHeaders;

    /* Align image size */
    image_size = (uint32_t)vmem_round_up(image_size, section_align);

    /* Allocate memory for the module
     * Try to allocate at preferred base, fall back to any address
     * Avoid allocating across 4GB boundary for compatibility */
    VMEM_BLOCK *mem = NULL;

    /* Try preferred base first */
    mem = vmem_alloc(preferred_base, image_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    /* If that fails, try any address */
    if (!mem) {
        mem = vmem_alloc(0, image_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    }

    if (!mem) {
        set_error("Failed to allocate %u bytes for module", image_size);
        free(m);
        return WINLOADER_ERROR_MEMORY_ALLOC;
    }

    m->memory = mem;
    m->base_addr = vmem_addr(mem);

    /* Clear memory */
    vmem_clear(mem);

    /* Copy headers */
    vmem_write_at(mem, data, headers_size, 0);

    /* Copy sections */
    for (uint16_t i = 0; i < m->pe_module.num_sections; i++) {
        IMAGE_SECTION_HEADER *section = &m->pe_module.sections[i];

        if (section->SizeOfRawData == 0) {
            continue;
        }

        uint32_t dest_offset = section->VirtualAddress;
        uint32_t src_offset = section->PointerToRawData;
        uint32_t copy_size = section->SizeOfRawData;

        /* Bounds check */
        if (src_offset + copy_size > size) {
            copy_size = (uint32_t)(size - src_offset);
        }

        vmem_write_at(mem, data + src_offset, copy_size, dest_offset);
    }

    /* Perform base relocation if needed */
    int64_t delta = (int64_t)m->base_addr - (int64_t)preferred_base;
    if (delta != 0) {
        BASE_RELOC *relocs = NULL;
        uint32_t num_relocs = 0;

        pe_err = pe_load_base_relocs(&m->pe_module, mem->data, &relocs, &num_relocs);
        if (pe_err != PE_SUCCESS) {
            set_error("Failed to load relocations");
            vmem_free(mem);
            free(m);
            return WINLOADER_ERROR_RELOCATION_FAILED;
        }

        pe_err = pe_relocate(&m->pe_module, mem->data, relocs, num_relocs, delta);
        free(relocs);

        if (pe_err != PE_SUCCESS) {
            set_error("Failed to apply relocations");
            vmem_free(mem);
            free(m);
            return WINLOADER_ERROR_RELOCATION_FAILED;
        }
    }

    /* Resolve imports */
    err = resolve_imports(m);
    if (err != WINLOADER_SUCCESS) {
        vmem_free(mem);
        free(m);
        return err;
    }

    /* Set section protections */
    for (uint16_t i = 0; i < m->pe_module.num_sections; i++) {
        IMAGE_SECTION_HEADER *section = &m->pe_module.sections[i];
        uint32_t protect = get_section_protection(section->Characteristics);
        uint32_t section_size = (section->VirtualSize > 0) ?
                                section->VirtualSize : section->SizeOfRawData;
        section_size = (uint32_t)vmem_round_up(section_size, section_align);

        if (!vmem_protect(mem, section->VirtualAddress, section_size, protect)) {
            set_error("Failed to set protection for section %.8s", section->Name);
            vmem_free(mem);
            free(m);
            return WINLOADER_ERROR_PROTECTION_FAILED;
        }
    }

    /* Load export table */
    pe_err = pe_load_exports(&m->pe_module, mem->data, m->base_addr, &m->exports);
    if (pe_err != PE_SUCCESS) {
        set_error("Failed to load exports");
        vmem_free(mem);
        free(m);
        return WINLOADER_ERROR_INVALID_PE;
    }

    /* Execute TLS callbacks */
    err = execute_tls_callbacks(m, DLL_PROCESS_ATTACH);
    if (err != WINLOADER_SUCCESS) {
        vmem_free(mem);
        pe_free_exports(&m->exports);
        free(m);
        return err;
    }

    /* Call DLL entry point */
    err = call_entry_point(m, DLL_PROCESS_ATTACH);
    if (err != WINLOADER_SUCCESS) {
        execute_tls_callbacks(m, DLL_PROCESS_DETACH);
        vmem_free(mem);
        pe_free_exports(&m->exports);
        free(m);
        return err;
    }

    m->initialized = true;
    *module = m;

    return WINLOADER_SUCCESS;
}

/* Resolve imports for a module */
static WINLOADER_ERROR resolve_imports(LOADED_MODULE *module)
{
    if (!module || !module->memory) {
        return WINLOADER_ERROR_NULL_POINTER;
    }

    IMAGE_DATA_DIRECTORY dir = module->pe_module.nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (dir.Size == 0) {
        return WINLOADER_SUCCESS;  /* No imports */
    }

    uint8_t *mem = module->memory->data;
    bool is_pe64 = module->pe_module.is_pe64;
    size_t ptr_size = is_pe64 ? 8 : 4;
    uint64_t ordinal_flag = is_pe64 ? 0x8000000000000000ULL : 0x80000000ULL;

    /* Read import descriptors */
    size_t offset = dir.VirtualAddress;

    while (1) {
        IMAGE_IMPORT_DESCRIPTOR desc;
        memcpy(&desc, mem + offset, sizeof(desc));

        if (desc.Name == 0) {
            break;  /* End of import descriptors */
        }

        /* Read library name */
        char *libname = (char *)(mem + desc.Name);

        /* Try to find module in cache first */
        LOADED_MODULE *lib = cache_find(libname);

        /* If not in cache, load from file */
        if (!lib) {
#ifdef _WIN32
            WINLOADER_ERROR err = winloader_load_from_file(libname, &lib);
            if (err != WINLOADER_SUCCESS) {
                set_error("Failed to load import library: %s", libname);
                return WINLOADER_ERROR_IMPORT_FAILED;
            }
            /* Add to cache for future use */
            winloader_cache_add(libname, lib);
#else
            set_error("Import library not found in cache: %s", libname);
            return WINLOADER_ERROR_IMPORT_FAILED;
#endif
        }

        /* Get thunk addresses */
        uint32_t thunk_addr = desc.OriginalFirstThunk;
        if (thunk_addr == 0) {
            thunk_addr = desc.FirstThunk;
        }
        uint32_t iat_addr = desc.FirstThunk;

        /* Process thunks */
        size_t thunk_offset = thunk_addr;
        size_t iat_offset = iat_addr;

        while (1) {
            uint64_t thunk_data = 0;
            if (is_pe64) {
                memcpy(&thunk_data, mem + thunk_offset, 8);
            } else {
                uint32_t thunk32;
                memcpy(&thunk32, mem + thunk_offset, 4);
                thunk_data = thunk32;
            }

            if (thunk_data == 0) {
                break;  /* End of thunks */
            }

            uint64_t proc_addr = 0;

            if (thunk_data & ordinal_flag) {
                /* Import by ordinal */
                uint16_t ordinal = (uint16_t)(thunk_data & 0xFFFF);
                proc_addr = winloader_get_ordinal(lib, ordinal);
                if (proc_addr == 0) {
                    set_error("Failed to resolve ordinal %u in %s", ordinal, libname);
                    return WINLOADER_ERROR_IMPORT_FAILED;
                }
            } else {
                /* Import by name */
                /* Hint is at thunk_data, name follows at thunk_data+2 */
                char *func_name = (char *)(mem + thunk_data + 2);
                proc_addr = winloader_get_proc(lib, func_name);
                if (proc_addr == 0) {
                    set_error("Failed to resolve symbol '%s' in %s", func_name, libname);
                    return WINLOADER_ERROR_IMPORT_FAILED;
                }
            }

            /* Write resolved address to IAT */
            if (is_pe64) {
                memcpy(mem + iat_offset, &proc_addr, 8);
            } else {
                uint32_t addr32 = (uint32_t)proc_addr;
                memcpy(mem + iat_offset, &addr32, 4);
            }

            thunk_offset += ptr_size;
            iat_offset += ptr_size;
        }

        offset += sizeof(IMAGE_IMPORT_DESCRIPTOR);
    }

    /* Flush instruction cache after modifying IAT */
    vmem_flush_instruction_cache(mem, module->memory->size);

    return WINLOADER_SUCCESS;
}

/* Execute TLS callbacks */
static WINLOADER_ERROR execute_tls_callbacks(LOADED_MODULE *module, uint32_t reason)
{
#ifdef _WIN32
    if (!module || !module->memory) {
        return WINLOADER_ERROR_NULL_POINTER;
    }

    IMAGE_DATA_DIRECTORY dir = module->pe_module.nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
    if (dir.Size == 0) {
        return WINLOADER_SUCCESS;  /* No TLS */
    }

    uint8_t *mem = module->memory->data;
    bool is_pe64 = module->pe_module.is_pe64;

    /* Read TLS directory */
    if (is_pe64) {
        IMAGE_TLS_DIRECTORY64 tls_dir;
        memcpy(&tls_dir, mem + dir.VirtualAddress, sizeof(tls_dir));

        if (tls_dir.AddressOfCallBacks != 0) {
            /* Callback addresses are stored as absolute addresses */
            uint64_t *callbacks = (uint64_t *)(uintptr_t)tls_dir.AddressOfCallBacks;
            while (*callbacks != 0) {
                typedef void (WINAPI *TLS_CALLBACK)(void *, uint32_t, void *);
                TLS_CALLBACK callback = (TLS_CALLBACK)(uintptr_t)*callbacks;
                callback((void *)module->base_addr, reason, NULL);
                callbacks++;
            }
        }
    } else {
        IMAGE_TLS_DIRECTORY32 tls_dir;
        memcpy(&tls_dir, mem + dir.VirtualAddress, sizeof(tls_dir));

        if (tls_dir.AddressOfCallBacks != 0) {
            uint32_t *callbacks = (uint32_t *)(uintptr_t)tls_dir.AddressOfCallBacks;
            while (*callbacks != 0) {
                typedef void (WINAPI *TLS_CALLBACK)(void *, uint32_t, void *);
                TLS_CALLBACK callback = (TLS_CALLBACK)(uintptr_t)*callbacks;
                callback((void *)module->base_addr, reason, NULL);
                callbacks++;
            }
        }
    }
#else
    (void)module;
    (void)reason;
#endif

    return WINLOADER_SUCCESS;
}

/* Call DLL entry point */
static WINLOADER_ERROR call_entry_point(LOADED_MODULE *module, uint32_t reason)
{
#ifdef _WIN32
    if (!module || !module->memory) {
        return WINLOADER_ERROR_NULL_POINTER;
    }

    uint32_t entry_rva = module->pe_module.nt_headers.OptionalHeader.AddressOfEntryPoint;
    if (entry_rva == 0) {
        return WINLOADER_SUCCESS;  /* No entry point */
    }

    uint64_t entry_addr = module->base_addr + entry_rva;

    /* Check if this is a DLL */
    bool is_dll = (module->pe_module.nt_headers.FileHeader.Characteristics & IMAGE_FILE_DLL) != 0;

    if (is_dll) {
        typedef BOOL (WINAPI *DLL_ENTRY_POINT)(HINSTANCE, DWORD, LPVOID);
        DLL_ENTRY_POINT entry = (DLL_ENTRY_POINT)(uintptr_t)entry_addr;

        BOOL result = entry((HINSTANCE)module->base_addr, reason, NULL);
        if (!result && reason == DLL_PROCESS_ATTACH) {
            set_error("DLL entry point returned FALSE");
            return WINLOADER_ERROR_ENTRY_POINT_FAILED;
        }
    }
    /* For EXE, we don't call the entry point automatically */
#else
    (void)module;
    (void)reason;
#endif

    return WINLOADER_SUCCESS;
}

/* Get procedure address by name */
uint64_t winloader_get_proc(LOADED_MODULE *module, const char *name)
{
    if (!module || !name) return 0;

    if (module->type == MODULE_TYPE_NATIVE) {
#ifdef _WIN32
        FARPROC proc = GetProcAddress(module->native_handle, name);
        return (uint64_t)(uintptr_t)proc;
#else
        return 0;
#endif
    }

    return pe_get_proc(&module->exports, name);
}

/* Get procedure address by ordinal */
uint64_t winloader_get_ordinal(LOADED_MODULE *module, uint16_t ordinal)
{
    if (!module) return 0;

    if (module->type == MODULE_TYPE_NATIVE) {
#ifdef _WIN32
        FARPROC proc = GetProcAddress(module->native_handle, MAKEINTRESOURCEA(ordinal));
        return (uint64_t)(uintptr_t)proc;
#else
        return 0;
#endif
    }

    return pe_get_ordinal(&module->exports, ordinal);
}

/* Free a loaded module */
void winloader_free(LOADED_MODULE *module)
{
    if (!module) return;

    if (module->type == MODULE_TYPE_MEMORY) {
        if (module->initialized) {
            /* Call detach */
            execute_tls_callbacks(module, DLL_PROCESS_DETACH);
            call_entry_point(module, DLL_PROCESS_DETACH);
        }

        pe_free_exports(&module->exports);

        if (module->memory) {
            vmem_free(module->memory);
        }
    } else if (module->type == MODULE_TYPE_NATIVE) {
#ifdef _WIN32
        if (module->native_handle) {
            FreeLibrary(module->native_handle);
        }
#endif
    }

    free(module);
}

/* Get module HINSTANCE */
uint64_t winloader_get_instance(LOADED_MODULE *module)
{
    if (!module) return 0;

    if (module->type == MODULE_TYPE_NATIVE) {
#ifdef _WIN32
        return (uint64_t)(uintptr_t)module->native_handle;
#else
        return 0;
#endif
    }

    return module->base_addr;
}

/* Find module in cache */
static LOADED_MODULE *cache_find(const char *name)
{
    if (!name) return NULL;

    /* Normalize name by removing .dll extension if present */
    char normalized[256];
    strncpy(normalized, name, sizeof(normalized) - 1);
    normalized[sizeof(normalized) - 1] = 0;

    size_t len = strlen(normalized);
    if (len > 4 && stricmp_portable(normalized + len - 4, ".dll") == 0) {
        normalized[len - 4] = 0;
    }

    /* Search cache */
    CACHE_ENTRY *entry = s_cache_head;
    while (entry) {
        if (stricmp_portable(entry->name, normalized) == 0) {
            return entry->module;
        }
        /* Also check with .dll extension */
        char with_ext[260];
        snprintf(with_ext, sizeof(with_ext), "%s.dll", entry->name);
        if (stricmp_portable(with_ext, name) == 0) {
            return entry->module;
        }
        entry = entry->next;
    }

    return NULL;
}

/* Add module to cache */
WINLOADER_ERROR winloader_cache_add(const char *name, LOADED_MODULE *module)
{
    if (!name || !module) {
        return WINLOADER_ERROR_NULL_POINTER;
    }

    /* Normalize name */
    char *normalized = strdup(name);
    if (!normalized) {
        return WINLOADER_ERROR_MEMORY_ALLOC;
    }

    /* Convert to lowercase and remove .dll extension */
    for (char *p = normalized; *p; p++) {
        if (*p >= 'A' && *p <= 'Z') {
            *p += 32;
        }
    }
    size_t len = strlen(normalized);
    if (len > 4 && strcmp(normalized + len - 4, ".dll") == 0) {
        normalized[len - 4] = 0;
    }

    /* Create cache entry */
    CACHE_ENTRY *entry = (CACHE_ENTRY *)malloc(sizeof(CACHE_ENTRY));
    if (!entry) {
        free(normalized);
        return WINLOADER_ERROR_MEMORY_ALLOC;
    }

    entry->name = normalized;
    entry->module = module;
    entry->next = s_cache_head;
    s_cache_head = entry;

    return WINLOADER_SUCCESS;
}

/* Remove module from cache */
void winloader_cache_remove(const char *name)
{
    if (!name) return;

    /* Normalize name */
    char normalized[256];
    strncpy(normalized, name, sizeof(normalized) - 1);
    normalized[sizeof(normalized) - 1] = 0;

    for (char *p = normalized; *p; p++) {
        if (*p >= 'A' && *p <= 'Z') {
            *p += 32;
        }
    }
    size_t len = strlen(normalized);
    if (len > 4 && strcmp(normalized + len - 4, ".dll") == 0) {
        normalized[len - 4] = 0;
    }

    /* Find and remove entry */
    CACHE_ENTRY **pp = &s_cache_head;
    while (*pp) {
        if (strcmp((*pp)->name, normalized) == 0) {
            CACHE_ENTRY *entry = *pp;
            *pp = entry->next;
            free(entry->name);
            free(entry);
            return;
        }
        pp = &(*pp)->next;
    }
}

/* Get last error message */
const char *winloader_get_error_message(void)
{
    return s_error_message;
}

/* Call a procedure with arguments */
uint64_t winloader_call(uint64_t addr, int num_args, ...)
{
    if (addr == 0) return 0;

#ifdef _WIN32
    va_list args;
    va_start(args, num_args);

    /* For x64, we can only reliably call functions with up to 4 arguments
     * using the standard calling convention without assembly.
     * For more complex cases, the caller should use the address directly. */
    typedef uint64_t (*PROC0)(void);
    typedef uint64_t (*PROC1)(uint64_t);
    typedef uint64_t (*PROC2)(uint64_t, uint64_t);
    typedef uint64_t (*PROC3)(uint64_t, uint64_t, uint64_t);
    typedef uint64_t (*PROC4)(uint64_t, uint64_t, uint64_t, uint64_t);

    uint64_t result = 0;
    uint64_t a[4] = {0};

    for (int i = 0; i < num_args && i < 4; i++) {
        a[i] = va_arg(args, uint64_t);
    }

    va_end(args);

    switch (num_args) {
    case 0:
        result = ((PROC0)(uintptr_t)addr)();
        break;
    case 1:
        result = ((PROC1)(uintptr_t)addr)(a[0]);
        break;
    case 2:
        result = ((PROC2)(uintptr_t)addr)(a[0], a[1]);
        break;
    case 3:
        result = ((PROC3)(uintptr_t)addr)(a[0], a[1], a[2]);
        break;
    case 4:
    default:
        result = ((PROC4)(uintptr_t)addr)(a[0], a[1], a[2], a[3]);
        break;
    }

    return result;
#else
    (void)num_args;
    return 0;
#endif
}
