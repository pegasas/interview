# linux cgroup

CGroup 介绍
CGroup 是 Control Groups 的缩写，是 Linux 内核提供的一种可以限制、记录、隔离进程组 (process groups) 所使用的物力资源 (如 cpu memory i/o 等等) 的机制。2007 年进入 Linux 2.6.24 内核，CGroups 不是全新创造的，它将进程管理从 cpuset 中剥离出来，作者是 Google 的 Paul Menage。CGroups 也是 LXC 为实现虚拟化所使用的资源管理手段。

CGroup 功能及组成
CGroup 是将任意进程进行分组化管理的 Linux 内核功能。CGroup 本身是提供将进程进行分组化管理的功能和接口的基础结构，I/O 或内存的分配控制等具体的资源管理功能是通过这个功能来实现的。这些具体的资源管理功能称为 CGroup 子系统或控制器。CGroup 子系统有控制内存的 Memory 控制器、控制进程调度的 CPU 控制器等。运行中的内核可以使用的 Cgroup 子系统由/proc/cgroup 来确认。
CGroup 提供了一个 CGroup 虚拟文件系统，作为进行分组管理和各子系统设置的用户接口。要使用 CGroup，必须挂载 CGroup 文件系统。这时通过挂载选项指定使用哪个子系统。

Cgroups提供了以下功能：
1）限制进程组可以使用的资源数量（Resource limiting ）。比如：memory子系统可以为进程组设定一个memory使用上限，一旦进程组使用的内存达到限额再申请内存，就会出发OOM（out of memory）。
2）进程组的优先级控制（Prioritization ）。比如：可以使用cpu子系统为某个进程组分配特定cpu share。
3）记录进程组使用的资源数量（Accounting ）。比如：可以使用cpuacct子系统记录某个进程组使用的cpu时间
4）进程组隔离（Isolation）。比如：使用ns子系统可以使不同的进程组使用不同的namespace，以达到隔离的目的，不同的进程组有各自的进程、网络、文件系统挂载空间。
5）进程组控制（Control）。比如：使用freezer子系统可以将进程组挂起和恢复。

CGroup 支持的文件种类
表 1. CGroup 支持的文件种类

文件名	R/W	用途
Release_agent

RW

删除分组时执行的命令，这个文件只存在于根分组

Notify_on_release

RW

设置是否执行 release_agent。为 1 时执行

Tasks

RW

属于分组的线程 TID 列表

Cgroup.procs

R

属于分组的进程 PID 列表。仅包括多线程进程的线程 leader 的 TID，这点与 tasks 不同

Cgroup.event_control

RW

监视状态变化和分组删除事件的配置文件

CGroup 相关概念解释
1）任务（task）。在 cgroups 中，任务就是系统的一个进程；
2）控制族群（control group）。控制族群就是一组按照某种标准划分的进程。Cgroups 中的资源控制都是以控制族群为单位实现。一个进程可以加入到某个控制族群，也从一个进程组迁移到另一个控制族群。一个进程组的进程可以使用 cgroups 以控制族群为单位分配的资源，同时受到 cgroups 以控制族群为单位设定的限制；
3）层级（hierarchy）。控制族群可以组织成 hierarchical 的形式，既一颗控制族群树。控制族群树上的子节点控制族群是父节点控制族群的孩子，继承父控制族群的特定的属性；
4）子系统（subsystem）。一个子系统就是一个资源控制器，比如 cpu 子系统就是控制 cpu 时间分配的一个控制器。子系统必须附加（attach）到一个层级上才能起作用，一个子系统附加到某个层级以后，这个层级上的所有控制族群都受到这个子系统的控制。

相互关系
1）每次在系统中创建新层级时，该系统中的所有任务都是那个层级的默认 cgroup（我们称之为 root cgroup，此 cgroup 在创建层级时自动创建，后面在该层级中创建的 cgroup 都是此 cgroup 的后代）的初始成员；
2）一个子系统最多只能附加到一个层级；
3）一个层级可以附加多个子系统；
4）一个任务可以是多个 cgroup 的成员，但是这些 cgroup 必须在不同的层级；
5）系统中的进程（任务）创建子进程（任务）时，该子任务自动成为其父进程所在 cgroup 的成员。然后可根据需要将该子任务移动到不同的 cgroup 中，但开始时它总是继承其父任务的 cgroup。

