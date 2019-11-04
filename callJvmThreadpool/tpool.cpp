//
// Created by wanhui on 10/12/19.
//

#include <unistd.h>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include "tpool.h"

static tpool_t *tpool = nullptr;

static void* thread_routine(void *arg)
{
    tpool_work_t *work;

    while(true)
    {
        pthread_mutex_lock(&tpool->queue_lock);
        while(!tpool->queue_head && !tpool->shutdown)
        {
            pthread_cond_wait(&tpool->queue_ready, &tpool->queue_lock);
        }
        if(tpool->shutdown)
        {
            pthread_mutex_unlock(&tpool->queue_lock);
            pthread_exit(nullptr);
        }
        work = tpool->queue_head;
        tpool->queue_head = tpool->queue_head->next;
        pthread_mutex_unlock(&tpool->queue_lock);

        work->routine(work->arg);
        free(work);
    }

    return nullptr;
}

int tpool_create(int max_thr_num)
{
    int i;

    tpool = static_cast<tpool_t *>(calloc(1, sizeof(tpool_t)));
    if(!tpool)
    {
        printf("%s: calloc failed\n", __FUNCTION__);
        exit(1);
    }

    tpool->max_thr_num = max_thr_num;
    tpool->shutdown = 0;
    tpool->queue_head = nullptr;

    if(pthread_mutex_init(&tpool->queue_lock, nullptr) != 0)
    {
        printf("%s: pthread_mutex_init failed, errno:%d, error:%s\n", __FUNCTION__, errno, strerror(errno));
        exit(1);
    }

    if(pthread_cond_init(&tpool->queue_ready, nullptr) != 0)
    {
        printf("%s: pthread_cond_init failed, errno:%d, error:%s\n", __FUNCTION__, errno, strerror(errno));
        exit(1);
    }

    tpool->thr_id = static_cast<pthread_t *>(calloc(max_thr_num, sizeof(pthread_t)));
    if(!tpool->thr_id)
    {
        printf("%s: calloc failed\n", __FUNCTION__);
        exit(1);
    }

    for(i = 0; i < max_thr_num; i++)
    {
        if(pthread_create(&tpool->thr_id[i], nullptr, thread_routine, nullptr) != 0)
        {
            printf("%s: pthread_create failed, errno:%d, error:%s\n", __FUNCTION__, errno, strerror(errno));
            exit(1);
        }
    }

    return 0;
}

void tpool_destroy()
{
    int i;
    tpool_work_t *member;

    if(tpool->shutdown)
    {
        return;
    }
    tpool->shutdown = 1;

    pthread_mutex_lock(&tpool->queue_lock);
    pthread_cond_broadcast(&tpool->queue_ready);
    pthread_mutex_unlock(&tpool->queue_lock);

    for(i = 0; i < tpool->max_thr_num; i++)
    {
        pthread_join(tpool->thr_id[i], nullptr);
    }
    free(tpool->thr_id);

    while (tpool->queue_head)
    {
        member = tpool->queue_head;
        tpool->queue_head = tpool->queue_head->next;
        free(member);
    }

    pthread_mutex_destroy(&tpool->queue_lock);
    pthread_cond_destroy(&tpool->queue_ready);

    free(tpool);
}

int tpool_add_work(void*(*routine)(void*), void* arg)
{
    tpool_work_t *work, *member;

    if(!routine)
    {
        printf("%s: Invalid argument\n", __FUNCTION__);
        return -1;
    }

    work = static_cast<tpool_work_t *>(malloc(sizeof(tpool_work_t)));
    if(!work)
    {
        printf("%s: malloc failed\n", __FUNCTION__);
        return -1;
    }

    work->routine = routine;
    work->arg = arg;
    work->next = nullptr;

    pthread_mutex_lock(&tpool->queue_lock);
    member = tpool->queue_head;
    if(!member)
    {
        tpool->queue_head = work;
    }
    else
    {
        while(member->next)
        {
            member = member->next;
        }
        member->next = work;
    }

    pthread_cond_signal(&tpool->queue_ready);
    pthread_mutex_unlock(&tpool->queue_lock);

    return 0;
}
