# WinLoader

A C library for loading Windows PE modules directly from memory without requiring temporary files on disk.

**Ported from [jchv/go-winloader](https://github.com/jchv/go-winloader)**

## Overview

WinLoader reimplements Windows' module loading functionality in pure C. The primary advantage of this library is that you can load modules directly from memory without needing to write intermediate temporary files to disk.

## Features

- **Memory-based DLL loading**: Load PE modules directly from byte arrays
- **Native loader fallback**: Can also load DLLs from disk using the native Windows loader
- **Import resolution**: Automatically resolves imports from other loaded modules
- **Export table support**: Look up exported functions by name or ordinal
- **TLS callback support**: Executes TLS callbacks during module initialization
- **Base relocation**: Handles modules loaded at non-preferred addresses
- **Module caching**: Cache modules for import resolution

## Limitations

- **Windows only**: The memory loading functionality only works on Windows
- **Architecture specific**: Modules must match the host architecture (x86, x64, ARM64)
- **No WinSXS support**: Side-by-side assembly manifests are not supported
- **Experimental**: No guarantees of API or runtime stability

## Building

### Requirements

- C compiler (GCC, Clang, or MSVC)
- Make (GNU Make on Unix, or nmake on Windows)

### Build Commands

```bash
# Build library and examples
make

# Build library only
make lib

# Build with debug symbols
make debug

# Clean build artifacts
make clean

# Install (Unix-like systems)
sudo make install
```

### Windows with MSVC

```batch
cl /c /I include src\*.c
lib /OUT:winloader.lib *.obj
```

## Usage

### Basic Example

```c
#include <stdio.h>
#include "winloader.h"

int main() {
    // Read DLL file into memory
    uint8_t *dll_data = read_file("example.dll", &size);

    // Load DLL from memory
    LOADED_MODULE *module = NULL;
    WINLOADER_ERROR err = winloader_load_from_memory(dll_data, size, &module);

    if (err != WINLOADER_SUCCESS) {
        printf("Failed to load: %s\n", winloader_get_error_message());
        return 1;
    }

    // Get exported function
    uint64_t func_addr = winloader_get_proc(module, "MyFunction");

    // Call the function
    typedef int (*MyFunctionType)(int, int);
    MyFunctionType func = (MyFunctionType)(uintptr_t)func_addr;
    int result = func(1, 2);

    // Cleanup
    winloader_free(module);
    free(dll_data);

    return 0;
}
```

### Loading from File

```c
LOADED_MODULE *module = NULL;
WINLOADER_ERROR err = winloader_load_from_file("kernel32.dll", &module);
```

### Module Caching

```c
// Add a module to the cache so other memory-loaded modules can import from it
winloader_cache_add("mylib.dll", module);

// Remove from cache
winloader_cache_remove("mylib.dll");
```

## API Reference

### Module Loading

```c
// Load a PE module from memory
WINLOADER_ERROR winloader_load_from_memory(const uint8_t *data, size_t size,
                                           LOADED_MODULE **module);

// Load a PE module from file using native Windows loader
WINLOADER_ERROR winloader_load_from_file(const char *filename,
                                         LOADED_MODULE **module);

// Free a loaded module
void winloader_free(LOADED_MODULE *module);
```

### Procedure Lookup

```c
// Get procedure address by name
uint64_t winloader_get_proc(LOADED_MODULE *module, const char *name);

// Get procedure address by ordinal
uint64_t winloader_get_ordinal(LOADED_MODULE *module, uint16_t ordinal);

// Call a procedure with up to 4 arguments
uint64_t winloader_call(uint64_t addr, int num_args, ...);
```

### Module Information

```c
// Get the HINSTANCE/HMODULE of a loaded module
uint64_t winloader_get_instance(LOADED_MODULE *module);

// Check if architecture is supported
bool winloader_is_architecture_supported(uint16_t machine);

// Get native machine architecture
uint16_t winloader_get_native_architecture();
```

### Error Handling

```c
// Get last error message
const char *winloader_get_error_message(void);
```

### Module Cache

```c
// Add a module to the loader cache
WINLOADER_ERROR winloader_cache_add(const char *name, LOADED_MODULE *module);

// Remove a module from the cache
void winloader_cache_remove(const char *name);
```

## Error Codes

| Error Code | Description |
|------------|-------------|
| `WINLOADER_SUCCESS` | Operation completed successfully |
| `WINLOADER_ERROR_INVALID_PE` | Invalid PE format |
| `WINLOADER_ERROR_MEMORY_ALLOC` | Memory allocation failed |
| `WINLOADER_ERROR_ARCHITECTURE_MISMATCH` | Module architecture doesn't match host |
| `WINLOADER_ERROR_RELOCATION_FAILED` | Failed to apply base relocations |
| `WINLOADER_ERROR_IMPORT_FAILED` | Failed to resolve imports |
| `WINLOADER_ERROR_PROTECTION_FAILED` | Failed to set memory protection |
| `WINLOADER_ERROR_ENTRY_POINT_FAILED` | DLL entry point returned FALSE |
| `WINLOADER_ERROR_TLS_FAILED` | TLS callback failed |
| `WINLOADER_ERROR_NOT_FOUND` | Module or symbol not found |
| `WINLOADER_ERROR_LOAD_FAILED` | Failed to load module |
| `WINLOADER_ERROR_NULL_POINTER` | Null pointer argument |

## Project Structure

```
winloader/
├── include/
│   ├── winloader.h    # Main public API
│   ├── pe.h           # PE format structures
│   └── vmem.h         # Virtual memory management
├── src/
│   ├── winloader.c    # Main loader implementation
│   ├── pe.c           # PE parsing
│   └── vmem.c         # Virtual memory operations
├── examples/
│   └── demo.c         # Example program
├── Makefile
└── README.md
```

## Architecture Notes

Unlike the Windows API which uses processor-sized register values internally, this implementation uses `uint64_t` for internal values to remain architecture-neutral at compile time. This approach may help in future cross-architecture scenarios.

## License

ISC License (same as the original go-winloader)

## Credits

- Original Go implementation: [jchv/go-winloader](https://github.com/jchv/go-winloader)
- C port: This repository

## See Also

- [PE Format Documentation](https://docs.microsoft.com/en-us/windows/win32/debug/pe-format)
- [Windows Loader](https://docs.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-functions)
