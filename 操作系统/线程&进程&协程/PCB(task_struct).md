linux PCB task_struct

每个进程在内核中都有一个进程控制块(PCB)来维护进程相关的信息,Linux内核的进程控制块是task_struct结构体，task_struct是Linux内核的一种数据结构，它会被装载到RAM里并且包含着进程的信息
每个进程都把它的信息放在 task_struct 这个数据结构里，task_struct 包含了以下内容：
1).标示符 ： 描述本进程的唯一标示符，用来区别其他进程。
2).状态 ：任务状态，退出代码，退出信号等。
3).优先级 ：相对于其他进程的优先级。
4).程序计数器：程序中即将被执行的下一条指令的地址。
5).内存指针：包括程序代码和进程相关数据的指针，还有和其他进程共享的内存块的指针
6).硬件上下文数据：进程执行时处理器的寄存器中的数据。
7).I／O状态信息：包括显示的I/O请求,分配给进程的I／O设备和被进程使用的文件列表。
8).记账信息：可能包括处理器时间总和，使用的时钟数总和，时间限制，记账号等。

剖析task_struct结构体

（1）进程的状态

volatile long state；

state的可能取值为：

```
#define TASK_RUNNING        0//进程要么正在执行，要么准备执行
#define TASK_INTERRUPTIBLE  1 //可中断的睡眠，可以通过一个信号唤醒
#define TASK_UNINTERRUPTIBLE    2 //不可中断睡眠，不可以通过信号进行唤醒
#define __TASK_STOPPED      4 //进程停止执行
#define __TASK_TRACED       8 //进程被追踪
/* in tsk->exit_state */
#define EXIT_ZOMBIE     16 //僵尸状态的进程，表示进程被终止，但是父进程还没有获取它的终止信息，比如进程有没有执行完等信息。
#define EXIT_DEAD       32 //进程的最终状态，进程死亡
/* in tsk->state again */
#define TASK_DEAD       64 //死亡
#define TASK_WAKEKILL       128 //唤醒并杀死的进程
#define TASK_WAKING     256 //唤醒进程
```

（2）进程的唯一标识(pid)
```
pid_t pid;//进程的唯一标识
pid_t tgid;// 线程组的领头线程的pid成员的值
```

在Linux系统中，一个线程组中的所有线程使用和该线程组的领头线程（该组中的第一个轻量级进程）相同的PID，并被存放在tgid成员中。只有线程组的领头线程的pid成员才会被设置为与tgid相同的值。注意，getpid()系统调用返回的是当前进程的tgid值而不是pid值。（线程是程序运行的最小单位，进程是程序运行的基本单位。）

（3）进程的标记:

```
unsigned int flags; //flags成员的可能取值如下

//进程的标志信息
#define PF_ALIGNWARN    0x00000001    /* Print alignment warning msgs */
                    /* Not implemented yet, only for 486*/
#define PF_STARTING    0x00000002    /* being created */
#define PF_EXITING    0x00000004    /* getting shut down */
#define PF_EXITPIDONE    0x00000008    /* pi exit done on shut down */
#define PF_VCPU        0x00000010    /* I'm a virtual CPU */
#define PF_FORKNOEXEC    0x00000040    /* forked but didn't exec */
#define PF_MCE_PROCESS  0x00000080      /* process policy on mce errors */
#define PF_SUPERPRIV    0x00000100    /* used super-user privileges */
#define PF_DUMPCORE    0x00000200    /* dumped core */
#define PF_SIGNALED    0x00000400    /* killed by a signal */
#define PF_MEMALLOC    0x00000800    /* Allocating memory */
#define PF_FLUSHER    0x00001000    /* responsible for disk writeback */
#define PF_USED_MATH    0x00002000    /* if unset the fpu must be initialized before use */
#define PF_FREEZING    0x00004000    /* freeze in progress. do not account to load */
#define PF_NOFREEZE    0x00008000    /* this thread should not be frozen */
#define PF_FROZEN    0x00010000    /* frozen for system suspend */
#define PF_FSTRANS    0x00020000    /* inside a filesystem transaction */
#define PF_KSWAPD    0x00040000    /* I am kswapd */
#define PF_OOM_ORIGIN    0x00080000    /* Allocating much memory to others */
#define PF_LESS_THROTTLE 0x00100000    /* Throttle me less: I clean memory */
#define PF_KTHREAD    0x00200000    /* I am a kernel thread */
#define PF_RANDOMIZE    0x00400000    /* randomize virtual address space */
#define PF_SWAPWRITE    0x00800000    /* Allowed to write to swap */
#define PF_SPREAD_PAGE    0x01000000    /* Spread page cache over cpuset */
#define PF_SPREAD_SLAB    0x02000000    /* Spread some slab caches over cpuset */
#define PF_THREAD_BOUND    0x04000000    /* Thread bound to specific cpu */
#define PF_MCE_EARLY    0x08000000      /* Early kill for mce process policy */
#define PF_MEMPOLICY    0x10000000    /* Non-default NUMA mempolicy */
#define PF_MUTEX_TESTER    0x20000000    /* Thread belongs to the rt mutex tester */
#define PF_FREEZER_SKIP    0x40000000    /* Freezer should not count it as freezeable */
#define PF_FREEZER_NOSIG 0x80000000    /* Freezer won't send signals to it */
```

