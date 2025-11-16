#include <stdarg.h>
#include <stdio.h>
#include "ptx_logging.h"

extern "C" void ptx_log_init() { /* no-op for tests */ }
extern "C" void ptx_log(const char* file, int line, const char* msg) { /* no-op */ }
extern "C" void ptx_logf(const char* file, int line, const char* format, ...) {
    // Optional: capture logs for assertions; for now, ignore.
    (void)file; (void)line; (void)format;
}
extern "C" const char* ptx_get_filename(const char* path) { return path; }