图 1. CGroup 层级图


图 1 所示的 CGroup 层级关系显示，CPU 和 Memory 两个子系统有自己独立的层级系统，而又通过 Task Group 取得关联关系。

CGroup 特点
在 cgroups 中，任务就是系统的一个进程。
控制族群（control group）。控制族群就是一组按照某种标准划分的进程。Cgroups 中的资源控制都是以控制族群为单位实现。一个进程可以加入到某个控制族群，也从一个进程组迁移到另一个控制族群。一个进程组的进程可以使用 cgroups 以控制族群为单位分配的资源，同时受到 cgroups 以控制族群为单位设定的限制。
层级（hierarchy）。控制族群可以组织成 hierarchical 的形式，既一颗控制族群树。控制族群树上的子节点控制族群是父节点控制族群的孩子，继承父控制族群的特定的属性。
子系统（subsytem）。一个子系统就是一个资源控制器，比如 cpu 子系统就是控制 cpu 时间分配的一个控制器。子系统必须附加（attach）到一个层级上才能起作用，一个子系统附加到某个层级以后，这个层级上的所有控制族群都受到这个子系统的控制。

子系统的介绍
blkio -- 这个子系统为块设备设定输入/输出限制，比如物理设备（磁盘，固态硬盘，USB 等等）。
cpu -- 这个子系统使用调度程序提供对 CPU 的 cgroup 任务访问。
cpuacct -- 这个子系统自动生成 cgroup 中任务所使用的 CPU 报告。
cpuset -- 这个子系统为 cgroup 中的任务分配独立 CPU（在多核系统）和内存节点。
devices -- 这个子系统可允许或者拒绝 cgroup 中的任务访问设备。
freezer -- 这个子系统挂起或者恢复 cgroup 中的任务。
memory -- 这个子系统设定 cgroup 中任务使用的内存限制，并自动生成由那些任务使用的内存资源报告。
net_cls -- 这个子系统使用等级识别符（classid）标记网络数据包，可允许 Linux 流量控制程序（tc）识别从具体 cgroup 中生成的数据包。

图 2. CGroup 典型应用架构图


如图 2 所示，CGroup 技术可以被用来在操作系统底层限制物理资源，起到 Container 的作用。图中每一个 JVM 进程对应一个 Container Cgroup 层级，通过 CGroup 提供的各类子系统，可以对每一个 JVM 进程对应的线程级别进行物理限制，这些限制包括 CPU、内存等等许多种类的资源。下一部分会具体对应用程序进行 CPU 资源隔离进行演示。



前置：这里使用的linux版本是4.8，x86体系。

cgroup_init_early();
聊这个函数就需要先了解cgroup。

cgroup概念
这个函数就是初始化cgroup所需要的参数的。cgroup最初是在2006年由google的一名工程师提出的，目的是把一些共同目标的进程放在一个组里面，而这个组里面的进程能共享指定数额的资源。而后就有了cgroup这个概念了。

我们把每种资源叫做子系统，比如CPU子系统，内存子系统。为什么叫做子系统呢，因为它是从整个操作系统的资源衍生出来的。然后我们创建一种虚拟的节点，叫做cgroup，然后这个虚拟节点可以扩展，以树形的结构，有root节点，和子节点。这个父节点和各个子节点就形成了层级（hierarchiy）。每个层级都可以附带继承一个或者多个子系统，就意味着，我们把资源按照分割到多个层级系统中，层级系统中的每个节点对这个资源的占比各有不同。

下面我们想法子把进程分组，进程分组的逻辑叫做css_set。这里的css是cgroup_subsys_state的缩写。所以css_set和进程的关系是一对多的关系。另外，在cgroup眼中，进程请不要叫做进程，叫做task。这个可能是为了和内核中进程的名词区分开吧。