（4）进程之间的亲属关系：

```
struct task_struct *real_parent; /* real parent process */
struct task_struct *parent; /* recipient of SIGCHLD, wait4() reports */
struct list_head children;    /* list of my children */
struct list_head sibling;    /* linkage in my parent's children list */
struct task_struct *group_leader;    /* threadgroup leader */
```

在Linux系统中，所有进程之间都有着直接或间接地联系，每个进程都有其父进程，也可能有零个或多个子进程。拥有同一父进程的所有进程具有兄弟关系。

real_parent指向其父进程，如果创建它的父进程不再存在，则指向PID为1的init进程。 
parent指向其父进程，当它终止时，必须向它的父进程发送信号。它的值通常与 real_parent相同。 
children表示链表的头部，链表中的所有元素都是它的子进程（进程的子进程链表）。 
sibling用于把当前进程插入到兄弟链表中（进程的兄弟链表）。 
group_leader指向其所在进程组的领头进程。


（5）进程调度信息：

```
int prio, static_prio, normal_prio;
 unsigned int rt_priority;
 const struct sched_class *sched_class;
 struct sched_entity se;
 struct sched_rt_entity rt;
 unsigned int policy;
```

实时优先级范围是0到MAX_RT_PRIO-1（即99），而普通进程的静态优先级范围是从MAX_RT_PRIO到MAX_PRIO-1（即100到139）。值越大静态优先级越低。

static_prio用于保存静态优先级，可以通过nice系统调用来进行修改。 
rt_priority用于保存实时优先级。 
normal_prio的值取决于静态优先级和调度策略(进程的调度策略有：先来先服务，短作业优先、时间片轮转、高响应比优先等等的调度算法) 
prio用于保存动态优先级。 
policy表示进程的调度策略，目前主要有以下五种：

```
#define SCHED_NORMAL        0//按照优先级进行调度（有些地方也说是CFS调度器）
#define SCHED_FIFO        1//先进先出的调度算法
#define SCHED_RR        2//时间片轮转的调度算法
#define SCHED_BATCH        3//用于非交互的处理机消耗型的进程
#define SCHED_IDLE        5//系统负载很低时的调度算法
#define SCHED_RESET_ON_FORK     0x40000000
```

SCHED_NORMAL用于普通进程，通过CFS调度器实现;

SCHED_BATCH用于非交互的处理器消耗型进程;

SCHED_IDLE是在系统负载很低时使用;

SCHED_FIFO（先入先出调度算法）和SCHED_RR（轮流调度算法）都是实时调度策略.

(6)ptrace系统调用

```
unsigned int ptrace;
struct list_head ptraced;
struct list_head ptrace_entry;
unsigned long ptrace_message;
siginfo_t *last_siginfo;      /* For ptrace use.  */
ifdef CONFIG_HAVE_HW_BREAKPOINT
atomic_t ptrace_bp_refcnt;
endif
```

成员ptrace被设置为0时表示不需要被跟踪，它的可能取值如下：
```
/* linux-2.6.38.8/include/linux/ptrace.h */
#define PT_PTRACED  0x00000001
#define PT_DTRACE   0x00000002  /* delayed trace (used on m68k, i386) */
#define PT_TRACESYSGOOD 0x00000004
#define PT_PTRACE_CAP   0x00000008  /* ptracer can follow suid-exec */
#define PT_TRACE_FORK   0x00000010
#define PT_TRACE_VFORK  0x00000020
#define PT_TRACE_CLONE  0x00000040
#define PT_TRACE_EXEC   0x00000080
#define PT_TRACE_VFORK_DONE 0x00000100
#define PT_TRACE_EXIT   0x00000200
```

（7） 时间数据成员

关于时间，一个进程从创建到终止叫做该进程的生存期，进程在其生存期内使用CPU时间，内核都需要进行记录，进程耗费的时间分为两部分，一部分是用户模式下耗费的时间，一部分是在系统模式下耗费的时间.

