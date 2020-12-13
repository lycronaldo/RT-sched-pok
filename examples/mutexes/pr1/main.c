/*
 *                               POK header
 *
 * The following file is a part of the POK project. Any modification should
 * made according to the POK licence. You CANNOT use this file or a part of
 * this file is this part of a file for your own project
 *
 * For more information on the POK licence, please see our LICENCE FILE
 *
 * Please follow the coding guidelines described in doc/CODING_GUIDELINES
 *
 *                                      Copyright (c) 2007-2009 POK team
 *
 * Created by julien on Thu Jan 15 23:34:13 2009
 */

#include <libc/stdio.h>
#include <core/thread.h>
#include <core/partition.h>
#include <types.h>
#include "activity.h"

uint8_t mid;

int main() {
    uint8_t tid;
    pok_ret_t ret;
    pok_thread_attr_t tattr;

    tattr.period = 10;
    tattr.time_capacity = 10;
    tattr.weight = 4;
    tattr.entry = thread1_job;

    ret = pok_thread_create(&tid, &tattr);

    tattr.period = 15;
    tattr.time_capacity = 8;
    tattr.weight = 3;
    tattr.entry = thread2_job;

    ret = pok_thread_create(&tid, &tattr);

    tattr.period = 20;
    tattr.time_capacity = 6;
    tattr.weight = 2;
    tattr.entry = thread3_job;

    ret = pok_thread_create(&tid, &tattr);

    pok_partition_set_mode(POK_PARTITION_MODE_NORMAL);
    pok_thread_wait_infinite();

    return (0);
}