进程分组css_set，不同层级中的节点cgroup也都有了。那么，就要把节点cgroup和层级进行关联，和数据库中关系表一样。这个事一个多对多的关系。为什么呢？首先，一个节点可以隶属于多个css_set，这就代表这这批css_set中的进程都拥有这个cgroup所代表的资源。其次，一个css_set需要多个cgroup。因为一个层级的cgroup只代表一种或者几种资源，而一般进程是需要多种资源的集合体。

美团的这个图片描写的非常清晰，一看就了解了：



task_struct
首先先看进程的结构，里面和cgroup有关的是

#ifdef CONFIG_CGROUPS
     // 设置这个进程属于哪个css_set
    struct css_set __rcu *cgroups;
     // cg_list是用于将所有同属于一个css_set的task连成一起
    struct list_head cg_list;
#endif
我们会在代码中经常见到list_head。它其实就是表示，这个在链表中存在。

struct list_head {
    struct list_head *next, *prev;
};
它的结构很简单，就能把某种相同性质的结构连成一个链表，根据这个链表我能前后找全整个链表或者头部节点等。

css_set
结构体在include/linux/cgroup-defs.h中。

struct css_set {
    // 引用计数，gc使用，如果子系统有引用到这个css_set,则计数＋1
    atomic_t refcount;

     // TODO: 列出有相同hash值的cgroup（还不清楚为什么）
    struct hlist_node hlist;

     // 将所有的task连起来。mg_tasks代表迁移的任务
    struct list_head tasks;
    struct list_head mg_tasks;
     // 将这个css_set对应的cgroup连起来
    struct list_head cgrp_links;

    // 默认连接的cgroup
    struct cgroup *dfl_cgrp;

     // 包含一系列的css(cgroup_subsys_state)，css就是子系统，这个就代表了css_set和子系统的多对多的其中一面
    struct cgroup_subsys_state *subsys[CGROUP_SUBSYS_COUNT];

    // 内存迁移的时候产生的系列数据
    struct list_head mg_preload_node;
    struct list_head mg_node;
    struct cgroup *mg_src_cgrp;
    struct cgroup *mg_dst_cgrp;
    struct css_set *mg_dst_cset;

    // 把->subsys[ssid]->cgroup->e_csets[ssid]结构展平放在这里，提高迭代效率
    struct list_head e_cset_node[CGROUP_SUBSYS_COUNT];

     // 所有迭代任务的列表，这个补丁参考:https://patchwork.kernel.org/patch/7368941/
    struct list_head task_iters;

    // 这个css_set是否已经无效了
    bool dead;

    // rcu锁所需要的callback等信息
    struct rcu_head rcu_head;
};
这里说一下rcu锁，这个锁是linux2.6引入的。它是非常高效的，适合读多写少的情况。全称是（Read-Copy Update）读－拷贝修改。原理就是读操作的时候，不需要任何锁，直接进行读取，写操作的时候，先拷贝一个副本，然后对副本进行修改，最后使用回调（callback）在适当的时候把指向原来数据的指针指向新的被修改的数据。https://www.ibm.com/developerworks/cn/linux/l-rcu/

这里的rcu_head就存储了对这个结构上rcu锁所需要的回调信息。

struct callback_head {
    struct callback_head *next;
    void (*func)(struct callback_head *head);
} __attribute__((aligned(sizeof(void *))));
#define rcu_head callback_head
回到css_set，其实最重要的就是cgroup_subsys_state subsys[]数组这个结构。

cgroup_subsys_state 和 cgroup_subsys
这个结构最重要的就是存储的进程与特定子系统相关的信息。通过它，可以将task_struct和cgroup连接起来了：task_struct->css_set->cgroup_subsys_state->cgroup

struct cgroup_subsys_state {
    // 对应的cgroup
    struct cgroup *cgroup;

    // 子系统
    struct cgroup_subsys *ss;

    // 带cpu信息的引用计数（不大理解）
    struct percpu_ref refcnt;

