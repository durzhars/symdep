/*
 * Symlink & Dependency Manager (symdep)
 * Zero-Dependency POSIX Thread Pool Utilities
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

#ifndef SYMDEP_THREAD_POOL_H
#define SYMDEP_THREAD_POOL_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct Task {
    void (*function)(void *arg);
    void *arg;
    struct Task *next;
} Task;

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_cond_t idle;
    pthread_t *threads;
    Task *task_queue_head;
    Task *task_queue_tail;
    size_t thread_count;
    size_t active_tasks;
    size_t queue_size;
    bool stop;
} ThreadPool;

/**
 * @brief Create a new thread pool.
 *
 * @param num_threads Number of worker threads (0 for auto-detect nproc).
 * @return Pointer to ThreadPool or NULL on failure.
 */
ThreadPool *thread_pool_create(size_t num_threads);

/**
 * @brief Add a task work item to the thread pool queue.
 *
 * @param pool ThreadPool pointer.
 * @param function Work function to execute.
 * @param arg Pointer argument passed to work function.
 * @return true on success, false on failure.
 */
bool thread_pool_add_task(ThreadPool *pool, void (*function)(void *arg), void *arg);

/**
 * @brief Wait for all queued and active tasks in the thread pool to finish.
 *
 * @param pool ThreadPool pointer.
 */
void thread_pool_wait(ThreadPool *pool);

/**
 * @brief Destroy thread pool and free resources.
 *
 * @param pool ThreadPool pointer.
 */
void thread_pool_destroy(ThreadPool *pool);

/**
 * @brief Check if caller is executing inside a thread pool worker thread.
 *
 * @return true if called from a worker thread, false otherwise.
 */
bool is_in_worker_thread(void);

/**
 * @brief Query number of available CPU processing cores.
 *
 * @return Number of online CPU cores (fallback to 4 if query fails).
 */
size_t get_cpu_core_count(void);

#endif /* SYMDEP_THREAD_POOL_H */