```
cputime_t utime, stime, utimescaled, stimescaled;
cputime_t gtime;
cputime_t prev_utime, prev_stime;//记录当前的运行时间（用户态和内核态）
unsigned long nvcsw, nivcsw; //自愿/非自愿上下文切换计数
struct timespec start_time;  //进程的开始执行时间
struct timespec real_start_time;  //进程真正的开始执行时间
unsigned long min_flt, maj_flt;
struct task_cputime cputime_expires;//cpu执行的有效时间
struct list_head cpu_timers[3];//用来统计进程或进程组被处理器追踪的时间
struct list_head run_list;
unsigned long timeout;//当前已使用的时间（与开始时间的差值）
unsigned int time_slice;//进程的时间片的大小
int nr_cpus_allowed;
```

（8）信号处理信息

```
struct signal_struct *signal;//指向进程信号描述符
struct sighand_struct *sighand;//指向进程信号处理程序描述符
sigset_t blocked, real_blocked;//阻塞信号的掩码
sigset_t saved_sigmask;    /* restored if set_restore_sigmask() was used */
struct sigpending pending;//进程上还需要处理的信号
unsigned long sas_ss_sp;//信号处理程序备用堆栈的地址
size_t sas_ss_size;//信号处理程序的堆栈的地址
```

（9）文件系统信息

```
/* filesystem information */
struct fs_struct *fs;//文件系统的信息的指针
/* open file information */
struct files_struct *files;//打开文件的信息指针
```

以下是对task_struct的定义及注释：