    // 父css
    struct cgroup_subsys_state *parent;

    // 兄弟和孩子链表串
    struct list_head sibling;
    struct list_head children;

    // css的唯一id
    int id;

     // 可设置的flag有：CSS_NO_REF/CSS_ONLINE/CSS_RELEASED/CSS_VISIBLE
    unsigned int flags;

    // 为了保证遍历的顺序性，设置遍历按照这个字段的升序走
    u64 serial_nr;

    // 计数，计算本身css和子css的活跃数，当这个数大于1，说明还有有效子css
    atomic_t online_cnt;

    // TODO: 带cpu信息的引用计数使用的rcu锁（不大理解）
    struct rcu_head rcu_head;
    struct work_struct destroy_work;
};
cgroup_subsys结构体在include/linux/cgroup-defs.h里面

struct cgroup_subsys {
     // 下面的是函数指针，定义了子系统对css_set结构的系列操作
    struct cgroup_subsys_state *(*css_alloc)(struct cgroup_subsys_state *parent_css);
    int (*css_online)(struct cgroup_subsys_state *css);
    void (*css_offline)(struct cgroup_subsys_state *css);
    void (*css_released)(struct cgroup_subsys_state *css);
    void (*css_free)(struct cgroup_subsys_state *css);
    void (*css_reset)(struct cgroup_subsys_state *css);

     // 这些函数指针表示了对子系统对进程task的一系列操作
    int (*can_attach)(struct cgroup_taskset *tset);
    void (*cancel_attach)(struct cgroup_taskset *tset);
    void (*attach)(struct cgroup_taskset *tset);
    void (*post_attach)(void);
    int (*can_fork)(struct task_struct *task);
    void (*cancel_fork)(struct task_struct *task);
    void (*fork)(struct task_struct *task);
    void (*exit)(struct task_struct *task);
    void (*free)(struct task_struct *task);

    void (*bind)(struct cgroup_subsys_state *root_css);

     // 是否在前期初始化了
    bool early_init:1;

     // 如果设置了true，那么在cgroup.controllers和cgroup.subtree_control就不会显示, TODO:
    bool implicit_on_dfl:1;

     // 如果设置为false，则子cgroup会继承父cgroup的子系统资源，否则不继承或者只继承一半
     // 但是现在，我们规定，不允许一个cgroup有不可继承子系统仍然可以衍生出cgroup。如果做类似操作，我们会根据
     // warned_broken_hierarch出现错误提示。
    bool broken_hierarchy:1;
    bool warned_broken_hierarchy:1;

    int id;
    const char *name;

     // 如果子cgroup的结构继承子系统的时候没有设置name，就会沿用父系统的子系统名字，所以这里存的就是父cgroup的子系统名字
    const char *legacy_name;

    struct cgroup_root *root;  // 这个就是子系统指向的层级中的root的cgroup

    struct idr css_idr; // 对应的css的idr

     // 对应的文件系统相关信息
    struct list_head cfts;
    struct cftype *dfl_cftypes;    /* 默认的文件系统 */
    struct cftype *legacy_cftypes;    /* 继承的文件系统 */

     // 有的子系统是依赖其他子系统的，这里是一个掩码来表示这个子系统依赖哪些子系统
    unsigned int depends_on;
};
这里特别说一下cftype。它是cgroup_filesystem_type的缩写。这个要从我们的linux虚拟文件系统说起（VFS）。VFS封装了标准文件的所有系统调用。那么我们使用cgroup，也抽象出了一个文件系统，自然也需要实现这个VFS。实现这个VFS就是使用这个cftype结构。

这里说一下idr。这个是linux的整数id管理机制。你可以把它看成一个map，这个map是把id和制定指针关联在一起的机制。它的原理是使用基数树。一个结构存储了一个idr，就能很方便根据id找出这个id对应的结构的地址了。http://blog.csdn.net/dlutbrucezhang/article/details/10103371

cgroup
cgroup结构也在相同文件，但是cgroup_root和子节点cgroup是使用两个不同结构表示的。

