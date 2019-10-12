//
// Created by wanhui on 10/12/19.
//

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "tpool.h"

void* func(int arg)
{
    printf("thread %d\n", arg);
    return nullptr;
}

int main()
{
    if(tpool_create(5) != 0)
    {
        printf("tpool_create failed\n");
        exit(1);
    }

    int i;
    for(i = 0; i < 10; i++)
    {
        tpool_add_work(reinterpret_cast<void *(*)(void *)>(func), reinterpret_cast<void *>(i));
    }
    sleep(2);
    tpool_destroy();

    return 0;
}