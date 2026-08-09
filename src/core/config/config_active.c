/*
 * Symlink & Dependency Manager (symdep)
 * Active Runtime Source & Target Path Resolution Submodule
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

#include "core/config/internal.h"

const char *getenv_first(const char *const names[], size_t count)
{
    for (size_t i = 0; i < count; i++) {
        const char *val = getenv(names[i]);
        if (val && *val != '\0') {
            return val;
        }
    }
    return NULL;
}

void get_active_source_dir(const char *cli_override, char *buf, size_t buf_size)
{
    if (cli_override && strlen(cli_override) > 0) {
        expand_tilde_path(cli_override, buf, buf_size);
        normalize_path(buf);
    } else {
        static const char *const env_src_vars[] = {
            "SYMDEP_SOURCE_DIR", "SOURCE_DIR", "STOW_DOTFILES_DIR", "DOTFILES_DIR"
        };
        const char *env_dir = getenv_first(env_src_vars, sizeof(env_src_vars) / sizeof(env_src_vars[0]));

        if (env_dir) {
            expand_tilde_path(env_dir, buf, buf_size);
            normalize_path(buf);
        } else {
            char cwd[STOW_PATH_LARGE];
            if (getcwd(cwd, sizeof(cwd))) {
                const char *candidates[] = {
                    "symdep.registry", ".symdepregistry", "stow.registry", ".stowregistry"
                };
                char test_reg[STOW_PATH_LARGE];
                for (size_t i = 0; i < 4; i++) {
                    join_path(test_reg, sizeof(test_reg), cwd, candidates[i]);
                    if (file_exists(test_reg)) {
                        snprintf(buf, buf_size, "%s", cwd);
                        goto validate;
                    }
                }
            }

            Config cfg;
            config_load_active(&cfg);
            if (cfg.source_dirs.count > 0) {
                snprintf(buf, buf_size, "%s", cfg.source_dirs.items[0]);
                config_free(&cfg);
            } else {
                config_free(&cfg);
                get_dotfiles_dir(buf, buf_size);
            }
        }
    }

validate:;
    PathSanityResult sanity = verify_path_sanity(buf);
    if (sanity != PATH_VALID) {
        log_error("Fatal: Source directory error: %s. Exiting.", path_sanity_strerror(sanity, buf));
        exit(EXIT_FAILURE);
    }
}

void get_active_target_dir(const char *cli_override, char *buf, size_t buf_size)
{
    if (cli_override && strlen(cli_override) > 0) {
        expand_tilde_path(cli_override, buf, buf_size);
        normalize_path(buf);
    } else {
        static const char *const env_tgt_vars[] = {
            "SYMDEP_TARGET_DIR", "STOW_TARGET_DIR", "TARGET_DIR"
        };
        const char *env_target = getenv_first(env_tgt_vars, sizeof(env_tgt_vars) / sizeof(env_tgt_vars[0]));

        if (env_target) {
            expand_tilde_path(env_target, buf, buf_size);
            normalize_path(buf);
        } else {
            Config cfg;
            config_load_active(&cfg);
            if (cfg.target_dir[0] != '\0') {
                snprintf(buf, buf_size, "%s", cfg.target_dir);
                config_free(&cfg);
            } else {
                config_free(&cfg);

                const char *env_home = getenv("HOME");
                if (!env_home || *env_home == '\0') {
                    log_error("Fatal: $HOME environment variable is not set");
                    log_info("Hint: Set $HOME or configure a target directory via 'symdep config set target <path>'.");
                    exit(EXIT_FAILURE);
                }
                snprintf(buf, buf_size, "%s", env_home);
            }
        }
    }

    PathSanityResult sanity = verify_path_sanity(buf);
    if (sanity != PATH_VALID) {
        log_error("Fatal: Target directory error: %s. Exiting.", path_sanity_strerror(sanity, buf));
        exit(EXIT_FAILURE);
    }
}

void get_active_config_dirs(const char *cli_source_override,
                            const char *cli_target_override,
                            char *src_buf,
                            size_t src_size,
                            char *tgt_buf,
                            size_t tgt_size)
{
    Config cfg;
    bool cfg_loaded = false;

    // 1. Resolve Source Dir
    if (cli_source_override && strlen(cli_source_override) > 0) {
        expand_tilde_path(cli_source_override, src_buf, src_size);
        normalize_path(src_buf);
    } else {
        static const char *const env_src_vars[] = {
            "SYMDEP_SOURCE_DIR", "SOURCE_DIR", "STOW_DOTFILES_DIR", "DOTFILES_DIR"
        };
        const char *env_dir = getenv_first(env_src_vars, 4);

        if (env_dir) {
            expand_tilde_path(env_dir, src_buf, src_size);
            normalize_path(src_buf);
        } else {
            char cwd[STOW_PATH_LARGE];
            bool found_reg = false;
            if (getcwd(cwd, sizeof(cwd))) {
                const char *candidates[] = {
                    "symdep.registry", ".symdepregistry", "stow.registry", ".stowregistry"
                };
                char test_reg[STOW_PATH_LARGE];
                for (size_t i = 0; i < 4; i++) {
                    join_path(test_reg, sizeof(test_reg), cwd, candidates[i]);
                    if (file_exists(test_reg)) {
                        snprintf(src_buf, src_size, "%s", cwd);
                        found_reg = true;
                        break;
                    }
                }
            }
            if (!found_reg) {
                if (!cfg_loaded) {
                    config_load_active(&cfg);
                    cfg_loaded = true;
                }
                if (cfg.source_dirs.count > 0) {
                    snprintf(src_buf, src_size, "%s", cfg.source_dirs.items[0]);
                } else {
                    get_dotfiles_dir(src_buf, src_size);
                }
            }
        }
    }

    PathSanityResult src_sanity = verify_path_sanity(src_buf);
    if (src_sanity != PATH_VALID) {
        if (cfg_loaded) config_free(&cfg);
        log_error("Fatal: Source directory error: %s. Exiting.", path_sanity_strerror(src_sanity, src_buf));
        exit(EXIT_FAILURE);
    }

    // 2. Resolve Target Dir
    if (cli_target_override && strlen(cli_target_override) > 0) {
        expand_tilde_path(cli_target_override, tgt_buf, tgt_size);
        normalize_path(tgt_buf);
    } else {
        static const char *const env_tgt_vars[] = {
            "SYMDEP_TARGET_DIR", "STOW_TARGET_DIR", "TARGET_DIR"
        };
        const char *env_target = getenv_first(env_tgt_vars, 3);

        if (env_target) {
            expand_tilde_path(env_target, tgt_buf, tgt_size);
            normalize_path(tgt_buf);
        } else {
            if (!cfg_loaded) {
                config_load_active(&cfg);
                cfg_loaded = true;
            }
            if (cfg.target_dir[0] != '\0') {
                snprintf(tgt_buf, tgt_size, "%s", cfg.target_dir);
            } else {
                const char *env_home = getenv("HOME");
                if (!env_home || *env_home == '\0') {
                    if (cfg_loaded) config_free(&cfg);
                    log_error("Fatal: $HOME environment variable is not set");
                    exit(EXIT_FAILURE);
                }
                snprintf(tgt_buf, tgt_size, "%s", env_home);
            }
        }
    }

    if (cfg_loaded) {
        config_free(&cfg);
    }

    PathSanityResult tgt_sanity = verify_path_sanity(tgt_buf);
    if (tgt_sanity != PATH_VALID) {
        log_error("Fatal: Target directory error: %s. Exiting.", path_sanity_strerror(tgt_sanity, tgt_buf));
        exit(EXIT_FAILURE);
    }
}

void get_active_target_dir_for_pkg(const char *cli_override,
                                   const char *source_dir,
                                   const char *pkg_name,
                                   char *buf,
                                   size_t buf_size)
{
    if (cli_override && strlen(cli_override) > 0) {
        expand_tilde_path(cli_override, buf, buf_size);
        normalize_path(buf);
        PathSanityResult sanity = verify_path_sanity(buf);
        if (sanity != PATH_VALID) {
            log_error("Fatal: Override target directory invalid: %s. Exiting.",
                      path_sanity_strerror(sanity, buf));
            exit(EXIT_FAILURE);
        }
        return;
    }

    if (source_dir && pkg_name && strlen(pkg_name) > 0) {
        PackageManifest manifest;
        manifest_init(&manifest, pkg_name);
        if (manifest_load(&manifest, source_dir) && manifest.target_path &&
            strlen(manifest.target_path) > 0) {
            expand_tilde_path(manifest.target_path, buf, buf_size);
            normalize_path(buf);
            manifest_free(&manifest);

            PathSanityResult sanity = verify_path_sanity(buf);
            if (sanity != PATH_VALID) {
                log_error("Fatal: Package manifest target directory invalid: %s. Exiting.",
                          path_sanity_strerror(sanity, buf));
                exit(EXIT_FAILURE);
            }
            return;
        }
        manifest_free(&manifest);
    }

    get_active_target_dir(NULL, buf, buf_size);
}
