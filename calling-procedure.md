# EDF --pok

### 线程初始化过程

* 设置线程属性taddr
* 调用pok_thread_create()
  * pok_partition_thread_create()
    * 获取当前partition能用的最小线程id,并直接使用此thread id对应数组的位置来作为thread结构体
    * 设置thread priority
    * 设置thread period, next activation = period
    * 设置thread deadline
    * 设置thread time_capacity, remaining time_capacity (不设则为default)
    * 设置stack address, 设置stack pointer
    * 设置thread state = POK_STATE_RUNNABLE
    * 设置wakeup_time = 0
    * 设置thread所属的partition, entry, id
* 调用pok_partition_set_mode
  * 对于当前partition的每个thread
    * 如果不是delayed_start, 就不做处理
    * wakeup_time == 0
      * 设state为POK_STATE_RUNNABLE
      * wakeup_time设置为当前时间
      * endtime为wakeup_time + time_capacity
    * wakeup_time > 0
      * thread->next_activation为下个当前partition frame到来时的wakeup_time(意思是这个frame就只运行main thread了)
      * 设置state和end_time
    * stop main thread
    * 调用pok_sched开启调度
* 调用pok_thread_wait_infinite, 其实就是suspend自己

### 线程调度过程

* pok_sched
  * pok_elect_partition
  * pok_elect_thread
    * 获取当前时间
    * 对于partition的state为POK_STATE_WAITING且thread->wakeup_time小于当前时间的每个线程, 切换其state为POK_STATE_RUNNABLE
    * 对于partition的state为POK_STATE_WAIT_NEXT_ACTIVATION且thread->wakeup_time小于当前时间的每个线程, 切换其state为POK_STATE_RUNNABLE, 重新填充其time_capacity, 使其next_activation加上其period
    * mode为INIT, 则调度main thread
    * mode为NORMAL, 且不为idle和mian thread, 则remaining_time_capacity--, 如果减为0, 则设置state为POK_STATE_WAIT_NEXT_ACTIVATION
    * 调用sched_func
    * 设置要调度的线程的end_time = now + remaining_time_capacity
  * 如果选择的thread和当前的不一样, 则切换线程
  * pok_sched_context_switch

