/**
 * @file ptx_logging.h
 * @brief PTX Logging library for Arduino projects
 * @details Provides formatted logging with timestamp, filename and line number
 */

#ifndef PTX_LOGGING_H
#define PTX_LOGGING_H

#include <Arduino.h>

/**
 * @brief Log macro with automatic file and line detection
 * @param msg Message string to log
 */
#define PTX_LOG(msg) ptx_log(__FILE__, __LINE__, msg)

/**
 * @brief Log macro with printf-style formatting
 * @param format Printf-style format string
 * @param ... Variable arguments for formatting
 */
#define PTX_LOGF(format, ...) ptx_logf(__FILE__, __LINE__, format, ##__VA_ARGS__)

/**
 * @brief Initialize logging system
 * @details Sets up Serial communication for logging output
 */
void ptx_log_init();

/**
 * @brief Basic logging function
 * @param file Source file name (automatically provided by macro)
 * @param line Line number (automatically provided by macro)
 * @param msg Message to log
 */
void ptx_log(const char* file, int line, const char* msg);

/**
 * @brief Formatted logging function
 * @param file Source file name (automatically provided by macro)
 * @param line Line number (automatically provided by macro)
 * @param format Printf-style format string
 * @param ... Variable arguments for formatting
 */
void ptx_logf(const char* file, int line, const char* format, ...);

/**
 * @brief Extract filename from full path
 * @param path Full file path
 * @return Pointer to filename portion
 */
const char* ptx_get_filename(const char* path);

#endif // PTX_LOGGING_H
