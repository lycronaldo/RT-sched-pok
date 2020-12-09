# About Pok

**系统结构，调用方式和相关的宏定义**

*系统结构*

pok的partition（分区）和进程的概念类似。而且最基本的执行单位是线程，也就是thread。

使用pok来写一个application需要写自己的makefile，必要的时候需要在misc/mk文件夹下加上自己的.mk文件。.mk文件实质上就是关于编译的一些配置，比如CFLAGS，CPPFLAGS，还有各种gcc的-Werror，-nostdlib之类的flag。

pok是分段的内存管理，partition是直接在内存里划分出一段来直接使用的，所以对于partition是没有虚拟地址这个概念的。但是partition结构体(也就是进程的pcb)有一个base_vaddr的变量，x86平台上在初始化的时候会直接被设为0，这个是给partition内部的线程使用的。也就是对于线程来说有虚拟地址的概念，但是在实际的物理地址上，虚拟地址也是连续的。因为根本没有页表和物理地址的精细化管理。所以这个部分是比较简单的，调试的时候也许可以利用地址偏移非常方便的进行错误信息的打印。

*宏定义*

partition的初始化是在boot期间就做好的。boot期间会把partition的各项参数用宏定义来设好。有以下几个宏比较重要：

POK_NEEDS_THREADS	一般应设为1，thread的开关

POK_NEEDS_PARTITIONS	一般应设为1，partition的开关

POK_NEEDS_SCHED	一般设为1，调度器的开关

POK_NEEDS_GETTICK	一般设为1，时钟的开关

POK_CONFIG_NB_PARTITIONS	按照需要设定（一般2个就够了），partition的数量

POK_CONFIG_NB_THREADS	按照需要设定，这个是pok需要用到的线程数目。一般计算方式是：

​																				pok线程总数 = **Σ** （partition线程数目）+ 2

​													原因在于kernel本身需要一个线程来运行，还有一个线程是用于干什么的不是很清楚。

POK_CONFIG_PARTITIONS_NTHREADS	是一个数组，数组大小和partition的数量一样。数组元素是对应的partition的线程数目。一般来说都要把线程数目设为相等（可能是为了方便，不一样其实也一样的）。

POK_CONFIG_PARTITIONS_SCHEDULER	一个数组，和partition一一对应。数组的元素是pok_sched_t中的一种。pok_sched_t是一个enum类型（在schedvalues.h里定义的），后面实现不同的调度算法要在这里加一个调度类型，在sched.c中实现具体调度算法函数，并在partition.c中的pok_partition_setup_scheduler函数（该函数会在boot的时候被调用）中将具体调度函数与partiton挂钩，就可以使用了。注意此处的调度函数是用于partition调度thread的函数，partition自身的调度与此无关。调度函数会在pok_sched函数调用pok_elect_thread时，具体选择出partition内部的某一个线程进行执行。

POK_CONFIG_SCHEDULING_SLOTS	时间片额度数组，
