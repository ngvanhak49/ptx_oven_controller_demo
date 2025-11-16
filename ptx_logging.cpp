/**
 * @file ptx_logging.cpp
 * @brief Implementation of PTX logging functions
 */

#include "ptx_logging.h"
#include <stdarg.h>

void ptx_log_init() {
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for Serial port to connect
    }
}

const char* ptx_get_filename(const char* path) {
    const char* filename = path;
    
    // Find last '\' or '/' character
    for (const char* p = path; *p; p++) {
        if (*p == '\\' || *p == '/') {
            filename = p + 1;
        }
    }
    
    return filename;
}

void ptx_log(const char* file, int line, const char* msg) {
    unsigned long currentTime = millis();
    const char* filename = ptx_get_filename(file);
    
    // Format: [time][filename:line] message
    Serial.print("[");
    Serial.print(currentTime);
    Serial.print("][");
    Serial.print(filename);
    Serial.print(":");
    Serial.print(line);
    Serial.print("] ");
    Serial.println(msg);
}

void ptx_logf(const char* file, int line, const char* format, ...) {
    char buffer[256];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    ptx_log(file, line, buffer);
}
