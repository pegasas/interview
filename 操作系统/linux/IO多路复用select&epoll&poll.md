# IO多路复用select&epoll&poll

# select实现
select --> sys_select --> core_sys_select --> do_select

应用程序调用select
进入内核调用sys_select，做些简单初始化工作
接着进入core_sys_select，主要工作是把描述符集合从用户空间复制到内核空间，最终进入do_select，完成其主要的功能
do_select里，调用 poll_initwait，主要工作是注册poll_wait的回调函数为__pollwait，
当在设备驱动的poll回调函数里调用poll_wait，其实就是调用__pollwait，
__pollwait的主要工作是把当前进程挂载到等待队列里，当等待的事件到来就会唤醒此进程。接着执行for循环，循环里首先遍历每个文件描述符，调用对应描述符的poll回调函数，检测是否就绪，遍历完所有描述符之后，只要有描述符处于就绪状态,信号中断,出错或者超时，就退出循环，否则会调用schedule_xxx函数，让当前进程睡眠，一直到超时或者有描述符就绪被唤醒。接着又会再次遍历每个描述符，调用poll再次检测。如此循环，直到符合条件才会退出。

select函数的部分片段

```
int do_select( int n, fd_set_bits *fds, struct timespec *end_time )
{
	ktime_t			expire, *to = NULL;
	struct poll_wqueues	table;
	poll_table		*wait;
	int			retval, i, timed_out = 0;
	unsigned long		slack = 0;

	/*
	 * /这里为了获得集合中的最大描述符，这样可减少循环中遍历的次数。
	 * /也就是为什么linux中select第一个参数为何如此重要了
	 */
	rcu_read_lock();
	retval = max_select_fd( n, fds );
	rcu_read_unlock();
	if ( retval < 0 )
		return(retval);
	n = retval;

	/* //初始化 poll_table结构，其中一个重要任务是把 __pollwait函数地址赋值给它， */
	poll_initwait( &table );
	wait = &table.pt;
	if ( end_time && !end_time->tv_sec && !end_time->tv_nsec )
	{
		wait		= NULL;
		timed_out	= 1;
	}
	if ( end_time && !timed_out )
		slack = estimate_accuracy( end_time );

	retval = 0;
	/* /主循环，将会在这里完成描述符的状态轮训 */
	for (;; )
	{
		unsigned long *rinp, *routp, *rexp, *inp, *outp, *exp;

		inp	= fds->in; outp = fds->out; exp = fds->ex;
		rinp	= fds->res_in; routp = fds->res_out; rexp = fds->res_ex;

		for ( i = 0; i < n; ++rinp, ++routp, ++rexp )
		{
			unsigned long			in, out, ex, all_bits, bit = 1, mask, j;
			unsigned long			res_in	= 0, res_out = 0, res_ex = 0;
			const struct file_operations	*f_op	= NULL;
			struct file			*file	= NULL;
			/*
			 * /select中 fd_set 以及 do_select 中的 fd_set_bits 参数，都是按照位来保存描述符，意思是比如申请一个1024位的内存，
			 * /如果第 28位置1，说明此集合有 描述符 28，
			 */
			in		= *inp++; out = *outp++; ex = *exp++;
			all_bits	= in | out | ex; /* 检测读写异常3个集合中有无描述符 */
			if ( all_bits == 0 )
			{
				i += __NFDBITS;
				continue;
			}

			for ( j = 0; j < __NFDBITS; ++j, ++i, bit <<= 1 )
			{
				int fput_needed;
				if ( i >= n )
					break;
				if ( !(bit & all_bits) )
					continue;
				file = fget_light( i, &fput_needed );                   /* /通过 描述符 index 获得 struct file结构指针， */
				if ( file )
				{
					f_op	= file->f_op;                           /* 通过 struct file 获得 file_operations，这是操作文件的回调函数集合。 */
					mask	= DEFAULT_POLLMASK;
					if ( f_op && f_op->poll )
					{
						wait_key_set( wait, in, out, bit );
						mask = (*f_op->poll)( file, wait );     /* 调用我们的设备中实现的 poll函数， */
						/* 因此，为了能让select正常工作，在我们设备驱动中，必须要提供poll的实现， */
					}
					fput_light( file, fput_needed );
					if ( (mask & POLLIN_SET) && (in & bit) )
					{
						res_in |= bit;
						retval++;
						wait = NULL;    /*
						                 * / 此处包括以下的，把wait设置为NULL，是因为检测到mask = (*f_op->poll)(file, wait); 描述符已经就绪
						                 * / 无需再把当前进程添加到等待队列里，do_select 遍历完所有描述符之后就会退出。
						                 */
					}
					if ( (mask & POLLOUT_SET) && (out & bit) )
					{
						res_out |= bit;
						retval++;
						wait = NULL;
					}
					if ( (mask & POLLEX_SET) && (ex & bit) )
					{
						res_ex |= bit;
						retval++;
						wait = NULL;
					}
				}
			}
			if ( res_in )
				*rinp = res_in;
			if ( res_out )
				*routp = res_out;
			if ( res_ex )
				*rexp = res_ex;
			cond_resched();
		}
		wait = NULL;                                            /* 已经遍历完一遍，该加到等待队列的，都已经加了，无需再加，因此设置为NULL */
		if ( retval || timed_out || signal_pending( current ) ) /* 描述符就绪，超时，或者信号中断就退出循环 */
			break;
		if ( table.error )                                      /* 出错退出循环 */
		{
			retval = table.error;
			break;
		}


		/*
		 * If this is the first loop and we have a timeout
		 * given, then we convert to ktime_t and set the to
		 * pointer to the expiry value.
		 */
		if ( end_time && !to )
		{
			expire	= timespec_to_ktime( *end_time );
			to	= &expire;
		}
		/* ///让进程休眠，直到超时，或者被就绪的描述符唤醒， */
		if ( !poll_schedule_timeout( &table, TASK_INTERRUPTIBLE,
					     to, slack ) )
			timed_out = 1;
	}

	poll_freewait( &table );

	return(retval);
}


void poll_initwait( struct poll_wqueues *pwq )
{
	init_poll_funcptr( &pwq->pt, __pollwait ); /* 设置poll_table的回调函数为 __pollwait,这样当我们在驱动中调用poll_wait 就会调用到 __pollwait */
	... .....
}


static void __pollwait( struct file *filp, wait_queue_head_t *wait_address,
			poll_table *p )
{
	... ... ... ... ... ....
	init_waitqueue_func_entry( &entry->wait, pollwake );    /* 设置唤醒进程调用的回调函数，当在驱动中调用 wake_up唤醒队列时候， */
	/*
	 * pollwake会被调用，这里其实就是调用队列的默认函数 default_wake_function
	 * 用来唤醒睡眠的进程。
	 */
	add_wait_queue( wait_address, &entry->wait );           /* 加入到等待队列 */
}


int core_sys_select( int n, fd_set __user *inp, fd_set __user *outp,
		     fd_set __user *exp, struct timespec *end_time )
{
	... .....
	/* 把描述符集合从用户空间复制到内核空间 */
	if ( (ret = get_fd_set( n, inp, fds.in ) ) ||
	     (ret = get_fd_set( n, outp, fds.out ) ) ||
	     (ret = get_fd_set( n, exp, fds.ex ) ) )
		... ... ...
		ret = do_select( n, &fds, end_time );
	... ... ... ....
	/* //把do_select返回集合，从内核空间复制到用户空间 */
	if ( set_fd_set( n, inp, fds.res_in ) ||
	     set_fd_set( n, outp, fds.res_out ) ||
	     set_fd_set( n, exp, fds.res_ex ) )
		ret = -EFAULT;
	... ... ... ...
}
```

