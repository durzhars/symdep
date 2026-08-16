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
#include "utils/thread_pool.h"
#include <stdatomic.h>
#include <unistd.h>

typedef struct {
    atomic_int *counter;
    atomic_int *worker_flags_sum;
} TaskTestCtx;

static void inc_counter_task(void *arg)
{
    TaskTestCtx *ctx = (TaskTestCtx *)arg;
    if (ctx && ctx->counter) {
        atomic_fetch_add(ctx->counter, 1);
    }
    if (ctx && ctx->worker_flags_sum) {
        if (is_in_worker_thread()) {
            atomic_fetch_add(ctx->worker_flags_sum, 1);
        }
    }
}

void test_thread_pool_lifecycle(void)
{
    size_t cores = get_cpu_core_count();
    ASSERT(cores >= 1, "get_cpu_core_count should report at least 1 core");

    ASSERT(!is_in_worker_thread(), "Main test thread should not be flagged as worker thread");

    ThreadPool *pool = thread_pool_create(0);
    ASSERT(pool != NULL, "thread_pool_create(0) should succeed");
    ASSERT(pool->thread_count >= 1 && pool->thread_count <= 4,
           "Thread count for 0 should be bounded between 1 and 4");

    atomic_int counter = 0;
    atomic_int worker_flags = 0;
    TaskTestCtx ctx = {.counter = &counter, .worker_flags_sum = &worker_flags};

    bool added = thread_pool_add_task(pool, inc_counter_task, &ctx);
    ASSERT(added, "thread_pool_add_task should succeed");

    thread_pool_wait(pool);
    ASSERT(atomic_load(&counter) == 1, "Single task should have incremented counter to 1");
    ASSERT(atomic_load(&worker_flags) == 1,
           "Task executed inside pool should report is_in_worker_thread() == true");

    thread_pool_destroy(pool);
}

void test_thread_pool_concurrency(void)
{
    ThreadPool *pool = thread_pool_create(4);
    ASSERT(pool != NULL, "thread_pool_create(4) should succeed");
    ASSERT(pool->thread_count == 4, "Thread pool count should be exactly 4");

    atomic_int counter = 0;
    atomic_int worker_flags = 0;
    TaskTestCtx ctx = {.counter = &counter, .worker_flags_sum = &worker_flags};

    const int BATCH_SIZE_1 = 100;
    for (int i = 0; i < BATCH_SIZE_1; i++) {
        ASSERT(thread_pool_add_task(pool, inc_counter_task, &ctx),
               "thread_pool_add_task should succeed for batch 1");
    }

    thread_pool_wait(pool);
    ASSERT(atomic_load(&counter) == BATCH_SIZE_1,
           "Counter after batch 1 wait should equal BATCH_SIZE_1 (100)");
    ASSERT(atomic_load(&worker_flags) == BATCH_SIZE_1,
           "All 100 tasks should report running in worker thread");

    const int BATCH_SIZE_2 = 50;
    for (int i = 0; i < BATCH_SIZE_2; i++) {
        ASSERT(thread_pool_add_task(pool, inc_counter_task, &ctx),
               "thread_pool_add_task should succeed for batch 2");
    }

    thread_pool_wait(pool);
    ASSERT(atomic_load(&counter) == BATCH_SIZE_1 + BATCH_SIZE_2,
           "Counter after batch 2 wait should equal 150");

    thread_pool_destroy(pool);
}

void test_thread_pool_null_and_error_guards(void)
{
    ASSERT(!thread_pool_add_task(NULL, inc_counter_task, NULL),
           "thread_pool_add_task with NULL pool should return false");

    ThreadPool *pool = thread_pool_create(2);
    ASSERT(pool != NULL, "thread_pool_create(2) should succeed");

    ASSERT(!thread_pool_add_task(pool, NULL, NULL),
           "thread_pool_add_task with NULL function should return false");

    // NULL wait and destroy should not segfault
    thread_pool_wait(NULL);
    thread_pool_destroy(NULL);

    thread_pool_destroy(pool);
}
