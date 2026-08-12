/*
 * Symlink & Dependency Manager (symdep)
 * Signal Safety & Temporary Path Cleanup Engine
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
#ifndef UTILS_SIGNAL_H
#define UTILS_SIGNAL_H

#include <signal.h>

/**
 * @brief Global atomic process interruption flag set by signal handlers.
 */
extern volatile sig_atomic_t g_interrupted;

/**
 * @brief Install POSIX sigaction handlers for SIGINT, SIGTERM, and SIGHUP.
 */
void setup_signal_handlers(void);

/**
 * @brief Register a temporary directory or scratch file for atomic cleanup on exit/signal.
 *
 * @param path Temporary file or directory path.
 */
void register_temp_path(const char *path);

/**
 * @brief Unregister a temporary path after successful completion.
 *
 * @param path Temporary path string.
 */
void unregister_temp_path(const char *path);

/**
 * @brief Clean up all registered temporary paths (standard process exit).
 */
void cleanup_temp_paths(void);

/**
 * @brief Async-signal-safe cleanup handler invoked from signal handlers on SIGINT/SIGTERM.
 */
void cleanup_temp_paths_signal_safe(void);

#endif /* UTILS_SIGNAL_H */
