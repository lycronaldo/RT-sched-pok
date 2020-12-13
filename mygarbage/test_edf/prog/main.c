#include <libc/stdio.h>
#include <libc/string.h>
#include <core/thread.h>
#include <core/partition.h>
#include <core/time.h>
#include <types.h>

static void task1();
static void task2();
static void task3();

#define SCHED_NUM(x) ((x))

int main() {
    uint8_t tid;
    pok_thread_attr_t tattr;
    memset(&tattr, 0, sizeof(pok_thread_attr_t));
    int ret;

    tattr.period = SCHED_NUM(15);
    tattr.deadline = SCHED_NUM(10);
    tattr.time_capacity = 5;
    tattr.priority = 42;
    tattr.entry = task1;
    ret = pok_thread_create(&tid, &tattr);
    printf("[P1] pok_thread_create (1) return=%d\n", ret);

    tattr.period = SCHED_NUM(20);
    tattr.deadline = SCHED_NUM(15);
    tattr.time_capacity = 8;
    tattr.priority = 42;
    tattr.entry = task2;
    ret = pok_thread_create(&tid, &tattr);
    printf("[P1] pok_thread_create (2) return=%d\n", ret);

    tattr.period = SCHED_NUM(30);
    tattr.deadline = SCHED_NUM(25);
    tattr.time_capacity = 10;
    tattr.priority = 42;
    tattr.entry = task3;
    ret = pok_thread_create(&tid, &tattr);
    printf("[P1] pok_thread_create (2) return=%d\n", ret);

    pok_partition_set_mode(POK_PARTITION_MODE_NORMAL);
    pok_thread_wait_infinite();
    return 0;
}

static void __attribute__((optimize("O0"))) make_cpu_busy_for_about(pok_time_t duration) {
    long long a, b;
    for (unsigned i = 0; i < duration; i++) {
        a = 1, b = 1;
        for (unsigned long j = 0; j < 43700; j++) {
            a = a + b;
            b = a + b;
        }
    }
}

static void task1() {
    pok_time_t tick;
    while (1) {
        pok_time_gettick(&tick);
        printf("Task 1 running at %u\n", tick);
        make_cpu_busy_for_about(100);
        // pok_time_gettick(&tick);
        //printf("Task 1: job finish at %u\n", tick);
        // pok_thread_sleep(1000);
    }
}

static void task2() {
    pok_time_t tick;
    while (1) {
        pok_time_gettick(&tick);
        printf("Task 2 running at %u\n", tick);
        make_cpu_busy_for_about(100);
        //pok_time_gettick(&tick);
        //printf("Task 2: job finish at %u\n", tick);
        // pok_thread_sleep(1000);
    }
}

static void task3() {
    pok_time_t tick;
    while (1) {
        pok_time_gettick(&tick);
        printf("Task 3 running at %u\n", tick);
        make_cpu_busy_for_about(100);
        // pok_time_gettick(&tick);
        //printf("Task 3: job finish at %u\n", tick);
        // pok_thread_sleep(1000);
    }
}