# poll实现
poll的实现跟select基本差不多，按照

poll --> do_sys_poll --> do_poll --> do_pollfd 的调用序列

其中do_pollfd是对每个描述符调用 其回调poll状态轮训。

poll比select的好处就是没有描述多少限制，select 有1024 的限制，描述符不能超过此值，poll不受限制。

总结出select/poll缺陷如下：

1）每次调用select/poll都需要要把描述符集合从用户空间copy到内核空间，检测完成之后，又要把检测的结果集合从内核空间copy到用户空间当描述符很多，而且select经常被唤醒，这种开销会比较大

2）如果说描述符集合来回复制不算什么，那么多次的全部描述符遍历就比较恐怖了，
我们在应用程序中，每次调用select/poll 都必须首先遍历描述符，把他们加到fd_set集合里，这是应用层的第一次遍历，接着进入内核空间，至少进行一次遍历和调用每个描述符的poll回调检测，一般可能是2次遍历，第一次没发现就绪描述符，加入等待队列，第二次是被唤醒，接着再遍历一遍。再回到应用层，我们还必须再次遍历所有描述符，用 FD_ISSET检测结果集。如果描述符很多，这种遍历就很消耗CPU资源了。

3）描述符多少限制，当然poll没有限制，select却有1024的硬性限制，除了修改内核增加1024限制外没别的办法。

