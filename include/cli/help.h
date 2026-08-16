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

#ifndef SYMDEP_HELP_H
#define SYMDEP_HELP_H

/**
 * @brief Display top-level usage manual and command summary to stdout.
 */
void show_help(void);

/**
 * @brief Display detailed manual and workflow examples for the dependency scanner (`symdep scan`).
 */
void show_scan_help(void);

/**
 * @brief Display detailed documentation manual for a specific subcommand topic.
 *
 * @param topic Subcommand topic identifier (e.g. "config", "scan").
 */
void show_subcommand_help(const char *topic);

#endif /* SYMDEP_HELP_H */
