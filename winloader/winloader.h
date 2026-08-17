/*
 * WinLoader - Windows PE Memory Loader
 * Ported from jchv/go-winloader
 *
 * A library for loading Windows PE modules directly from memory
 * without requiring temporary files on disk.
 */

#ifndef WINLOADER_H
#define WINLOADER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct _LOADED_MODULE;
typedef struct _LOADED_MODULE LOADED_MODULE;

/* Error codes */
typedef enum _WINLOADER_ERROR {
    WINLOADER_SUCCESS = 0,
    WINLOADER_ERROR_INVALID_PE,
    WINLOADER_ERROR_MEMORY_ALLOC,
    WINLOADER_ERROR_ARCHITECTURE_MISMATCH,
    WINLOADER_ERROR_RELOCATION_FAILED,
    WINLOADER_ERROR_IMPORT_FAILED,
    WINLOADER_ERROR_PROTECTION_FAILED,
    WINLOADER_ERROR_ENTRY_POINT_FAILED,
    WINLOADER_ERROR_TLS_FAILED,
    WINLOADER_ERROR_NOT_FOUND,
    WINLOADER_ERROR_LOAD_FAILED,
    WINLOADER_ERROR_NULL_POINTER
} WINLOADER_ERROR;

/* Procedure callback type for calls */
typedef uint64_t (*PROC_CALLBACK)(void);

/* DLL reason codes for DllMain */
#define DLL_PROCESS_DETACH 0
#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH  2
#define DLL_THREAD_DETACH  3

/**
 * Load a PE module from memory
 * @param data Pointer to PE file data
 * @param size Size of the data buffer
 * @param module Output: Handle to loaded module
 * @return WINLOADER_SUCCESS on success, error code otherwise
 */
WINLOADER_ERROR winloader_load_from_memory(const uint8_t *data, size_t size,
                                           LOADED_MODULE **module);

/**
 * Load a PE module from file using native Windows loader
 * @param filename Path to the DLL file
 * @param module Output: Handle to loaded module
 * @return WINLOADER_SUCCESS on success, error code otherwise
 */
WINLOADER_ERROR winloader_load_from_file(const char *filename,
                                         LOADED_MODULE **module);

/**
 * Get procedure address by name
 * @param module Loaded module
 * @param name Procedure name
 * @return Procedure address, or 0 if not found
 */
uint64_t winloader_get_proc(LOADED_MODULE *module, const char *name);

/**
 * Get procedure address by ordinal
 * @param module Loaded module
 * @param ordinal Procedure ordinal
 * @return Procedure address, or 0 if not found
 */
uint64_t winloader_get_ordinal(LOADED_MODULE *module, uint16_t ordinal);

/**
 * Free a loaded module
 * @param module Module to free
 */
void winloader_free(LOADED_MODULE *module);

/**
 * Add a module to the loader cache
 * This allows memory-loaded libraries to link to it
 * @param name Module name
 * @param module Module to add
 * @return WINLOADER_SUCCESS on success, error code otherwise
 */
WINLOADER_ERROR winloader_cache_add(const char *name, LOADED_MODULE *module);

/**
 * Remove a module from the loader cache
 * @param name Module name
 */
void winloader_cache_remove(const char *name);

/**
 * Get the HINSTANCE/HMODULE of a loaded module
 * @param module Loaded module
 * @return HINSTANCE value
 */
uint64_t winloader_get_instance(LOADED_MODULE *module);

/**
 * Get last error message
 * @return Error message string (do not free)
 */
const char *winloader_get_error_message(void);

/**
 * Check if architecture is supported for loading
 * @param machine Machine type from PE header
 * @return true if supported, false otherwise
 */
bool winloader_is_architecture_supported(uint16_t machine);

/**
 * Get native machine architecture
 * @return Native machine type constant
 */
uint16_t winloader_get_native_architecture(void);

/**
 * Call a procedure with arguments
 * @param addr Procedure address
 * @param num_args Number of arguments
 * @param ... Arguments (as uint64_t)
 * @return Return value from procedure
 */
uint64_t winloader_call(uint64_t addr, int num_args, ...);

#ifdef __cplusplus
}
#endif

#endif /* WINLOADER_H */
