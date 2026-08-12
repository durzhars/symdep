/*
 * Symlink & Dependency Manager (symdep)
 * Zero-Dependency POSIX Thread Pool Utilities Implementation
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
#define _POSIX_C_SOURCE 200809L

#include "utils/thread_pool.h"
#include "utils/mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

size_t get_cpu_core_count(void)
{
#if defined(_SC_NPROCESSORS_ONLN)
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    if (nprocs > 0) {
        return (size_t)nprocs;
    }
#endif
    return 4;
}

static _Thread_local bool g_in_worker_thread = false;

bool is_in_worker_thread(void)
{
    return g_in_worker_thread;
}

static void *worker_thread_routine(void *arg)
{
    g_in_worker_thread = true;
    ThreadPool *pool = (ThreadPool *)arg;

    while (1) {
        pthread_mutex_lock(&pool->lock);

        while (pool->task_queue_head == NULL && !pool->stop) {
            pthread_cond_wait(&pool->notify, &pool->lock);
        }

        if (pool->stop && pool->task_queue_head == NULL) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        Task *task = pool->task_queue_head;
        if (task) {
            pool->task_queue_head = task->next;
            if (pool->task_queue_head == NULL) {
                pool->task_queue_tail = NULL;
            }
            pool->queue_size--;
            pool->active_tasks++;
        }

        pthread_mutex_unlock(&pool->lock);

        if (task) {
            if (task->function) {
                task->function(task->arg);
            }
            free(task);

            pthread_mutex_lock(&pool->lock);
            pool->active_tasks--;
            if (pool->active_tasks == 0 && pool->queue_size == 0) {
                pthread_cond_broadcast(&pool->idle);
            }
            pthread_mutex_unlock(&pool->lock);
        }
    }

    return NULL;
}

ThreadPool *thread_pool_create(size_t num_threads)
{
    if (num_threads == 0) {
        num_threads = get_cpu_core_count();
        if (num_threads > 4) {
            num_threads = 4;
        }
    }

    ThreadPool *pool = (ThreadPool *)safe_malloc(sizeof(ThreadPool));
    pool->thread_count = num_threads;
    pool->active_tasks = 0;
    pool->queue_size = 0;
    pool->task_queue_head = NULL;
    pool->task_queue_tail = NULL;
    pool->stop = false;

    if (pthread_mutex_init(&pool->lock, NULL) != 0) {
        free(pool);
        return NULL;
    }

    if (pthread_cond_init(&pool->notify, NULL) != 0) {
        pthread_mutex_destroy(&pool->lock);
        free(pool);
        return NULL;
    }

    if (pthread_cond_init(&pool->idle, NULL) != 0) {
        pthread_cond_destroy(&pool->notify);
        pthread_mutex_destroy(&pool->lock);
        free(pool);
        return NULL;
    }

    pool->threads = (pthread_t *)safe_calloc(num_threads, sizeof(pthread_t));

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 512 * 1024);

    for (size_t i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->threads[i], &attr, worker_thread_routine, pool) != 0) {
            pthread_attr_destroy(&attr);
            thread_pool_destroy(pool);
            return NULL;
        }
    }
    pthread_attr_destroy(&attr);

    return pool;
}

bool thread_pool_add_task(ThreadPool *pool, void (*function)(void *arg), void *arg)
{
    if (!pool || !function) {
        return false;
    }

    Task *task = (Task *)safe_malloc(sizeof(Task));
    task->function = function;
    task->arg = arg;
    task->next = NULL;

    pthread_mutex_lock(&pool->lock);

    if (pool->stop) {
        pthread_mutex_unlock(&pool->lock);
        free(task);
        return false;
    }

    if (pool->task_queue_tail) {
        pool->task_queue_tail->next = task;
        pool->task_queue_tail = task;
    } else {
        pool->task_queue_head = task;
        pool->task_queue_tail = task;
    }

    pool->queue_size++;
    pthread_cond_signal(&pool->notify);
    pthread_mutex_unlock(&pool->lock);

    return true;
}

void thread_pool_wait(ThreadPool *pool)
{
    if (!pool) {
        return;
    }

    pthread_mutex_lock(&pool->lock);
    while (pool->queue_size > 0 || pool->active_tasks > 0) {
        pthread_cond_wait(&pool->idle, &pool->lock);
    }
    pthread_mutex_unlock(&pool->lock);
}

void thread_pool_destroy(ThreadPool *pool)
{
    if (!pool) {
        return;
    }

    pthread_mutex_lock(&pool->lock);
    pool->stop = true;
    pthread_cond_broadcast(&pool->notify);
    pthread_mutex_unlock(&pool->lock);

    for (size_t i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    pthread_mutex_lock(&pool->lock);
    Task *curr = pool->task_queue_head;
    while (curr) {
        Task *next = curr->next;
        free(curr);
        curr = next;
    }
    pthread_mutex_unlock(&pool->lock);

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);
    pthread_cond_destroy(&pool->idle);
    free(pool->threads);
    free(pool);
}
