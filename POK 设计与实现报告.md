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

### 1.2 抢占式优先级调度

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

### 1.3 EDF调度

**环境设置**

* 设置`POK_CONFIG_NB_PARTITIONS`为1，我们只需要一个分区即可。
*  修改内核时钟中断为每1ms触发一次，`POK_TIMER_QUANTUM`修改为1，即每个tick (1ms)都会出发一次调度。

**实现**

* 由于在原来的pok内核中，并没有用到thread的`deadline`属性，只是在thread的初始化的时候初始化了一下，于是为了使用EDF调度算法，我们需要把这个属性利用起来，同时新增一个属性`deadline_actual`，用于表示每个`period`内绝对的deadline时间。
* 直接使用`time_capacity`来表示每个线程所需要的执行时间，`period`表示每个线程的周期，`deadline`表示每个`period`内的deadline时间，`deadline_actual`表示绝对deadline时间，value为`period开始时间 + deadline`。
* 算法的基本思想，就是在每个tick触发调度时，检查当前partition的每一个thread是否miss了`deadline_actual`，如果miss了，就简单的将其状态设为`POK_STATE_STOPPED`，不再执行，如果一个线程成功地在其`deadline_actual`之前完成了，那么就正常地将其状态设置为`POK_STATE_WAIT_NEXT_ACTIVATION`。然后再次遍历每一个线程，找出其中`deadline_actual`最小的，选择这个线程进行调度，若没有可执行的线程，那么就选择`idle_thread`。

``` c
uint32_t pok_sched_part_edf(const uint32_t index_low, const uint32_t index_high, const uint32_t prev_thread,
                           const uint32_t current_thread) {
		/* 前面的不重要的代码均省略 */
    for (index = index_low; index <= index_high; ++index) {
      	ct = &pok_threads[index];
        if ((ct->deadline_actual > 0 && ct->state == POK_STATE_RUNNABLE)) {
            if (ct->remaining_time_capacity == 0 && ct->deadline_actual >= now) {
                ct->state = POK_STATE_WAIT_NEXT_ACTIVATION;
                printf("thread %d finished sucessfully\n", index);
            }
            if (ct->deadline_actual >= now)
              	continue;
            else {
                ct->state = POK_STATE_STOPPED;
                printf("thread %d miss its deadline, kill this thread\n", index);
            }
        }
    }
    earliest_ddl = -1;
    for (index = index_low; index <= index_high; ++index) {
        ct = &pok_threads[index];
        if (ct->state != POK_STATE_RUNNABLE)
            continue;
        if (ct->deadline_actual < earliest_ddl) {
            earliest_ddl = ct->deadline_actual;
            res = index;
        }
    }
    if (earliest_ddl == (uint64_t)-1)
        res = IDLE_THREAD;
    return res;
}
```

## 2. 动态线程调度

**环境设置**

* 设置`POK_CONFIG_NB_PARTITIONS`为1，我们只需要一个分区即可。
* 修改内核时钟中断为每1ms触发一次，`POK_TIMER_QUANTUM`修改为1，即每个tick (1ms)都会出发一次调度。

**实现**

* 在`pok_thread_t`, `pok_thread_attr_t`这两个结构体中添加了`arrive_time`属性, 表示线程到达的绝对时间。
* 在线程初始化时, 也就是在`pok_partition_thread_create`中初始化此字段。
* 在每次时钟中断触发调度时, 都检查一次所有线程的arrive_time是否小于等于当前时间, 如果小于,则将其状态改为`POK_STATE_RUNNABLE`, 否则, 将其状态改为`POK_STATE_STOPPED`。

```c
for (i = 0; i < new_partition->nthreads; i++) {
        thread = &(pok_threads[new_partition->thread_index_low + i]);
        if (thread->arrive_time > now) {
            thread->state = POK_STATE_STOPPED;
            continue;
        }
        else {
            thread->state = POK_STATE_RUNNABLE;
        }
}
```

## 3. 应用场景简述

### 3.1 抢占式优先级调度场景

**场景设置：**由于实时系统在 cyber physical system 中应用广泛，所以我们设计的应用场景就是现在非常流行的自动驾驶无人机。在这个简化的无人机系统中，一共有三个任务需要并发的运行，分别是：

* 网络收发包线程：负责网络的收发包。这个线程的特点是，运行的周期十分频繁，但是任务执行的时间很短（utilization比较低）。
* 飞行控制线程：负责障碍物的检测、飞行路线的计算任务。这个线程的特点是，运行的周期长，一次运行执行的时间也很长，而且该任务的关键性非常高，即出错的代价非常高。
* 视频数据传输：负责将采集到的视频数据传回地面。这个线程的特点是时间敏感性不高，而且没有成功执行的代价也不高，所以可以在 cpu 空闲时占用cpu。

结合以上三个线程的特点，我们可以分析出以下结论。飞行控制线程的关键性很高，但是一次执行的时间较长，这可能阻塞运行的网络收发包线程。所以我们需要让网络收发包拥有更高的优先级，让它能够抢占飞行控制线程获得 cpu 控制权，但是同时我们也要限制其 time capacity 保证飞行控制线程也能够顺利完成。而视频数据传输作为一个 best effort 线程，给予其 100% 的 cpu 利用率，但是限制它的优先级为最低，这样让它能够在另外两个线程不占用 cpu 时随意地使用 cpu。

**线程属性：**

|                    | time_capacity | period | priority |
| :----------------- | ------------- | ------ | -------- |
| **网络收发包线程** | 1             | 100    | 0        |
| **飞行控制线程**   | 25            | 1000   | 1        |
| **视频数据传输**   | 50            | 1000   | 2        |

**注：**POK的 timer interrupt 已经修改过，现在是 1 ms 触发一次 timer interrupt（1 个 tick），`POK_TIMER_QUANTUM`（20）次 timer interrupt 后触发 `pok_sched`，会给 `thread->remaining_time_capacity` 减 1。也就是说 time_capacity 以20个 tick 为单位。同时 period 是以1个 tick 为单位的。

### 3.2 抢占式EDF调度场景

**场景设置：**带触控板的风扇控制器

* GUI线程：负责与用户的交互。每 15 ms 运行一次，每次运行时间为 5 ms，deadline 为 10 ms。
* 电机控制线程：负责根据用户的输入，实时的设置系统参数（比如转速、定时等）。每 20 ms 运行一次，每次运行时间为 8 ms，deadline 为 15 ms。
* 电机驱动线程：负责驱动风扇的电机运转。每 30 ms 运行一次，每次运行时间为 10 ms，deadline 为 25 ms。

**线程属性**：

|                | Time_capacity | Period | Deadline | Priority |
| -------------- | ------------- | ------ | -------- | -------- |
| 网络收发包线程 | 5             | 15     | 10       | 42       |
| 线程2          | 8             | 20     | 15       | 42       |
| 电机控制线程   | 10            | 30     | 25       | 42       |

**注：**时钟中断为1ms触发一次，每次时钟中断均会触发调度。若一个进程错过了其deadline，就将它kill掉。

