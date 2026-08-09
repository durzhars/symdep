/*
 * Symlink & Dependency Manager (symdep)
 * Configuration Submodules Internal Header
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

#ifndef SYMDEP_CONFIG_INTERNAL_H
#define SYMDEP_CONFIG_INTERNAL_H

#define _GNU_SOURCE
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/config.h"
#include "core/manifest.h"
#include "utils/env.h"
#include "utils/fs.h"
#include "utils/logger.h"
#include "utils/path.h"

/* Internal Shared Helper Declarations */
void config_load_active(Config *cfg);

bool prepare_config_path(const char *input_path,
                                char *out_buf,
                                size_t buf_size,
                                const char *context,
                                bool check_sanity);

const char *getenv_first(const char *const names[], size_t count);

#endif /* SYMDEP_CONFIG_INTERNAL_H */