既然有这么些缺点 ，那不是 select/poll变得一无是处了，那就大错特错了。他们依然是代码移植的最好函数，因为几乎所有平台都有对它们的实现提供接口。在描述符不是太多，他们依然十分出色的完成多路复用IO， 而且如果每个连接上的描述符都处于活跃状态，他们的效率其实跟epoll也差不了多少。曾经使用多个线程+每个线程采用poll的办法开发TCP服务器，处理文件收发，连接达到几千个，当时的瓶颈已经不在网络IO，而在磁盘IO了。

# epoll实现

epoll分为三个函数 epoll_create,epoll_ctl, epoll_wait 。

epoll_create创建epoll设备，用来管理所有添加进去的描述符，epoll_ctl 用来添加新的描述符，修改或者删除描述符。

epoll_wait等待描述符事件。epoll_wait的等待已经不再是轮训方式的等待了，epoll内部有个描述符就绪队列，epoll_wait只检测这个队列即可，他采用睡眠一会检测一下的方式，如果发现描述符就绪队列不为空，就把此队列中的描述符copy到用户空间，然后返回。描述符就绪队列里的数据又是从何而来的？原来使用 epoll_ctl添加新描述符时候，epoll_ctl内核实现里会修改两个回调函数，一个是 poll_table结构里的qproc回调函数指针， 在 select中是 __pollwait函数，在epoll中换成 ep_ptable_queue_proc，当在epoll_ctl中调用新添加的描述符的poll回调时候，底层驱动就会调用 poll_wait添加等待队列，底层驱动调用poll_wait时候，其实就是调用ep_ptable_queue_proc，此函数会修改等待队列的回调函数为 ep_poll_callback， 并加入到等待队列头里；一旦底层驱动发现数据就绪，就会调用wake_up唤醒等待队列，从而 ep_poll_callback将被调用，在ep_poll_callback中 会把这个就绪的描述符添加到 epoll的描述符就绪队列里，并同时唤醒 epoll_wait 所在的进程。

epoll是如何解决 select/poll的缺陷的，首先他通过 epoll_ctl的EPOLL_CTL_ADD命令把描述符添加进epoll内部管理器里，只需添加一次即可，直到用 epoll_ctl的EPOLL_CTL_DEL命令删除此描述符为止，而不像select/poll是每次执行都必须添加，很显然大量减少了描述符在内核和用户空间不断的来回copy的开销。其次虽然 epoll_wait内部也是循环检测，但是它只需检测描述符就绪队列是否为空即可，
比起select/poll必须轮训每个描述符的poll，其开销简直可以忽略不计。
他同时也没描述符多少的限制，只要你机器的内存够大，就能容纳非常多的描述符。

epoll相关部分内核代码片段：