```
struct task_struct {
	volatile long	state;                  /* 说明了该进程是否可以执行,还是可中断等信息 */
	unsigned long	flags;                  /* Flage 是进程号,在调用fork()时给出 */
	int		sigpending;             /* 进程上是否有待处理的信号 */
	mm_segment_t	addr_limit;             /*
	                                         * 进程地址空间,区分内核进程与普通进程在内存存放的位置不同
	                                         * 0-0xBFFFFFFF for user-thead
	                                         * 0-0xFFFFFFFF for kernel-thread
	                                         */
/* 调度标志,表示该进程是否需要重新调度,若非0,则当从内核态返回到用户态,会发生调度 */
	volatile long	need_resched;
	int		lock_depth;             /* 锁深度 */
	long		nice;                   /* 进程的基本时间片 */
/* 进程的调度策略,有三种,实时进程:SCHED_FIFO,SCHED_RR, 分时进程:SCHED_OTHER */
	unsigned long		policy;
	struct mm_struct	*mm;            /* 进程内存管理信息 */
	int			processor;
/* 若进程不在任何CPU上运行, cpus_runnable 的值是0，否则是1 这个值在运行队列被锁时更新 */
	unsigned long		cpus_runnable, cpus_allowed;
	struct list_head	run_list;       /* 指向运行队列的指针 */
	unsigned long		sleep_time;     /* 进程的睡眠时间 */
/* 用于将系统中所有的进程连成一个双向循环链表, 其根是init_task */
	struct task_struct	*next_task, *prev_task;
	struct mm_struct	*active_mm;
	struct list_head	local_pages;    /* 指向本地页面 */
	unsigned int		allocation_order, nr_local_pages;
	struct linux_binfmt	*binfmt;        /* 进程所运行的可执行文件的格式 */
	int			exit_code, exit_signal;
	int			pdeath_signal;  /* 父进程终止时向子进程发送的信号 */
	unsigned long		personality;
/* Linux可以运行由其他UNIX操作系统生成的符合iBCS2标准的程序 */
	int			did_exec : 1;
	pid_t			pid;            /* 进程标识符,用来代表一个进程 */
	pid_t			pgrp;           /* 进程组标识,表示进程所属的进程组 */
	pid_t			tty_old_pgrp;   /* 进程控制终端所在的组标识 */
	pid_t			session;        /* 进程的会话标识 */
	pid_t			tgid;
	int			leader;         /* 表示进程是否为会话主管 */
	struct task_struct	*p_opptr, *p_pptr, *p_cptr, *p_ysptr, *p_osptr;
	struct list_head	thread_group;   /* 线程链表 */
	struct task_struct	*pidhash_next;  /* 用于将进程链入HASH表 */
	struct task_struct	**pidhash_pprev;
	wait_queue_head_t	wait_chldexit;  /* 供wait4()使用 */
	struct completion	*vfork_done;    /* 供vfork() 使用 */
	unsigned long		rt_priority;    /* 实时优先级，用它计算实时进程调度时的weight值 */

/*
 * it_real_value，it_real_incr用于REAL定时器，单位为jiffies, 系统根据it_real_value
 * 设置定时器的第一个终止时间. 在定时器到期时，向进程发送SIGALRM信号，同时根据
 * it_real_incr重置终止时间，it_prof_value，it_prof_incr用于Profile定时器，单位为jiffies。
 * 当进程运行时，不管在何种状态下，每个tick都使it_prof_value值减一，当减到0时，向进程发送
 * 信号SIGPROF，并根据it_prof_incr重置时间.
 * it_virt_value，it_virt_value用于Virtual定时器，单位为jiffies。当进程运行时，不管在何种
 * 状态下，每个tick都使it_virt_value值减一当减到0时，向进程发送信号SIGVTALRM，根据
 * it_virt_incr重置初值。
 */
	unsigned long		it_real_value, it_prof_value, it_virt_value;
	unsigned long		it_real_incr, it_prof_incr, it_virt_value;
	struct timer_list	real_timer;     /* 指向实时定时器的指针 */
	struct tms		times;          /* 记录进程消耗的时间 */
	unsigned long		start_time;     /* 进程创建的时间 */
/* 记录进程在每个CPU上所消耗的用户态时间和核心态时间 */
	long per_cpu_utime[NR_CPUS], per_cpu_stime[NR_CPUS];
/*
 * 内存缺页和交换信息:
 * min_flt, maj_flt累计进程的次缺页数（Copy on　Write页和匿名页）和主缺页数（从映射文件或交换
 * 设备读入的页面数）； nswap记录进程累计换出的页面数，即写到交换设备上的页面数。
 * cmin_flt, cmaj_flt, cnswap记录本进程为祖先的所有子孙进程的累计次缺页数，主缺页数和换出页面数。
 * 在父进程回收终止的子进程时，父进程会将子进程的这些信息累计到自己结构的这些域中
 */
	unsigned long	min_flt, maj_flt, nswap, cmin_flt, cmaj_flt, cnswap;
	int		swappable : 1; /* 表示进程的虚拟地址空间是否允许换出 */
/*
 * 进程认证信息
 * uid,gid为运行该进程的用户的用户标识符和组标识符，通常是进程创建者的uid，gid
 * euid，egid为有效uid,gid
 * fsuid，fsgid为文件系统uid,gid，这两个ID号通常与有效uid,gid相等，在检查对于文件
 * 系统的访问权限时使用他们。
 * suid，sgid为备份uid,gid
 */
	uid_t	uid, euid, suid, fsuid;
	gid_t	gid, egid, sgid, fsgid;
	int	ngroups;                                /* 记录进程在多少个用户组中 */
	gid_t	groups[NGROUPS];                        /* 记录进程所在的组 */
/* 进程的权能，分别是有效位集合，继承位集合，允许位集合 */
	kernel_cap_t		cap_effective, cap_inheritable, cap_permitted;
	int			keep_capabilities : 1;
	struct user_struct	*user;
	struct rlimit		rlim[RLIM_NLIMITS];     /* 与进程相关的资源限制信息 */
	unsigned short		used_math;              /* 是否使用FPU */
	char			comm[16];               /* 进程正在运行的可执行文件名 */
	/* 文件系统信息 */
	int link_count, total_link_count;
/* NULL if no tty 进程所在的控制终端，如果不需要控制终端，则该指针为空 */
	struct tty_struct	*tty;
	unsigned int		locks;
/* 进程间通信信息 */
	struct sem_undo		*semundo;               /* 进程在信号灯上的所有undo操作 */
	struct sem_queue	*semsleeping;           /* 当进程因为信号灯操作而挂起时，他在该队列中记录等待的操作 */
/* 进程的CPU状态，切换时，要保存到停止进程的task_struct中 */
	struct thread_struct thread;
	/* 文件系统信息 */
	struct fs_struct *fs;
	/* 打开文件信息 */
	struct files_struct *files;
	/* 信号处理函数 */
	spinlock_t		sigmask_lock;
	struct signal_struct	*sig;           /* 信号处理函数 */
	sigset_t		blocked;        /* 进程当前要阻塞的信号，每个信号对应一位 */
	struct sigpending	pending;        /* 进程上是否有待处理的信号 */
	unsigned long		sas_ss_sp;
	size_t			sas_ss_size;
	int			(*notifier)( void *priv );
	void			*notifier_data;
	sigset_t		*notifier_mask;
	u32			parent_exec_id;
	u32			self_exec_id;

	spinlock_t	alloc_lock;
	void		*journal_info;
};
```
