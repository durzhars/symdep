/*
 * Dotfiles Stow Manager (stow-manager)
 * Copyright (C) 2026 durzhars
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SYMDEP_LOGGER_H
#define SYMDEP_LOGGER_H

#include <stdbool.h>

/**
 * @enum LogLevel
 * @brief Logging severity levels.
 */
typedef enum {
    LOG_LEVEL_DEBUG = 0, /**< Detailed diagnostic traces */
    LOG_LEVEL_INFO,      /**< Informational runtime events */
    LOG_LEVEL_SUCCESS,   /**< Successful operation completion */
    LOG_LEVEL_WARN,      /**< Warning conditions and non-fatal conflicts */
    LOG_LEVEL_ERROR,     /**< Fatal or recoverable runtime errors */
    LOG_LEVEL_OFF        /**< Suppress all log outputs */
} LogLevel;

/**
 * @brief Initialize logger with a specific severity threshold and optional log file.
 *
 * @param level         Minimum severity level to emit.
 * @param log_file_path Optional destination log file path (NULL for stdout/stderr only).
 */
void logger_init(LogLevel level, const char *log_file_path);

/** Set the active logging severity threshold level */
void logger_set_level(LogLevel level);

/** Close open log file descriptors and flush log buffers */
void logger_close(void);

/** Log debug message (emitted only when level is LOG_LEVEL_DEBUG) */
void log_debug(const char *fmt, ...);

/** Log standard informational message */
void log_info(const char *fmt, ...);

/** Log successful operation message (colorized green) */
void log_success(const char *fmt, ...);

/** Log warning message (colorized yellow) */
void log_warn(const char *fmt, ...);

/** Log error message to stderr (colorized red) */
void log_error(const char *fmt, ...);

#endif /* SYMDEP_LOGGER_H */