```
struct epitem {
  /* RB tree node used to link this structure to the eventpoll RB tree */
  struct rb_node rbn; //   红黑树节点，

  struct epoll_filefd ffd;  // 存储此变量对应的描述符

  struct epoll_event event; //用户定义的结构
  /*其他成员*/

};

struct eventpoll {
  /*其他成员*/
  .......

  /* Wait queue used by file->poll() */
  wait_queue_head_t poll_wait;

  /* List of ready file descriptors */
  struct list_head rdllist;      ///描述符就绪队列，挂载的是 epitem结构

  /* RB tree root used to store monitored fd structs */
  struct rb_root rbr; /// 存储 新添加的 描述符的红黑树根， 此成员用来存储添加进来的所有描述符。挂载的是epitem结构

   .........
};

//epoll_create
SYSCALL_DEFINE1(epoll_create1, int, flags)
{
  int error;
  struct eventpoll *ep = NULL;

  /*其他代码*/
  ......
  //分配 eventpoll结构，这个结构是epoll的灵魂，他包含了所有需要处理得数据。
  error = ep_alloc(&ep);
  if (error < 0)
    return error;

  error = anon_inode_getfd("[eventpoll]", &eventpoll_fops, ep,
         flags & O_CLOEXEC); ///打开 eventpoll 的描述符，并把 ep存储到 file->private_data变量里。
  if (error < 0)
    ep_free(ep);

  return error;
}

SYSCALL_DEFINE4(epoll_ctl, int, epfd, int, op, int, fd,
    struct epoll_event __user *, event)
{
  /*其他代码*/
  .....
  ep = file->private_data;
  ......
  epi = ep_find(ep, tfile, fd);  ///从 eventpoll的 rbr里查找描述符是 fd 的 epitem，

  error = -EINVAL;
  switch (op) {
  case EPOLL_CTL_ADD:
    if (!epi) {
      epds.events |= POLLERR | POLLHUP;
      error = ep_insert(ep, &epds, tfile, fd);   // 在这个函数里添加新描述符，同时修改重要的回调函数。
                                      //同时还调用描述符的poll，查看就绪状态
    } else
      error = -EEXIST;
    break;
   /*其他代码*/
   ........
}
static int ep_insert(struct eventpoll *ep, struct epoll_event *event,
       struct file *tfile, int fd)
{
  ..... /*其他代码*/

  init_poll_funcptr(&epq.pt, ep_ptable_queue_proc);//设置 poll_tabe回调函数为 ep_ptable_queue_proc
         //ep_ptable_queue_proc会设置等待队列的回调指针为 ep_epoll_callback，同时添加等待队列。

  ........ /*其他代码*/

  revents = tfile->f_op->poll(tfile, &epq.pt); //调用描述符的poll回调，在此函数里 ep_ptable_queue_proc会被调用

  ....... /*其他代码*/

  ep_rbtree_insert(ep, epi); //把新生成关于epitem添加到红黑树里

  ...... /*其他代码*/

  if ((revents & event->events) && !ep_is_linked(&epi->rdllink)) {
    list_add_tail(&epi->rdllink, &ep->rdllist); //如果 上边的poll调用，检测到描述符就绪，添加本描述符到就绪队列里。

    if (waitqueue_active(&ep->wq))
      wake_up_locked(&ep->wq);
    if (waitqueue_active(&ep->poll_wait))
      pwake++;
  }
  ...... /*其他代码*/
  /* We have to call this outside the lock */
  if (pwake)
    ep_poll_safewake(&ep->poll_wait); // 如果描述符就绪队列不为空，则唤醒 epoll_wait所在的进程。

  ......... /*其他代码*/

}
//这个函数设置等待队列回调函数为 ep_poll_callback，
//这样到底层有数据唤醒等待队列时候，ep_poll_callback就会被调用，从而把就绪的描述符加到就绪队列。
static void ep_ptable_queue_proc(struct file *file, wait_queue_head_t *whead,
         poll_table *pt)
{
  struct epitem *epi = ep_item_from_epqueue(pt);
  struct eppoll_entry *pwq;

  if (epi->nwait >= 0 && (pwq = kmem_cache_alloc(pwq_cache, GFP_KERNEL))) {
    init_waitqueue_func_entry(&pwq->wait, ep_poll_callback);
    pwq->whead = whead;
    pwq->base = epi;
    add_wait_queue(whead, &pwq->wait);
    list_add_tail(&pwq->llink, &epi->pwqlist);
    epi->nwait++;
  } else {
    /* We have to signal that an error occurred */
    epi->nwait = -1;
  }
}
static int ep_poll_callback(wait_queue_t *wait, unsigned mode, int sync, void *key)
{
  int pwake = 0;
  unsigned long flags;
  struct epitem *epi = ep_item_from_wait(wait);
  struct eventpoll *ep = epi->ep;
  ......... /*其他代码*/
  if (!ep_is_linked(&epi->rdllink))
    list_add_tail(&epi->rdllink, &ep->rdllist); // 把当前就绪的描述epitem结构添加到就绪队列里
  ......... /*其他代码*/

  if (pwake)
    ep_poll_safewake(&ep->poll_wait); //如果队列不为空，唤醒 epoll_wait所在进程
  ......... /*其他代码*/
}

epoll_wait内核代码里主要是调用ep_poll，列出ep_poll部分代码片段：
static int ep_poll(struct eventpoll *ep, struct epoll_event __user *events,
      int maxevents, long timeout)
{
  int res, eavail;
  unsigned long flags;
  long jtimeout;
  wait_queue_t wait;
  ......... /*其他代码*/

  if (list_empty(&ep->rdllist)) {

    init_waitqueue_entry(&wait, current);
    wait.flags |= WQ_FLAG_EXCLUSIVE;
    __add_wait_queue(&ep->wq, &wait);
    // 如果检测到就绪队列为空，添加当前进程到等待队列，并执行否循环
    for (;;) {

      set_current_state(TASK_INTERRUPTIBLE);
      if (!list_empty(&ep->rdllist) || !jtimeout)  //如果就绪队列不为空，或者超时则退出循环
        break;
      if (signal_pending(current)) { //如果信号中断，退出循环
        res = -EINTR;
        break;
      }

      spin_unlock_irqrestore(&ep->lock, flags);
      jtimeout = schedule_timeout(jtimeout);//睡眠，知道被唤醒或者超时为止。
      spin_lock_irqsave(&ep->lock, flags);
    }
    __remove_wait_queue(&ep->wq, &wait);

    set_current_state(TASK_RUNNING);
  }

  ......... /*其他代码*/

  if (!res && eavail &&
    !(res = ep_send_events(ep, events, maxevents)) && jtimeout)
    goto retry;
   // ep_send_events主要任务是把就绪队列的就绪描述符copy到用户空间的 epoll_event数组里，

  return res;
}
```