struct cgroup {
    // cgroup所在css
    struct cgroup_subsys_state self;

    unsigned long flags;

    int id;

    // 这个cgroup所在层级中，当前cgroup的深度
    int level;

    // 每当有个非空的css_set和这个cgroup关联的时候，就增加计数1
    int populated_cnt;

    struct kernfs_node *kn;        /* cgroup kernfs entry */
    struct cgroup_file procs_file;    /* handle for "cgroup.procs" */
    struct cgroup_file events_file;    /* handle for "cgroup.events" */

    // TODO: 不理解
    u16 subtree_control;
    u16 subtree_ss_mask;
    u16 old_subtree_control;
    u16 old_subtree_ss_mask;

    // 一个cgroup属于多个css，这里就是保存了cgroup和css直接多对多关系的另一半
    struct cgroup_subsys_state __rcu *subsys[CGROUP_SUBSYS_COUNT];

     // 根cgroup
    struct cgroup_root *root;

    // 相同css_set的cgroup链表
    struct list_head cset_links;

    // 这个cgroup使用的所有子系统的每个链表
    struct list_head e_csets[CGROUP_SUBSYS_COUNT];

    // TODO: 不理解
    struct list_head pidlists;
    struct mutex pidlist_mutex;

    // 用来保存下线task
    wait_queue_head_t offline_waitq;

    // TODO: 用来保存释放任务？(不理解)
    struct work_struct release_agent_work;

    // 保存每个level的祖先
    int ancestor_ids[];
};
这里看到一个新的结构，wait_queue_head_t，这个结构是用来将一个资源挂在等待队列中，具体参考：http://www.cnblogs.com/lubiao/p/4858086.html

还有一个结构是cgroup_root

struct cgroup_root {
     // TODO: 不清楚
    struct kernfs_root *kf_root;

    // 子系统掩码
    unsigned int subsys_mask;

    // 层级的id
    int hierarchy_id;

    // 根部的cgroup，这里面就有下级cgroup
    struct cgroup cgrp;

    // 相等于cgrp->ancester_ids[0]
    int cgrp_ancestor_id_storage;

    // 这个root层级下的cgroup数，初始化的时候为1
    atomic_t nr_cgrps;

    // 串起所有的cgroup_root
    struct list_head root_list;

    unsigned int flags;

    // TODO: 不清楚
    struct idr cgroup_idr;

    // TODO: 不清楚
    char release_agent_path[PATH_MAX];

    // 这个层级的名称，有可能为空
    char name[MAX_CGROUP_ROOT_NAMELEN];
};
cgroup_init_early
回到这个函数

int __init cgroup_init_early(void)
{
    // 初始化cgroup_root，就是一个cgroup_root的结构
    init_cgroup_root(&cgrp_dfl_root, &opts);
    cgrp_dfl_root.cgrp.self.flags |= CSS_NO_REF;

    RCU_INIT_POINTER(init_task.cgroups, &init_css_set);

    for_each_subsys(ss, i) {
        WARN(!ss->css_alloc || !ss->css_free || ss->name || ss->id,
             "invalid cgroup_subsys %d:%s css_alloc=%p css_free=%p id:name=%d:%s\n",
             i, cgroup_subsys_name[i], ss->css_alloc, ss->css_free,
             ss->id, ss->name);
        WARN(strlen(cgroup_subsys_name[i]) > MAX_CGROUP_TYPE_NAMELEN,
             "cgroup_subsys_name %s too long\n", cgroup_subsys_name[i]);

        ss->id = i;
        ss->name = cgroup_subsys_name[i];
        if (!ss->legacy_name)
            ss->legacy_name = cgroup_subsys_name[i];

        if (ss->early_init)
            cgroup_init_subsys(ss, true);
    }
    return 0;
}
这个函数初始化的cgroup_root是一个全局的变量。定义在kernel/cgroup.c中。

struct cgroup_root cgrp_dfl_root;
EXPORT_SYMBOL_GPL(cgrp_dfl_root);
