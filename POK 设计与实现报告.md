# POK 设计与实现报告

## 1. 调度算法实现简述

### 1.1 POK 中的调度实现

**分区调度：**

* POK 中的分区调度的实现不太灵活，通过 `deployment.h` 中的宏定义来规定分区运行的顺序和运行的时间。

```c
	#define POK_CONFIG_NB_PARTITIONS 2  // 指定分区数目
	// 分区运行在一个slot当中
	#define POK_CONFIG_SCHEDULING_NBSLOTS 4                             // slot数目
	#define POK_CONFIG_SCHEDULING_SLOTS { 20000, 40000, 10000, 10000 }  // 每个 slot 分配的时间
	#define POK_CONFIG_SCHEDULING_SLOTS_ALLOCATION { 0, 1, 0, 1 }       // 每个 slot 运行哪个分区
  
```

**线程调度：**

* 每个分区有几个线程组成，每个分区中的线程调度算法也是通过 `deployment.h` 中的宏定义规定的。

```c
	#define POK_CONFIG_PARTITIONS_NTHREADS { 3, 2 }  // 第一个分区包括3个线程，第二个分区包括2个线程
	#define POK_CONFIG_PARTITIONS_SCHEDULER  { POK_SCHED_PREEMPTIVE_PRIORITY, POK_SCHED_RR }  // 每个分区使用的线程调度算法
```

* 每 `POK_TIMER_QUANTUM` 次时钟中断会触发一次调度 `pok_sched()`，具体分析如下。
* pok_sched
  * pok_elect_partition
  * pok_elect_thread
    * 获取当前时间
    * 对于partition的state为POK_STATE_WAITING且thread->wakeup_time小于当前时间的每个线程, 切换其state为POK_STATE_RUNNABLE
    * 对于partition的state为POK_STATE_WAIT_NEXT_ACTIVATION且thread->wakeup_time小于当前时间的每个线程, 切换其state为POK_STATE_RUNNABLE, 重新填充其time_capacity, 使其next_activation加上其period
    * **mode**为INIT, 则调度main thread
    * mode为NORMAL, 且不为idle和mian thread, 则remaining_time_capacity--, 如果减为0, 则设置state为POK_STATE_WAIT_NEXT_ACTIVATION
    * 调用sched_func
    * 设置要调度的线程的end_time = now + remaining_time_capacity
  * 如果选择的thread和当前的不一样, 则切换线程
  * pok_sched_context_switch

### 1.1 抢占式优先级调度

**准备工作**

* 在 `schedvalues.h` 中 `enum` 类型的 `pok_sched_t` 中添加 `POK_SCHED_PREEMPTIVE_PRIORITY`.
* 在  `partition.c#pok_partition_setup_scheduler()` 中，如果分区选取的调度策略为 `POK_SCHED_PREEMPTIVE_PRIORITY`, 则把对应分区的 `sched_fun` 设置为`pok_sched_preemptive_priority`.
* 在 `sched.h` 中添加声明，并在 `sched.c` 中给出`pok_sched_preemptive_priority()` 的定义。

**实现**

* 由于是抢占式的优先级调度，所以在调度的实现中即使当前线程的 `remaining_time_capacity` 大于0，也有可能调度到其他线程。
* 大致的实现思路是遍历一边当前分区的所有线程，选取状态为 `POK_STATE_RUNNABLE` 且具有最高优先级的线程进行调度。
* 在我们的实现中 `priority` 数值越低，线程具有的优先级越高。
* 线程的优先级数值的最大值为255，所以先初始化一个 `uint16_t highest_priority = 0x100;`
* 然后遍历分区的各个线程，选取优先级数值最低的线程进行执行。注意，为了避免线程在数组中的顺序前后导致两个优先级相同的线程中靠后的线程可能出现饿死的情况，我们选择当前线程的下一个作为遍历的起点。

```c
 	do {
        tmp++;
        if (tmp > index_high) {
            tmp = index_low;
        }
        if (pok_threads[tmp].state == POK_STATE_RUNNABLE && pok_threads[tmp].priority < highest_priority) {
            res = tmp;
            highest_priority = pok_threads[tmp].priority;
        }
    } while (tmp != from);
```

* 最后，如果没有可运行的线程，则返回 `IDLE_THREAD`