可以看到 ep_poll既epoll_wait的循环是相当轻松的循环，他只是简单检测就绪队列而已，因此他的开销很小。

我们最后看看描述符就绪时候，是如何通知给select/poll/epoll的，以网络套接字的TCP协议来进行说明。

tcp协议对应的 poll回调是tcp_poll， 对应的等待队列头是 struct sock结构里 sk_sleep成员，
在tcp_poll中会把 sk_sleep加入到等待队列，等待数据就绪。

当物理网卡接收到数据包，引发硬件中断，驱动在中断ISR例程里，构建skb包，把数据copy进skb，接着调用netif_rx
把skb挂载到CPU相关的 input_pkt_queue队列，同时引发软中断，在软中断的net_rx_action回调函数里从input_pkt_queue里取出
skb数据包，通过分析，调用协议相关的回调函数，这样层层传递，一直到struct sock，此结构里的 sk_data_ready回调指针被调用
sk_data_ready指向 sock_def_readable 函数，sock_def_readable函数其实就是 wake_up 唤醒 sock结构里 的 sk_sleep。
以上机制，对 select/poll/epoll都是一样的，接下来唤醒 sk_sleep方式就不一样了，因为他们指向了不同的回调函数。
在 select/poll实现中，等待队列回调函数是 pollwake其实就是调用default_wake_function，唤醒被select阻塞住的进程。
epoll实现中，等待回调函数是 ep_poll_callback， 此回调函数只是把就绪描述符加入到epoll的就绪队列里。

所以呢 select/poll/epoll其实他们在内核实现中，差别也不是太大，其实都差不多。
epoll虽然效率不错，可以跟windows平台中的完成端口比美，但是移植性太差，
目前几乎就只有linux平台才实现了epoll而且必须是2.6以上的内核版本。
