/*
 * WinLoader Demo
 *
 * This example demonstrates loading a DLL from memory
 * and calling its exported functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "winloader.h"

/* Helper function to read a file into memory */
static uint8_t *read_file(const char *filename, size_t *size)
{
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    *size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *data = (uint8_t *)malloc(*size);
    if (!data) {
        fclose(f);
        fprintf(stderr, "Failed to allocate memory for file\n");
        return NULL;
    }

    if (fread(data, 1, *size, f) != *size) {
        fclose(f);
        free(data);
        fprintf(stderr, "Failed to read file\n");
        return NULL;
    }

    fclose(f);
    return data;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("WinLoader Demo\n");
        printf("Ported from jchv/go-winloader\n\n");
        printf("Usage: %s <dll_file> [function_name]\n\n", argv[0]);
        printf("This program loads a DLL from memory and optionally\n");
        printf("calls an exported function that takes no arguments.\n");
        return 1;
    }

    const char *dll_path = argv[1];
    const char *func_name = (argc > 2) ? argv[2] : NULL;

    printf("WinLoader Demo\n");
    printf("==============\n\n");

    /* Check native architecture */
    uint16_t native_arch = winloader_get_native_architecture();
    printf("Native architecture: ");
    switch (native_arch) {
    case 0x8664: printf("x64 (AMD64)\n"); break;
    case 0x014c: printf("x86 (i386)\n"); break;
    case 0xaa64: printf("ARM64\n"); break;
    case 0x01c4: printf("ARM (Thumb-2)\n"); break;
    default: printf("Unknown (0x%04x)\n", native_arch); break;
    }
    printf("\n");

    /* Read DLL file */
    printf("Loading DLL: %s\n", dll_path);
    size_t dll_size;
    uint8_t *dll_data = read_file(dll_path, &dll_size);
    if (!dll_data) {
        return 1;
    }
    printf("File size: %zu bytes\n", dll_size);

    /* Load DLL from memory */
    LOADED_MODULE *module = NULL;
    WINLOADER_ERROR err = winloader_load_from_memory(dll_data, dll_size, &module);

    /* Free file data - no longer needed */
    free(dll_data);

    if (err != WINLOADER_SUCCESS) {
        fprintf(stderr, "Failed to load DLL: %s\n", winloader_get_error_message());
        return 1;
    }

    printf("DLL loaded successfully!\n");
    printf("Module base address: 0x%llx\n", (unsigned long long)winloader_get_instance(module));
    printf("\n");

    /* Call function if specified */
    if (func_name) {
        printf("Looking up function: %s\n", func_name);
        uint64_t func_addr = winloader_get_proc(module, func_name);

        if (func_addr == 0) {
            fprintf(stderr, "Function not found: %s\n", func_name);
            winloader_free(module);
            return 1;
        }

        printf("Function address: 0x%llx\n", (unsigned long long)func_addr);
        printf("Calling function...\n\n");

        /* Call the function with no arguments */
        uint64_t result = winloader_call(func_addr, 0);
        printf("\nFunction returned: %llu (0x%llx)\n",
               (unsigned long long)result, (unsigned long long)result);
    }

    /* Cleanup */
    printf("\nUnloading DLL...\n");
    winloader_free(module);
    printf("Done.\n");

    return 0;
}
