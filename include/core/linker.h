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

#ifndef SYMDEP_LINKER_H
#define SYMDEP_LINKER_H

#include "core/file_collector.h"
#include "utils/fs.h"

typedef enum { LINK_STATUS_UNLINKED = 0, LINK_STATUS_PARTIAL, LINK_STATUS_LINKED } LinkStatus;
typedef enum { LINK_ACTION_LINK = 0, LINK_ACTION_UNLINK, LINK_ACTION_RELINK } LinkAction;

/* Legacy aliases for backwards compatibility */
typedef LinkStatus StowStatus;
#define STOW_STATUS_UNSTOWED LINK_STATUS_UNLINKED
#define STOW_STATUS_PARTIAL LINK_STATUS_PARTIAL
#define STOW_STATUS_STOWED LINK_STATUS_LINKED

typedef LinkAction StowAction;
#define STOW_ACTION_STOW LINK_ACTION_LINK
#define STOW_ACTION_UNSTOW LINK_ACTION_UNLINK
#define STOW_ACTION_RESTOW LINK_ACTION_RELINK

/**
 * @brief Walk symlinks in target directory that match package file patterns.
 *
 * @param target_dir Target home directory.
 * @param source_dir Source repository directory.
 * @param pkg_files  List of files owned by package.
 * @param cb         Callback function invoked per discovered symlink.
 * @param user_data  Opaque pointer passed to callback.
 */
void walk_target_dir_symlinks_targeted(const char *target_dir,
                                       const char *source_dir,
                                       const PkgFileList *pkg_files,
                                       WalkSymlinkCallback cb,
                                       void *user_data);

/**
 * @brief Dynamically unfold target directory symlinks to resolve tree folding collisions.
 *
 * @param target_dir Target destination directory.
 * @param source_dir Active source repository directory.
 * @param pkg_files  List of files belonging to incoming package.
 * @param dry_run    If true, preview changes without modifying disk.
 */
void unfold_directory_symlinks(const char *target_dir,
                               const char *source_dir,
                               const PkgFileList *pkg_files,
                               bool dry_run);

/**
 * @brief Prepare target directory conflicts before linking.
 *
 * Backs up colliding non-symlink target files before deploying package symlinks.
 *
 * @param target_dir Target destination directory.
 * @param source_dir Active source repository directory.
 * @param pkg_name   Package name.
 * @param pkg_files  Files owned by package.
 * @param dry_run    If true, preview changes without modifying disk.
 */
void prepare_target_conflicts(const char *target_dir,
                              const char *source_dir,
                              const char *pkg_name,
                              const PkgFileList *pkg_files,
                              bool dry_run);

/**
 * @brief Evaluate active deployment status of a package.
 *
 * @param target_dir Target destination directory.
 * @param source_dir Active source repository directory.
 * @param pkg_name   Package name.
 * @return LINK_STATUS_LINKED, LINK_STATUS_PARTIAL, or LINK_STATUS_UNLINKED.
 */
LinkStatus
get_package_link_status(const char *target_dir, const char *source_dir, const char *pkg_name);

/**
 * @brief Check if package is fully linked.
 *
 * @param target_dir Target destination directory.
 * @param source_dir Active source repository directory.
 * @param pkg_name   Package name.
 * @return true if status is LINK_STATUS_LINKED.
 */
bool is_package_linked(const char *target_dir, const char *source_dir, const char *pkg_name);

/**
 * @brief Unlink mutually exclusive packages defined in package manifest.
 *
 * @param target_dir Target destination directory.
 * @param source_dir Active source repository directory.
 * @param pkg_name   Package name.
 * @param dry_run    If true, preview changes without modifying disk.
 */
void handle_mutual_exclusions(const char *target_dir,
                              const char *source_dir,
                              const char *pkg_name,
                              bool dry_run);

/**
 * @brief Deploy symlinks for a single package.
 *
 * @param source_dir   Active source repository directory.
 * @param target_dir   Target destination directory.
 * @param pkg_name     Package name.
 * @param auto_install Auto-confirm missing dependency installation.
 * @param dry_run      Preview mode without modifying disk.
 * @return 0 on success, non-zero on failure.
 */
int link_package(const char *source_dir,
                 const char *target_dir,
                 const char *pkg_name,
                 bool auto_install,
                 bool dry_run);

/**
 * @brief Remove stowed symlinks for a single package.
 *
 * @param source_dir Active source repository directory.
 * @param target_dir Target destination directory.
 * @param pkg_name   Package name.
 * @param dry_run    Preview mode without modifying disk.
 * @return 0 on success, non-zero on failure.
 */
int unlink_package(const char *source_dir,
                   const char *target_dir,
                   const char *pkg_name,
                   bool dry_run);

/**
 * @brief Relink (unlink then link) a single package.
 *
 * @param source_dir   Active source repository directory.
 * @param target_dir   Target destination directory.
 * @param pkg_name     Package name.
 * @param auto_install Auto-confirm missing dependency installation.
 * @param dry_run      Preview mode without modifying disk.
 * @return 0 on success, non-zero on failure.
 */
int relink_package(const char *source_dir,
                   const char *target_dir,
                   const char *pkg_name,
                   bool auto_install,
                   bool dry_run);

/**
 * @brief Deploy symlinks for all packages in source repository.
 *
 * @param source_dir   Active source repository directory.
 * @param target_dir   Target destination directory.
 * @param auto_install Auto-confirm missing dependency installation.
 * @param dry_run      Preview mode without modifying disk.
 */
void link_all_packages(const char *source_dir,
                       const char *target_dir,
                       bool auto_install,
                       bool dry_run);

/**
 * @brief Display deployment status ([LINKED], [PARTIAL], [UNLINKED]) for all packages.
 *
 * @param source_dir Active source repository directory.
 * @param target_dir Target destination directory.
 */
void list_packages_status(const char *source_dir, const char *target_dir);

/* Backwards-compatibility function aliases */
#define get_package_stow_status get_package_link_status
#define is_package_stowed is_package_linked
#define stow_package link_package
#define unstow_package unlink_package
#define restow_package relink_package
#define stow_all_packages link_all_packages

#endif /* SYMDEP_LINKER_H */
