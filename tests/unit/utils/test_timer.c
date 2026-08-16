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
#include "utils/logger.h"
#include "utils/timer.h"
#include <string.h>
#include <time.h>

void test_perf_timer(void)
{
    perf_profiler_set_enabled(true);
    ASSERT(perf_profiler_is_enabled() == true, "Profiler should be enabled");

    PerfTimer timer = perf_timer_start("test_operation");
    ASSERT(timer.running == true, "Timer should be running");

    struct timespec req = {0, 5000000L}; // 5 ms
    nanosleep(&req, NULL);

    double elapsed_ms = perf_timer_stop(&timer);
    ASSERT(timer.running == false, "Timer should be stopped");
    ASSERT(elapsed_ms >= 1.0, "Elapsed ms should be >= 1.0 ms after 5 ms sleep");

    double elapsed_us = perf_timer_elapsed_us(&timer);
    ASSERT(elapsed_us >= 1000.0, "Elapsed us should be >= 1000 us");

    // Idempotent stop: calling perf_timer_stop again returns identical result
    double elapsed_ms_again = perf_timer_stop(&timer);
    ASSERT(elapsed_ms_again == elapsed_ms,
           "Subsequent stop on stopped timer should return same elapsed time");
    ASSERT(timer.running == false, "Timer should remain not running");

    perf_timer_log(&timer);
    perf_profiler_set_enabled(false);
}

void test_perf_timer_edge_cases_and_null_guards(void)
{
    // 1. NULL pointer safety
    ASSERT(perf_timer_stop(NULL) == 0.0, "perf_timer_stop(NULL) should return 0.0");
    ASSERT(perf_timer_elapsed_us(NULL) == 0.0, "perf_timer_elapsed_us(NULL) should return 0.0");
    ASSERT(perf_timer_elapsed_ms(NULL) == 0.0, "perf_timer_elapsed_ms(NULL) should return 0.0");

    // NULL log calls should not crash
    perf_timer_log(NULL);
    perf_timer_log_force(NULL);

    // 2. Start timer with NULL name
    PerfTimer null_name_timer = perf_timer_start(NULL);
    ASSERT(null_name_timer.name != NULL, "Timer name should default to non-NULL string");
    ASSERT_STR_EQ(null_name_timer.name, "unnamed_operation");
    ASSERT(null_name_timer.running == true, "Timer with default name should be running");
    perf_timer_stop(&null_name_timer);

    // 3. Log behavior with profiler enabled vs disabled
    PerfTimer quick_timer = perf_timer_start("quick_op");
    perf_timer_stop(&quick_timer);

    perf_profiler_set_enabled(false);
    ASSERT(!perf_profiler_is_enabled(), "Profiler should report disabled");
    perf_timer_log(&quick_timer); // Silent no-op

    perf_profiler_set_enabled(true);
    ASSERT(perf_profiler_is_enabled(), "Profiler should report enabled");
    perf_timer_log(&quick_timer);

    // 4. Force log
    perf_timer_log_force(&quick_timer);

    perf_profiler_set_enabled(false);
}
