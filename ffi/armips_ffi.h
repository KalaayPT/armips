#pragma once

#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * C FFI for armips assembler
 * Mirrors the CLI interface with a C-compatible API
 */

typedef struct {
    const char* input_file;        /* Required: path to .asm file */
    const char* working_dir;       /* Optional: working directory (-root) */
    const char* temp_file;         /* Optional: temp output file (-temp) */
    const char* sym_file;          /* Optional: symbol file (-sym/-sym2) */
    int sym_version;               /* 0=none, 1=-sym, 2=-sym2 */

    /* Equations and labels: array of "NAME=VALUE" strings */
    const char** defines;
    size_t define_count;

    /* Flags */
    int error_on_warning;          /* Treat warnings as errors */
    int silent;                    /* Suppress output */
    int show_stats;                /* Show area statistics */

    /* Output: error messages (allocated on failure, NULL on success) */
    char** errors;
    size_t error_count;
} ArmipsFFIArgs;

/* Assemble a file. Returns 0 on success, non-zero on failure.
 * On failure, errors[] contains allocated C strings that must be freed
 * by calling armips_free_errors().
 */
int armips_assemble(const ArmipsFFIArgs* args);

/* Free error strings allocated by armips_assemble */
void armips_free_errors(char** errors, size_t count);

/* Get armips version */
void armips_version(int* major, int* minor, int* revision);

#ifdef __cplusplus
}
#endif
