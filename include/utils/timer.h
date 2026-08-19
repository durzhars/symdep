/*
 * Symlink & Dependency Manager (symdep)
 * Nanosecond Performance Profiler & Execution Timer Header
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
#ifndef SYMDEP_UTILS_TIMER_H
#define SYMDEP_UTILS_TIMER_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/**
 * @struct PerfTimer
 * @brief High-precision nanosecond execution timer.
 */
typedef struct {
    struct timespec start_ts; /**< Clock start timestamp */
    struct timespec stop_ts;  /**< Clock stop timestamp */
    const char *name;         /**< Timer identifier label */
    bool running;             /**< Whether timer is actively running */
} PerfTimer;

/** Enable or disable execution profiler logging globally */
void perf_profiler_set_enabled(bool enabled);

/** Check if execution profiling is active */
bool perf_profiler_is_enabled(void);

/** Start a high-resolution performance timer with the specified name */
PerfTimer perf_timer_start(const char *name);

/** Stop timer and return elapsed duration in milliseconds */
double perf_timer_stop(PerfTimer *timer);

/** Get elapsed duration in milliseconds */
double perf_timer_elapsed_ms(const PerfTimer *timer);

/** Get elapsed duration in microseconds */
double perf_timer_elapsed_us(const PerfTimer *timer);

/** Log timer duration if execution profiling is active */
void perf_timer_log(const PerfTimer *timer);

/** Forcibly log timer duration regardless of profiler toggle */
void perf_timer_log_force(const PerfTimer *timer);

/** Macro: Begin scoped performance measurement */
#define PERF_PROFILE_START(label) PerfTimer _timer_##label = perf_timer_start(#label)

/** Macro: Conclude scoped performance measurement and log duration */
#define PERF_PROFILE_END(label)           \
    do {                                  \
        perf_timer_stop(&_timer_##label); \
        perf_timer_log(&_timer_##label);  \
    } while (0)

#endif /* SYMDEP_UTILS_TIMER_H */
