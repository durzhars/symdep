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

    perf_timer_log(&timer);
    perf_profiler_set_enabled(false);
}
