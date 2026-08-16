/*
 * Symlink & Dependency Manager (symdep)
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

#define _GNU_SOURCE
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "../test_framework.h"
#include "utils/fs.h"
#include "utils/logger.h"
#include <stdio.h>
#include <string.h>

void test_logger_output(void)
{
    log_info("Testing logger info message");
    log_warn("Testing logger warning message");
    log_error("Testing logger error message");
    log_success("Testing logger success message");
    log_debug("Testing logger debug message");
}

void test_logger_file_logging_and_filtering(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "log_test") != NULL,
           "Should create temp dir for logger test");

    char log_file[PATH_MAX];
    snprintf(log_file, sizeof(log_file), "%s/symdep.log", tmp_dir);

    // Initialize with WARN level: DEBUG, INFO, SUCCESS should be filtered out
    logger_init(LOG_LEVEL_WARN, log_file);

    log_debug("SUPPRESSED_DEBUG_MSG_123");
    log_info("SUPPRESSED_INFO_MSG_456");
    log_success("SUPPRESSED_SUCCESS_MSG_789");
    log_warn("CAPTURED_WARN_MSG_ABC");
    log_error("CAPTURED_ERROR_MSG_DEF");

    logger_close();

    // Verify log file contents
    FILE *fp = fopen(log_file, "r");
    ASSERT(fp != NULL, "Log file should exist and be readable");

    char content[4096] = {0};
    size_t bytes_read = fread(content, 1, sizeof(content) - 1, fp);
    fclose(fp);
    content[bytes_read] = '\0';

    ASSERT(strstr(content, "SUPPRESSED_DEBUG_MSG_123") == NULL,
           "DEBUG message should be filtered out from file");
    ASSERT(strstr(content, "SUPPRESSED_INFO_MSG_456") == NULL,
           "INFO message should be filtered out from file");
    ASSERT(strstr(content, "SUPPRESSED_SUCCESS_MSG_789") == NULL,
           "SUCCESS message should be filtered out from file");

    ASSERT(strstr(content, "CAPTURED_WARN_MSG_ABC") != NULL,
           "WARN message should be present in file");
    ASSERT(strstr(content, "CAPTURED_ERROR_MSG_DEF") != NULL,
           "ERROR message should be present in file");
    ASSERT(strstr(content, "[WARNING]") != NULL, "Log file should contain [WARNING] tag");
    ASSERT(strstr(content, "[ERROR]") != NULL, "Log file should contain [ERROR] tag");

    cleanup_test_dir(tmp_dir);
}

void test_logger_level_switching(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "log_lvl") != NULL,
           "Should create temp dir for logger level switch test");

    char log_file[PATH_MAX];
    snprintf(log_file, sizeof(log_file), "%s/symdep_switch.log", tmp_dir);

    logger_init(LOG_LEVEL_ERROR, log_file);
    log_warn("IGNORED_WARN_BEFORE_SWITCH");

    logger_set_level(LOG_LEVEL_DEBUG);
    log_debug("CAPTURED_DEBUG_AFTER_SWITCH");

    logger_close();

    FILE *fp = fopen(log_file, "r");
    ASSERT(fp != NULL, "Switch log file should exist");

    char content[4096] = {0};
    size_t bytes_read = fread(content, 1, sizeof(content) - 1, fp);
    fclose(fp);
    content[bytes_read] = '\0';

    ASSERT(strstr(content, "IGNORED_WARN_BEFORE_SWITCH") == NULL,
           "WARN message before switch should be ignored at ERROR level");
    ASSERT(strstr(content, "CAPTURED_DEBUG_AFTER_SWITCH") != NULL,
           "DEBUG message should be captured after switching to DEBUG level");

    // Reset logger level to default INFO
    logger_set_level(LOG_LEVEL_INFO);

    cleanup_test_dir(tmp_dir);
}

void test_logger_close_idempotent(void)
{
    logger_close();
    logger_close();
    // Re-init with NULL path should be safe
    logger_init(LOG_LEVEL_INFO, NULL);
    logger_close();
}
