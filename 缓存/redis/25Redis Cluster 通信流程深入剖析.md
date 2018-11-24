
# Redis源码剖析和注释（二十五）--- Redis Cluster 的通信流程深入剖析（载入配置文件、节点握手、分配槽）
# Redis Cluster 通信流程深入剖析
## 1. Redis Cluster 介绍和搭建

请查看这篇博客：[Redis Cluster 介绍与搭建](http://blog.csdn.net/men_wen/article/details/72853078)

这篇博客会介绍`Redis Cluster`的**数据分区理论**和一个**三主三从集群**的搭建。

[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)  
本文会详细剖析搭建 Redis Cluster 的通信流程

## 2. Redis Cluster 和 Redis Sentinel

`Redis 2.8`之后正式提供了`Redis Sentinel(哨兵)`架构，而`Redis Cluster(集群)`是在`Redis 3.0`正式加入的功能。

`Redis Cluster`  和  `Redis Sentinel`都可以搭建`Redis`多节点服务，而目的都是解决`Redis`主从复制的问题，但是他们还是有一些不同。

`Redis`主从复制可将主节点数据同步给从节点，从节点此时有两个作用：

-   一旦主节点宕机，从节点作为主节点的备份可以随时顶上来。
-   扩展主节点的读能力，分担主节点读压力。

**但是，会出现以下问题：**

1.  一旦主节点宕机，从节点晋升成主节点，同时需要修改应用方的主节点地址，还需要命令所有从节点去复制新的主节点，整个过程需要人工干预。
2.  主节点的写能力或存储能力受到单机的限制。

**Redis的解决方案：**

-   `Redis Sentinel`旨在解决第一个问题，即使主节点宕机下线，`Redis Sentinel`可以自动完成故障检测和故障转移，并通知应用方，真正实现高可用性（HA）。
-   `Redis Cluster`则是`Redis`分布式的解决方案，解决后两个问题。当单机内存、并发、流量等瓶颈时，可以采用`Cluster`架构达到负载均衡的目的。

> 关于`Redis Sentinel`的介绍和分析：
> 
> [Redis Sentinel 介绍与部署](http://blog.csdn.net/men_wen/article/details/72724406)
> 
> [Redis Sentinel实现（上）（哨兵的执行过程和执行内容）](http://blog.csdn.net/men_wen/article/details/72805850)
> 
> [Redis Sentinel实现（下）（哨兵操作的深入剖析）](http://blog.csdn.net/men_wen/article/details/72805897)

## 3. 搭建 Redis Cluster的通信流程深入剖析

在[Redis Cluster 介绍与搭建](http://blog.csdn.net/men_wen/article/details/72853078)一文中介绍了搭建集群的流程，分为三步：

-   准备节点
-   节点握手
-   分配槽位

我们就根据这个流程分析`Redis Cluster`的执行过程。

[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)

### 3.1 准备节点

我们首先要准备`6`个节点，并且准备号对应端口号的配置文件，在配置文件中，要打开`cluster-enabled yes`选项，表示该节点以集群模式打开。因为集群节点服务器可以看做一个普通的`Redis`服务器，因此，集群节点开启服务器的流程和普通的相似，只不过打开了一些关于集群的标识。

当我们执行这条命令时，就会执行主函数
```
sudo redis-server conf/redis-6379.conf
```
在`main()`函数中，我们需要关注这几个函数：

-   `loadServerConfig(configfile,options)`载入配置文件。  
    -   底层最终调用`loadServerConfigFromString()`函数，会解析到`cluster-`开头的集群的相关配置，并且保存到服务器的状态中。
-   `initServer()`初始化服务器。  
    -   会为服务器设置时间事件的处理函数`serverCron()`，该函数会每间隔`100ms`执行一次集群的周期性函数`clusterCron()`。
    -   之后会执行`clusterInit()`，来初始化`server.cluster`，这是一个`clusterState`类型的结构，保存的是集群的状态信息。
    -   接着在`clusterInit()`函数中，如果是第一次创建集群节点，会创建一个随机名字的节点并且会生成一个集群专有的配置文件。如果是重启之前的集群节点，会读取第一次创建的集群专有配置文件，创建与之前相同名字的集群节点。
-   `verifyClusterConfigWithData()`该函数在载入AOF文件或RDB文件后被调用，用来检查载入的数据是否正确和校验配置是否正确。
-   `aeSetBeforeSleepProc()`在进入事件循环之前，为服务器设置每次事件循环之前都要执行的一个函数`beforeSleep()`，该函数一开始就会执行集群的`clusterBeforeSleep()`函数。
-   `aeMain()`进入事件循环，一开始就会执行之前设置的`beforeSleep()`函数，之后就等待事件发生，处理就绪的事件。

以上就是主函数在开启集群节点时会执行到的主要代码。

在第二步初始化时，会创建一个`clusterState`类型的结构来保存**当前节点视角下的集群状态**。我们列出该结构体的代码：
```
typedef struct clusterState {
	clusterNode *myself;    /* This node */
	/* 当前纪元 */
	uint64_t currentEpoch;
	/* 集群的状态 */
	int state;              /* CLUSTER_OK, CLUSTER_FAIL, ... */
	/* 集群中至少负责一个槽的主节点个数 */
	int size;               /* Num of master nodes with at least one slot */
	/* 保存集群节点的字典，键是节点名字，值是clusterNode结构的指针 */
	dict *nodes;            /* Hash table of name -> clusterNode structures */
	/* 防止重复添加节点的黑名单 */
	dict *nodes_black_list; /* Nodes we don't re-add for a few seconds. */
	/* 导入槽数据到目标节点，该数组记录这些节点 */
	clusterNode *migrating_slots_to[CLUSTER_SLOTS];
	/* 导出槽数据到目标节点，该数组记录这些节点 */
	clusterNode *importing_slots_from[CLUSTER_SLOTS];
	/* 槽和负责槽节点的映射 */
	clusterNode *slots[CLUSTER_SLOTS];
	/* 槽映射到键的有序集合 */
	zskiplist *slots_to_keys;
	/* The following fields are used to take the slave state on elections. */
	/* 之前或下一次选举的时间 */
	mstime_t failover_auth_time;    /* Time of previous or next election. */
	/* 节点获得支持的票数 */
	int failover_auth_count;        /* Number of votes received so far. */
	/* 如果为真，表示本节点已经向其他节点发送了投票请求 */
	int failover_auth_sent;         /* True if we already asked for votes. */
	/* 该从节点在当前请求中的排名 */
	int failover_auth_rank;         /* This slave rank for current auth request. */
	/* 当前选举的纪元 */
	uint64_t failover_auth_epoch;   /* Epoch of the current election. */
	/* 从节点不能执行故障转移的原因 */
	int cant_failover_reason;
	/* Manual failover state in common. */
	/* 如果为0，表示没有正在进行手动的故障转移。否则表示手动故障转移的时间限制 */
	mstime_t mf_end;
	/* Manual failover state of master. */
	/* 执行手动孤战转移的从节点 */
	clusterNode *mf_slave;  /* Slave performing the manual failover. */
	/* Manual failover state of slave. */
	/* 从节点记录手动故障转移时的主节点偏移量 */
	long long mf_master_offset;
	/* 非零值表示手动故障转移能开始 */
	int mf_can_start;
	/* The followign fields are used by masters to take state on elections. */
	/* 集群最近一次投票的纪元 */
	uint64_t lastVoteEpoch;                 /* Epoch of the last vote granted. */
	/* 调用clusterBeforeSleep()所做的一些事 */
	int todo_before_sleep;                  /* Things to do in clusterBeforeSleep(). */
	/* 发送的字节数 */
	long long stats_bus_messages_sent;      /* Num of msg sent via cluster bus. */
	/* 通过Cluster接收到的消息数量 */
	long long stats_bus_messages_received;  /* Num of msg rcvd via cluster bus.*/
} clusterState;
```
初始化完当前集群状态后，会创建集群节点，执行的代码是这样的：
```
myself = server.cluster->myself = createClusterNode(NULL,CLUSTER_NODE_MYSELF|CLUSTER_NODE_MASTER);
```
首先`myself`是一个全局变量，定义在`cluster.h`中，它指向当前集群节点，`server.cluster->myself`是集群状态结构中指向当前集群节点的变量，`createClusterNode()`函数用来创建一个集群节点，并设置了两个标识，表明身份状态信息。

该函数会创建一个如下结构来描述集群节点。
```
typedef struct clusterNode {
	/* 节点创建的时间 */
	mstime_t ctime;                         /* Node object creation time. */
	/* 名字 */
	char name[CLUSTER_NAMELEN];             /* Node name, hex string, sha1-size */
	/* 标识 */
	int		flags;                  /* CLUSTER_NODE_... */
	uint64_t	configEpoch;            /* Last configEpoch observed for this node */
	/* 节点的槽位图 */
	unsigned char slots[CLUSTER_SLOTS / 8]; /* slots handled by this node */
	/* 当前节点复制槽的数量 */
	int numslots;                           /* Number of slots handled by this node */
	/* 从节点的数量 */
	int numslaves;                          /* Number of slave nodes, if this is a master */
	/* 从节点指针数组 */
	struct clusterNode **slaves;            /* pointers to slave nodes */
	/* 指向主节点，即使是从节点也可以为NULL */
	struct clusterNode *slaveof;
	/* 最近一次发送PING的时间 */
	mstime_t ping_sent;                     /* Unix time we sent latest ping */
	/* 接收到PONG的时间 */
	mstime_t pong_received;                 /* Unix time we received the pong */
	/* 被设置为FAIL的下线时间 */
	mstime_t fail_time;                     /* Unix time when FAIL flag was set */
	/* 最近一次为从节点投票的时间 */
	mstime_t voted_time;                    /* Last time we voted for a slave of this master */
	/* 更新复制偏移量的时间 */
	mstime_t repl_offset_time;              /* Unix time we received offset for this node */
	/* 孤立的主节点迁移的时间 */
	mstime_t orphaned_time;                 /* Starting time of orphaned master condition */
	/* 该节点已知的复制偏移量 */
	long long repl_offset;                  /* Last known repl offset for this node. */
	/* ip地址 */
	char ip[NET_IP_STR_LEN];                /* Latest known IP address of this node */
	/* 节点端口号 */
	int port;                               /* Latest known port of this node */
	/* 与该节点关联的连接对象 */
	clusterLink *link;                      /* TCP/IP link with this node */
	/* 保存下线报告的链表 */
	list *fail_reports;                     /* List of nodes signaling this as failing */
} clusterNode;
```
初始化该结构时，会创建一个`link`为空的节点，该变量是`clusterLink`的指针，用来描述该节点与一个节点建立的连接。该结构定义如下：
```
typedef struct clusterLink {
	/* 连接创建的时间 */
	mstime_t ctime;                 /* Link creation time */
	/* TCP连接的文件描述符 */
	int fd;                         /* TCP socket file descriptor */
	/* 输出（发送）缓冲区 */
	sds sndbuf;                     /* Packet send buffer */
	/* 输入（接收）缓冲区 */
	sds rcvbuf;                     /* Packet reception buffer */
	/* 关联该连接的节点 */
	struct clusterNode *node;       /* Node related to this link if any, or NULL */
} clusterLink;
```
该结构用于集群两个节点之间相互发送消息。如果节点A发送`MEET`消息给节点B，那么节点A会创建一个`clusterLink`结构的连接，`fd`设置为连接后的套节字，`node`设置为节点B，最后将该`clusterLink`结构保存到节点B的`link`中。

### 3.2 节点握手

当我们创建好了6个节点时，需要通过节点握手来感知到到指定的进程。节点握手是指一批运行在集群模式的节点通过`Gossip`协议彼此通信。节点握手是集群彼此通信的第一步，可以详细分为这几个过程：

-   `myself`节点发送`MEET`消息给目标节点。
-   目标节点处理`MEET`消息，并回复一个`PONG`消息给`myself`节点。
-   `myself`节点处理`PONG`消息，回复一个`PING`消息给目标节点。

这里只列出了握手阶段的通信过程，之后无论什么节点，都会每隔`1s`发送一个`PING`命令给随机筛选出的`5`个节点，以进行故障检测。

接下来会分别以**`myself`节点**和**目标节点**的视角分别剖析这个握手的过程。

[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)

#### 3.2.1  `myself`节点发送 MEET 消息

由客户端发起命令：`cluster meet <ip> <port>`

当节点接收到客户端的`cluster meet`命令后会调用对应的函数来处理命令，该命令的执行函数是`clusterCommand()`函数，该函数能够处理所有的`cluster`命令，因此我们列出处理`meet`选项的代码：
```
/*
 * CLUSTER MEET <ip> <port>命令
 * 与给定地址的节点建立连接
 */
if ( !strcasecmp( c->argv[1]->ptr, "meet" ) && c->argc == 4 )
{
	long long port;
	/* 获取端口 */
	if ( getLongLongFromObject( c->argv[3], &port ) != C_OK )
	{
		addReplyErrorFormat( c, "Invalid TCP port specified: %s",
				     (char *) c->argv[3]->ptr );
		return;
	}
	/* 如果没有正在进行握手，那么根据执行的地址开始进行握手操作 */
	if ( clusterStartHandshake( c->argv[2]->ptr, port ) == 0 &&
	     errno == EINVAL )
	{
		addReplyErrorFormat( c, "Invalid node address specified: %s:%s",
				     (char *) c->argv[2]->ptr, (char *) c->argv[3]->ptr );
		/* 连接成功回复ok */
	} else {
		addReply( c, shared.ok );
	}
}
```
该函数先根据`cluster meet <ip> <port>`命令传入的参数，获取要与目标节点建立连接的节点地址，然后根据节点地址执行`clusterStartHandshake()`函数来开始执行握手操作。该函数代码如下：
```
int clusterStartHandshake( char *ip, int port )
{
	clusterNode		*n;
	char			norm_ip[NET_IP_STR_LEN];
	struct sockaddr_storage sa;

	/* 检查地址是否非法 */
	if ( inet_pton( AF_INET, ip,
			&( ( (struct sockaddr_in *) &sa)->sin_addr) ) )
	{
		sa.ss_family = AF_INET;
	} else if ( inet_pton( AF_INET6, ip,
			       &( ( (struct sockaddr_in6 *) &sa)->sin6_addr) ) )
	{
		sa.ss_family = AF_INET6;
	} else {
		errno = EINVAL;
		return(0);
	}
	/* 检查端口号是否合法 */
	if ( port <= 0 || port > (65535 - CLUSTER_PORT_INCR) )
	{
		errno = EINVAL;
		return(0);
	}
	/* 设置 norm_ip 作为节点地址的标准字符串表示形式 */
	memset( norm_ip, 0, NET_IP_STR_LEN );
	if ( sa.ss_family == AF_INET )
		inet_ntop( AF_INET,
			   (void *) &( ( (struct sockaddr_in *) &sa)->sin_addr),
			   norm_ip, NET_IP_STR_LEN );
	else
		inet_ntop( AF_INET6,
			   (void *) &( ( (struct sockaddr_in6 *) &sa)->sin6_addr),
			   norm_ip, NET_IP_STR_LEN );
	/* 判断当前地址是否处于握手状态，如果是，则设置errno并返回，该函数被用来避免重复和相同地址的节点进行握手 */
	if ( clusterHandshakeInProgress( norm_ip, port ) )
	{
		errno = EAGAIN;
		return(0);
	}

	/*
	 * 为node设置一个随机的地址，当握手完成时会为其设置真正的名字
	 * 创建一个随机名字的节点
	 */
	n = createClusterNode( NULL, CLUSTER_NODE_HANDSHAKE | CLUSTER_NODE_MEET );
	/* 设置地址 */
	memcpy( n->ip, norm_ip, sizeof(n->ip) );
	n->port = port;
	/* 添加到集群中 */
	clusterAddNode( n );
	return(1);
}
```
该函数先判断传入的地址是否非法，如果非法会设置`errno`，然后会调用`clusterHandshakeInProgress()`函数来判断是否要进行握手的节点也处于握手状态，以避免重复和相同地址的目标节点进行握手。然后创建一个随机名字的目标节点，并设置该目标节点的状态，如下：
```
n = createClusterNode(NULL,CLUSTER_NODE_HANDSHAKE|CLUSTER_NODE_MEET);
```
然后调用`clusterAddNode()`函数将该目标节点添加到集群中，也就是`server.cluster->nodes`字典，该字典的键是节点的名字，值是指向`clusterNode()`结构的指针。

此时`myself`节点并没有将`meet`消息发送给指定地址的目标节点，而是设置集群中目标节点的状态。而发送`meet`消息则是在`clusterCron()`函数中执行。我们列出周期性函数中发送`MEET`消息的代码：
```
/*
 * 获取握手状态超时的时间，最低为1s
 * 如果一个处于握手状态的节点如果没有在该超时时限内变成一个普通的节点，那么该节点从节点字典中被删除
 */
handshake_timeout = server.cluster_node_timeout;
if ( handshake_timeout < 1000 )
	handshake_timeout = 1000;

/* 检查是否当前集群中有断开连接的节点和重新建立连接的节点 */
di = dictGetSafeIterator( server.cluster->nodes );
/* 遍历所有集群中的节点，如果有未建立连接的节点，那么发送PING或PONG消息，建立连接 */
while ( (de = dictNext( di ) ) != NULL )
{
	clusterNode *node = dictGetVal( de );
	/* 跳过myself节点和处于NOADDR状态的节点 */
	if ( node->flags & (CLUSTER_NODE_MYSELF | CLUSTER_NODE_NOADDR) )
		continue;

	/* 如果仍然node节点处于握手状态，但是从建立连接开始到现在已经超时 */
	if ( nodeInHandshake( node ) && now - node->ctime > handshake_timeout )
	{
		/* 从集群中删除该节点，遍历下一个节点 */
		clusterDelNode( node );
		continue;
	}
	/* 如果节点的连接对象为空 */
	if ( node->link == NULL )
	{
		int		fd;
		mstime_t	old_ping_sent;
		clusterLink	*link;
		/* myself节点连接这个node节点 */
		fd = anetTcpNonBlockBindConnect( server.neterr, node->ip,
						 node->port + CLUSTER_PORT_INCR, NET_FIRST_BIND_ADDR );
		/* 连接出错，跳过该节点 */
		if ( fd == -1 )
		{
			/* 如果ping_sent为0，察觉故障无法执行，因此要设置发送PING的时间，当建立连接后会真正的的发送PING命令 */
			if ( node->ping_sent == 0 )
				node->ping_sent = mstime();
			serverLog( LL_DEBUG, "Unable to connect to "
				   "Cluster Node [%s]:%d -> %s", node->ip,
				   node->port + CLUSTER_PORT_INCR,
				   server.neterr );
			continue;
		}
		/* 为node节点创建一个连接对象 */
		link = createClusterLink( node );
		/* 设置连接对象的属性 */
		link->fd = fd;
		/* 为node设置连接对象 */
		node->link = link;
		/* 监听该连接的可读事件，设置可读时间的读处理函数 */
		aeCreateFileEvent( server.el, link->fd, AE_READABLE, clusterReadHandler, link );
		/* 备份旧的发送PING的时间 */
		old_ping_sent = node->ping_sent;
		/* 如果node节点指定了MEET标识，那么发送MEET命令，否则发送PING命令 */
		clusterSendPing( link, node->flags & CLUSTER_NODE_MEET ?
				 CLUSTERMSG_TYPE_MEET : CLUSTERMSG_TYPE_PING );
		/* 如果不是第一次发送PING命令，要将发送PING的时间还原，等待被clusterSendPing()更新 */
		if ( old_ping_sent )
		{
			node->ping_sent = old_ping_sent;
		}
		/*
		 * 发送MEET消息后，清除MEET标识
		 * 如果没有接收到PONG回复，那么不会在向该节点发送消息
		 * 如果接收到了PONG回复，取消MEET/HANDSHAKE状态，发送一个正常的PING消息。
		 */
		node->flags &= ~CLUSTER_NODE_MEET;
		serverLog( LL_DEBUG, "Connecting with Node %.40s at %s:%d",
			   node->name, node->ip, node->port + CLUSTER_PORT_INCR );
	}
}
dictReleaseIterator( di );
```
`clusterNode()`函数一开始就会处理集群中**断开连接的节点和重新建立连接的节点**。

**以`myself`节点的视角**，遍历集群中所有的节点，跳过操作当前`myself`节点和没有指定地址的节点，然后判断处于握手状态的节点是否在建立连接的过程中超时，如果超时则会删除该节点。如果还没有创建连接，那么`myself`节点会与当前这个目标节点建立`TCP`连接，并获取套接字`fd`，根据这个套接字，就可以创建`clusterLink`结构的连接对象，并将这个连接对象保存到当前这个目标节点。

`myself`节点创建完连接后，首先会监听与目标节点建立的`fd`的可读事件，并设置对应的处理程序`clusterReadHandler()`，因为当发送`MEET`消息给目标节点后，要接收目标节点回复的`PING`。

接下来，`myself`节点就调用`clusterSendPing()`函数发送`MEET`消息给目标节点。`MEET`消息是特殊的`PING`消息，只用于通知新节点的加入，而`PING`消息还需要更改一些时间信息，以便进行故障检测。

最后无论如何都要取消`CLUSTER_NODE_MEET`标识，但是没有取消`CLUSTER_NODE_HANDSHAKE`该标识，表示仍处于握手状态，但是已经发送了`MEET`消息了。

#### 3.2.2 目标节点处理 MEET 消息回复 PONG 消息

当`myself`节点将`MEET`消息发送给目标节点之前，就设置了`clusterReadHandler()`函数为处理接收的`PONG`消息。当时目标节点如何接收到`MEET`消息，并且回复`PONG`消息给`myself`节点呢？

在集群模式下，每个节点初始化时调用的`clusterInit`时，会监听节点的端口等待客户端的连接，并且会将该监听的套接字`fd`保存到`server.cfd`数组中，然后创建文件事件，监听该套接字`fd`的可读事件，并设置可读事件处理函数`clusterAcceptHandler()`，等待客户端发送数据。

那么，在`myself`节点在发送`MEET`消息首先会连接目标节点所监听的端口，触发目标节点执行`clusterAcceptHandler()`函数，该函数实际上就是`accept()`函数，接收`myself`节点的连接，然后监听该连接上的可读事件，设置可读事件的处理函数为`clusterReadHandler()`，等待`myself`节点发送数据，当`myself`节点发送`MEET`消息给目标节点时，触发目标节点执行`clusterReadHandler()`函数来处理消息。

接下来，我们以目标节点的视角，来分析处理`MEET`消息的过程。

`clusterReadHandler()`函数底层就是一个`read()`函数，代码如下：
```
void clusterReadHandler( aeEventLoop *el, int fd, void *privdata, int mask )
{
	char		buf[sizeof(clusterMsg)];
	ssize_t		nread;
	clusterMsg	*hdr;
	clusterLink	*link = (clusterLink *) privdata;
	unsigned int	readlen, rcvbuflen;
	UNUSED( el );
	UNUSED( mask );

	/* 循环从fd读取数据 */
	while ( 1 ) /* Read as long as there is data to read. */
	{ /* 获取连接对象的接收缓冲区的长度，表示一次最多能多大的数据量 */
		rcvbuflen = sdslen( link->rcvbuf );
		/* 如果接收缓冲区的长度小于八字节，就无法读入消息的总长 */
		if ( rcvbuflen < 8 )
		{
			readlen = 8 - rcvbuflen;
			/* 能够读入完整数据信息 */
		} else {
			hdr = (clusterMsg *) link->rcvbuf;
			/* 如果是8个字节 */
			if ( rcvbuflen == 8 )
			{
				/* 如果前四个字节不是"RCmb"签名，释放连接 */
				if ( memcmp( hdr->sig, "RCmb", 4 ) != 0 ||
				     ntohl( hdr->totlen ) < CLUSTERMSG_MIN_LEN )
				{
					serverLog( LL_WARNING,
						   "Bad message length or signature received "
						   "from Cluster bus." );
					handleLinkIOError( link );
					return;
				}
			}
			/* 记录已经读入的内容长度 */
			readlen = ntohl( hdr->totlen ) - rcvbuflen;
			if ( readlen > sizeof(buf) )
				readlen = sizeof(buf);
		}
		/* 从fd中读数据 */
		nread = read( fd, buf, readlen );
		/* 没有数据可读 */
		if ( nread == -1 && errno == EAGAIN )
			return;                     /* No more data ready. */
		/* 读错误，释放连接 */
		if ( nread <= 0 )
		{
			serverLog( LL_DEBUG, "I/O error reading from node link: %s",
				   (nread == 0) ? "connection closed" : strerror( errno ) );
			handleLinkIOError( link );
			return;
		} else {
			/* 将读到的数据追加到连接对象的接收缓冲区中 */
			link->rcvbuf	= sdscatlen( link->rcvbuf, buf, nread );
			hdr		= (clusterMsg *) link->rcvbuf;
			rcvbuflen	+= nread;
		}

		/* 检查接收的数据是否完整 */
		if ( rcvbuflen >= 8 && rcvbuflen == ntohl( hdr->totlen ) )
		{
			/* 如果读到的数据有效，处理读到接收缓冲区的数据 */
			if ( clusterProcessPacket( link ) )
			{
				/* 处理成功，则设置新的空的接收缓冲区 */
				sdsfree( link->rcvbuf );
				link->rcvbuf = sdsempty();
			} else {
				return; /* Link no longer valid. */
			}
		}
	}
}
```
之前在介绍`clusterLink`对象时，每个连接对象都有一个`link->rcvbuf`接收缓冲区和`link->sndbuf`发送缓冲区，因此这个函数就是从`fd`将数据读到`link`的接收缓冲区，然后进行是否读完整的判断，如果完整的读完数据，就调用`clusterProcessPacket()`函数来处理读到的数据，这里会处理`MEET`消息。该函数是一个通用的处理函数，因此能够处理各种类型的消息，所列只列出处理`MEET`消息的重要部分：
```
/* 从集群中查找sender节点 */
sender = clusterLookupNode( hdr->sender );

/* 初始处理PING和MEET请求，用PONG作为回复 */
if ( type == CLUSTERMSG_TYPE_PING || type == CLUSTERMSG_TYPE_MEET )
{
	serverLog( LL_DEBUG, "Ping packet received: %p", (void *) link->node );

	/*
	 * 我们使用传入的MEET消息来设置当前myself节点的地址，因为只有其他集群中的节点在握手的时会发送MEET消息，当有节点加入集群时，或者如果我们改变地址，这些节点将使用我们公开的地址来连接我们，所以在集群中，通过套接字来获取地址是一个简单的方法去发现或更新我们自己的地址，而不是在配置中的硬设置
	 * 但是，如果我们根本没有地址，即使使用正常的PING数据包，我们也会更新该地址。 如果是错误的，那么会被MEET修改
	 * 如果是MEET消息
	 * 或者是其他消息但是当前集群节点的IP为空
	 */
	if ( type == CLUSTERMSG_TYPE_MEET || myself->ip[0] == '\0' )
	{
		char ip[NET_IP_STR_LEN];
		/* 可以根据fd来获取ip，并设置myself节点的IP */
		if ( anetSockName( link->fd, ip, sizeof(ip), NULL ) != -1 &&
		     strcmp( ip, myself->ip ) )
		{
			memcpy( myself->ip, ip, NET_IP_STR_LEN );
			serverLog( LL_WARNING, "IP address for this node updated to %s",
				   myself->ip );
			clusterDoBeforeSleep( CLUSTER_TODO_SAVE_CONFIG );
		}
	}

	/*
	 * 如果当前sender节点是一个新的节点，并且消息是MEET消息类型，那么将这个节点添加到集群中
	 * 当前该节点的flags、slaveof等等都没有设置，当从其他节点接收到PONG时可以从中获取到信息
	 */
	if ( !sender && type == CLUSTERMSG_TYPE_MEET )
	{
		clusterNode *node;
		/* 创建一个处于握手状态的节点 */
		node = createClusterNode( NULL, CLUSTER_NODE_HANDSHAKE );
		/* 设置ip和port */
		nodeIp2String( node->ip, link );
		node->port = ntohs( hdr->port );
		/* 添加到集群中 */
		clusterAddNode( node );
		clusterDoBeforeSleep( CLUSTER_TODO_SAVE_CONFIG );
	}

	/* 如果是从一个未知的节点发送过来MEET包，处理流言信息 */
	if ( !sender && type == CLUSTERMSG_TYPE_MEET )
		/* 处理流言中的 PING or PONG 数据包 */
		clusterProcessGossipSection( hdr, link );

	/* Anyway reply with a PONG */
	/* 回复一个PONG消息 */
	clusterSendPing( link, CLUSTERMSG_TYPE_PONG );
}
```
在该函数中，首先先会对消息中的签名、版本、消息总大小，消息中包含的节点信息数量等等都进行判断，确保该消息是一个合法的消息，然后就计算消息的总长度，来判断接收到的消息和读到的消息是否一致完整。

**现在，再次强调一遍，当前是以目标节点的视角处理`MEET`消息。**

目标节点调用`clusterLookupNode()`函数在目标节点视角中的集群查找`MEET`消息的发送节点`hdr->sender`，该节点就是`myself`节点，由于这是第一次两个节点之间的握手，那么`myself`节点一定在目标节点视角中的集群是找不到的，所以`sender`变量为`NULL`。

然后就进入`if`条件判断，首先目标节点会根据`MEET`消息来获取自己的地址并更新自己的地址，因为如果通过从配置文件来设置地址，当节点重新上线，地址就有可能改变，但是配置文件中却没有修改，所用通过套接字获取地址来更新节点地址是一种非常好的办法。

然后继续执行第二个`if`中的代码，第一次`MEET`消息，而且`sender`发送该消息的节点并不存在目标节点视角中的集群，所以会为发送消息的`myself`节点创建一个处于握手状态的节点，并且，将该节点加入到目标节点视角中的集群。这样一来，目标节点就知道了`myself`节点的存在。

最后就是调用`clusterSendPing()`函数，指定回复一个`PONG`消息给`myself`节点。

#### 3.2.3  `myself`节点处理 PONG 消息回复 PING 消息

`myself`在发送消息`MEET`消息之前，就已经为监听`fd`的可读消息，当目标节点处理完`MEET`消息并回复`PONG`消息之后，触发`myself`节点的可读事件，调用`clusterReadHandler()`函数来处理目标节点发送来的`PONG`消息。

这次是以`myself`节点的视角来分析处理`PONG`消息。

`clusterReadHandler()`函数就是目标节点第一次接收`myself`节点发送`MEET`消息的函数，底层是`read()`函数来将套接字中的数据读取到`link->rcvbuf`接收缓冲区中，代码在`标题3.2.2`。它最后还是调用`clusterProcessPacket()`函数来处理`PONG`消息。

但是这次处理代码的部分不同，因为`myself`节点视角中的集群可以找到目标节点，也就是说，`myself`节点已经“认识”了目标节点。
```
if ( type == CLUSTERMSG_TYPE_PING || type == CLUSTERMSG_TYPE_PONG ||
     type == CLUSTERMSG_TYPE_MEET )
{
	serverLog( LL_DEBUG, "%s packet received: %p",
		   type == CLUSTERMSG_TYPE_PING ? "ping" : "pong",
		   (void *) link->node );
	/* 如果关联该连接的节点存在 */
	if ( link->node )
	{
		/* 如果关联该连接的节点处于握手状态 */
		if ( nodeInHandshake( link->node ) )
		{
			/* sender节点存在，用该新的连接地址更新sender节点的地址 */
			if ( sender )
			{
				serverLog( LL_VERBOSE,
					   "Handshake: we already know node %.40s, "
					   "updating the address if needed.", sender->name );
				if ( nodeUpdateAddressIfNeeded( sender, link, ntohs( hdr->port ) ) )
				{
					clusterDoBeforeSleep( CLUSTER_TODO_SAVE_CONFIG |
							      CLUSTER_TODO_UPDATE_STATE );
				}
				/* 释放关联该连接的节点 */
				clusterDelNode( link->node );
				return(0);
			}
			/* 将关联该连接的节点的名字用sender的名字替代 */
			clusterRenameNode( link->node, hdr->sender );
			serverLog( LL_DEBUG, "Handshake with node %.40s completed.",
				   link->node->name );
			/* 取消握手状态，设置节点的角色 */
			link->node->flags	&= ~CLUSTER_NODE_HANDSHAKE;
			link->node->flags	|= flags & (CLUSTER_NODE_MASTER | CLUSTER_NODE_SLAVE);
			clusterDoBeforeSleep( CLUSTER_TODO_SAVE_CONFIG );
			/* 如果sender的地址和关联该连接的节点的地址不相同 */
		} else if ( memcmp( link->node->name, hdr->sender,
				    CLUSTER_NAMELEN ) != 0 )
		{
			serverLog( LL_DEBUG, "PONG contains mismatching sender ID. About node %.40s added %d ms ago, having flags %d",
				   link->node->name,
				   (int) (mstime() - (link->node->ctime) ),
				   link->node->flags );
			/* 设置NOADDR标识，情况关联连接节点的地址 */
			link->node->flags	|= CLUSTER_NODE_NOADDR;
			link->node->ip[0]	= '\0';
			link->node->port	= 0;
			/* 释放连接对象 */
			freeClusterLink( link );
			clusterDoBeforeSleep( CLUSTER_TODO_SAVE_CONFIG );
			return(0);
		}
	}

	/* 关联该连接的节点存在，且消息类型为PONG */
	if ( link->node && type == CLUSTERMSG_TYPE_PONG )
	{
		/* 更新接收到PONG的时间 */
		link->node->pong_received = mstime();
		/* 清零最近一次发送PING的时间戳 */
		link->node->ping_sent = 0;

		/*
		 * 接收到PONG回复，可以删除PFAIL（疑似下线）标识
		 * FAIL标识能否删除，需要clearNodeFailureIfNeeded()来决定
		 * 如果关联该连接的节点疑似下线
		 */
		if ( nodeTimedOut( link->node ) )
		{
			/* 取消PFAIL标识 */
			link->node->flags &= ~CLUSTER_NODE_PFAIL;
			clusterDoBeforeSleep( CLUSTER_TODO_SAVE_CONFIG |
					      CLUSTER_TODO_UPDATE_STATE );
			/* 如果关联该连接的节点已经被判断为下线 */
		} else if ( nodeFailed( link->node ) )
		{
			/* 如果一个节点被标识为FAIL，需要检查是否取消该节点的FAIL标识，因为该节点在一定时间内重新上线了 */
			clearNodeFailureIfNeeded( link->node );
		}
	}
}
```
和之前处理`MEET`消息一样，首先先会对消息中的签名、版本、消息总大小，消息中包含的节点信息数量等等都进行判断，确保该消息是一个合法的消息，然后就计算消息的总长度，来判断接收到的消息和读到的消息是否一致完整。然后处理上述部分的代码。

由于`myself`节点已经“认识”目标节点，因此`myself`节点在发送`MEET`消息时已经为集群（`myself`节点视角）中的目标节点设置了连接对象，因此会执行判断连接对象是否存在的代码`if (nodeInHandshake(link->node))`，并且在`myself`节点发送完`MEET`消息后，只取消了目标节点的`CLUSTER_NODE_MEET`标识，保留了`CLUSTER_NODE_HANDSHAKE`标识，因此会执行`if (sender)`判断。

目标节点发送过来的`PONG`消息，在消息包的头部会包含`sender`发送节点的信息，但是名字对不上号，这是因为`myself`节点创建目标节点加入集群的时候，随机给他起的名字，因为`myself`节点当时也不知道目标节点的名字，所以在集群中找不到`sender`的名字，因此这个判断会失败，调用`clusterRenameNode()`函数把它的名字改过来，这样`myself`节点就真正的认识了目标节点，重新认识。之后会将目标节点的`CLUSTER_NODE_HANDSHAKE`状态取消，并且设置它的角色状态。

然后就是执行`if (link->node && type == CLUSTERMSG_TYPE_PONG)`判断，更新接收`PONG`的时间戳，清零发送`PING`的时间戳，根据接收`PONG`的时间等信息判断目标节点是否下线，如果下线要进行故障转移等操作。

之后`myself`节点并不会立即向目标节点发送`PING`消息，而是要等待下一次时间事件的发生，在`clusterCron()`函数中，每次执行都需要对集群中所有节点进行故障检测和主从切换等等操作，因此在遍历节点时，会处理以下一种情况：
```
while ( (de = dictNext( di ) ) != NULL )
{
	if ( node->flags &
	     (CLUSTER_NODE_MYSELF | CLUSTER_NODE_NOADDR | CLUSTER_NODE_HANDSHAKE) )
		continue;

	if ( node->link && node->ping_sent == 0 &&
	     (now - node->pong_received) > server.cluster_node_timeout / 2 )
	{
		/* 给node节点发送一个PING消息 */
		clusterSendPing( node->link, CLUSTERMSG_TYPE_PING );
		continue;
	}
}
```
首先跳过操作`myself`节点和处于握手状态的节点，在`myself`节点重新认识目标节点后，就将目标节点的握手状态取消了，因此会对目标节点做下面的判断操作。

当`myself`节点接收到`PONG`就会将目标节点`node->ping_sent`设置为`0`，表示目标节点还没有发送过`PING`消息，因此会发送`PING`消息给目标节点。

当发送了这个`PING`消息之后，节点之间的握手操作就完成了。之后每隔`1s`都会发送`PING`包，来进行故障检测等工作。

#### 3.2.4 Gossip协议

[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)

搭建`Redis Cluster`时，首先通过`CLUSTER MEET`命令将所有的节点加入到一个集群中，但是并没有在所有节点两两之间都执行`CLUSTER MEET`命令，那么因为节点之间使用`Gossip`协议进行工作。

`Gossip`  翻译过来就是流言，类似与病毒传播一样，只要一个人感染，如果时间足够，那么和被感染的人在一起的所有人都会被感染，因此随着时间推移，集群内的所有节点都会互相知道对方的存在。

> 关于Gossip介绍可以参考：[Gossip 算法](http://www.cnblogs.com/xingzc/p/6165084.html)

在`Redis`中，节点信息是如何传播的呢？答案是通过发送`PING`或`PONG`消息时，会包含节点信息，然后进行传播的。

我们先介绍一下`Redis Cluster`中，消息是如何抽象的。一个消息对象可以是`PING`、`PONG`、`MEET`，也可以是`UPDATE`、`PUBLISH`、`FAIL`等等消息。他们都是`clusterMsg`类型的结构，该类型主要由消息包头部和消息数据组成。

-   **消息包头部**包含签名、消息总大小、版本和发送消息节点的信息。
-   **消息数据**则是一个联合体`union clusterMsgData`，联合体中又有不同的结构体来构建不同的消息。

`PING`、`PONG`、`MEET`属于一类，是`clusterMsgDataGossip`类型的数组，可以存放多个节点的信息，该结构如下：
```
typedef struct {
	/* 节点名字 */
	char nodename[CLUSTER_NAMELEN];
	/* 最近一次发送PING的时间戳 */
	uint32_t ping_sent;
	/* 最近一次接收PONG的时间戳 */
	uint32_t pong_received;
	/* 节点的IP地址 */
	char ip[NET_IP_STR_LEN];        /* IP address last time it was seen */
	/* 节点的端口号 */
	uint16_t port;                  /* port last time it was seen */
	/* 节点的标识 */
	uint16_t flags;                 /* node->flags copy */
	/* 未使用 */
	uint16_t	notused1;       /* Some room for future improvements. */
	uint32_t	notused2;
} clusterMsgDataGossip;
```
在`clusterSendPing()`函数中，首先就是会将随机选择的节点的信息加入到消息中。代码如下：
```
void clusterSendPing( clusterLink *link, int type )
{
	unsigned char	*buf;
	clusterMsg	*hdr;
	int		gossipcount = 0;        /* Number of gossip sections added so far. */
	int		wanted;                 /* Number of gossip sections we want to append if possible. */
	int		totlen;                 /* Total packet length. */
	/*
	 * freshnodes 的值是除了当前myself节点和发送消息的两个节点之外，集群中的所有节点
	 * freshnodes 表示的意思是gossip协议中可以包含的有关节点信息的最大个数
	 */
	int freshnodes = dictSize( server.cluster->nodes ) - 2;
	/*
	 * wanted 的值是集群节点的十分之一向下取整，并且最小等于3
	 * wanted 表示的意思是gossip中要包含的其他节点信息个数
	 */
	wanted = floor( dictSize( server.cluster->nodes ) / 10 );
	if ( wanted < 3 )
		wanted = 3;
	/* 因此 wanted 最多等于 freshnodes。 */
	if ( wanted > freshnodes )
		wanted = freshnodes;

	/* 计算分配消息的最大空间 */
	totlen	= sizeof(clusterMsg) - sizeof(union clusterMsgData);
	totlen	+= (sizeof(clusterMsgDataGossip) * wanted);
	/* 消息的总长最少为一个消息结构的大小 */
	if ( totlen < (int) sizeof(clusterMsg) )
		totlen = sizeof(clusterMsg);
	/* 分配空间 */
	buf	= zcalloc( totlen );
	hdr	= (clusterMsg *) buf;

	/* 设置发送PING命令的时间 */
	if ( link->node && type == CLUSTERMSG_TYPE_PING )
		link->node->ping_sent = mstime();
	/* 构建消息的头部 */
	clusterBuildMessageHdr( hdr, type );

	int maxiterations = wanted * 3;
	/* 构建消息内容 */
	while ( freshnodes > 0 && gossipcount < wanted && maxiterations-- )
	{
		/* 随机选择一个集群节点 */
		dictEntry *de = dictGetRandomKey( server.cluster->nodes );
		clusterNode *this = dictGetVal( de );
		clusterMsgDataGossip	*gossip;
		int			j;

		/* 1. 跳过当前节点，不选myself节点 */
		if ( this == myself )
			continue;

		/* 2. 偏爱选择处于下线状态或疑似下线状态的节点 */
		if ( maxiterations > wanted * 2 &&
		     !(this->flags & (CLUSTER_NODE_PFAIL | CLUSTER_NODE_FAIL) ) )
			continue;

		/* 以下节点不能作为被选中的节点： */


		/*
		 *  1. 处于握手状态的节点
		 *  2. 带有NOADDR标识的节点
		 *  3. 因为不处理任何槽而断开连接的节点
		 */
		if ( this->flags & (CLUSTER_NODE_HANDSHAKE | CLUSTER_NODE_NOADDR) ||
		     (this->link == NULL && this->numslots == 0) )
		{
			freshnodes--; /* Tecnically not correct, but saves CPU. */
			continue;
		}

		/* 如果已经在gossip的消息中添加过了当前节点，则退出循环 */
		for ( j = 0; j < gossipcount; j++ )
		{
			if ( memcmp( hdr->data.ping.gossip[j].nodename, this->name,
				     CLUSTER_NAMELEN ) == 0 )
				break;
		}
		/* j 一定 == gossipcount */
		if ( j != gossipcount )
			continue;

		/* Add it */
		/* 这个节点满足条件，则将其添加到gossip消息中 */
		freshnodes--;
		/* 指向添加该节点的那个空间 */
		gossip = &(hdr->data.ping.gossip[gossipcount]);
		/* 添加名字 */
		memcpy( gossip->nodename, this->name, CLUSTER_NAMELEN );
		/* 记录发送PING的时间 */
		gossip->ping_sent = htonl( this->ping_sent );
		/* 接收到PING回复的时间 */
		gossip->pong_received = htonl( this->pong_received );
		/* 设置该节点的IP和port */
		memcpy( gossip->ip, this->ip, sizeof(this->ip) );
		gossip->port = htons( this->port );
		/* 记录标识 */
		gossip->flags		= htons( this->flags );
		gossip->notused1	= 0;
		gossip->notused2	= 0;
		/* 已经添加到gossip消息的节点数加1 */
		gossipcount++;
	}

	/* 计算消息的总长度 */
	totlen	= sizeof(clusterMsg) - sizeof(union clusterMsgData);
	totlen	+= (sizeof(clusterMsgDataGossip) * gossipcount);
	/* 记录消息节点的数量到包头 */
	hdr->count = htons( gossipcount );
	/* 记录消息节点的总长到包头 */
	hdr->totlen = htonl( totlen );
	/* 发送消息 */
	clusterSendMessage( link, buf, totlen );
	zfree( buf );
}
```
重点关注这几个变量：

-   `freshnodes`  
    -   `int freshnodes = dictSize(server.cluster->nodes)-2;`
    -   `freshnodes`的值是除了当前myself节点和发送消息的两个节点之外，集群中的所有节点。
    -   `freshnodes`  表示的意思是gossip协议中可以包含的有关节点信息的最大个数
-   `wanted`  
    -   `wanted = floor(dictSize(server.cluster->nodes)/10);`
    -   `wanted`  的值是集群节点的十分之一向下取整，并且最小等于3。
    -   `wanted`  表示的意思是`gossip`中要包含的其他节点信息个数。

`Gossip`协议包含的节点信息个数是`wanted`个，`wanted`  的值是集群节点的十分之一向下取整，并且最小等于3。为什么选择**十分之一**，这是因为`Redis Cluster`中计算故障转移超时时间是`server.cluster_node_timeout*2`，因此如果有节点下线，就能够收到大部分集群节点发送来的下线报告。

**十分之一的由来**：如果有`N`个主节点，那么`wanted`就是`N/10`，我们认为，在一个`node_timeout`的时间内，我们会接收到任意一个节点的4个消息包，因为，发送一个消息包，最慢被接收也不过`node_timeout/2`的时间，如果超过这个时间，那么接收回复的消息包就会超时，所以一个`node_timeout`时间内，当前节点会发送两个`PING`包，同理，接收当前节点的`PING`包，也会发送两个`PING`包给当前节点，并且会回复两个`PONG`包，这样一来，在一个`node_timeout`时间内，当前节点就会接收到4个包。

但是`Redis Cluster`中计算故障转移超时时间是`server.cluster_node_timeout*2`，是两倍的`node_timeout`时间，那么当前节点会接收到8个消息包。

因为`N`个主节点，那么`wanted`就是`N/10`，所以收到集群下线报告的概率就是`8*N/10`，也就是`80％`，这样就收到了大部分集群节点发送来的下线报告。

然后计算消息的总的大小，也就是`totlen`变量，消息包头部加上`wanted`个节点信息。

为消息分配空间，并调用`clusterBuildMessageHdr()`函数来构建消息包头部，将发送节点的信息填充进去。

接着使用`while`循环，选择`wanted`个集群节点，选择节点有一下几个特点：

-   当然不会选择`myself`节点，因为，在包头中已经包含了`myself`节点也就是发送节点的信息。
-   偏爱选择处于下线状态或疑似下线状态的节点，这样有利于进行故障检测。
-   不选，处于握手状态或没有地址状态的节点，还有就是因为不负责任何槽而断开连接的节点。

如果满足了上述条件，就会将节点的信息加入到`gossip`中，如果节点不够最少的3个，那么重复选择时会提前跳出循环。

最后，更新一下消息的总长度，然后调用`clusterSendMessage()`函数发送消息。

通过`Gossip`协议，每次能够将一些节点信息发送给目标节点，而每个节点都这么干，只要时间足够，理论上集群中所有的节点都会互相认识。

### 3.3 分配槽位

`Redis Cluster`采用槽分区，所有的键根据哈希函数映射到`0 ～ 16383`，计算公式：`slot = CRC16(key)&16383`。每一个节点负责维护一部分槽位以及槽位所映射的键值数据。

当将所有节点组成集群后，还不能工作，因为集群的节点还没有分配槽位（slot）。

分配槽位的命令`cluster addslots`，假如我们为`6379`端口的`myself`节点指定`{0..5461}`的槽位，命令如下：
```
redis-cli -h 127.0.0.1 -p 6379 cluster addslots {0..5461}
```
[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)

#### 3.3.1 槽位分配信息管理

就如上面为`6379`端口的`myself`节点指定`{0..5461}`的槽位，在`clusterNode`中，定义了该节点负责的槽位：
```
typedef struct clusterNode {
	/* 节点的槽位图 */
	unsigned char slots[CLUSTER_SLOTS / 8]; /* slots handled by this node */
	/* 当前节点复制槽的数量 */
	int numslots;                           /* Number of slots handled by this node */
} clusterNode;
```
因此，`6379`端口的`myself`节点所负责的槽，如图所示：如果节点负责该槽，那么设置为1，否则设置为0
![avatar](data:image/jpeg;base64,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cOHCJUuW4NbQUB9SqXTPnj3Lli3bu3cv37GAxXNxcdm1a1fXrl07d+4cFxfHdzgAjZFw0aJFfMcAT7qQkJBt27bJ5fJOnTrxHQtYMCcnp549e44bN653794BAQF8hwOWjWGY/v37+/r6Tpo0ydXVNSoqCo9gBagK50FCo3D9+vW+fftevXrVx8eH71jAsh04cGDKlCnr168fOHAg37GANbh+/fq0adNcXFxWrlzZokULvsMBaCywDxIaBS8vr6Kioo0bN44dO5bvWMCyhYaG9ujRY8KECUFBQeHh4XyHAxbPy8tr1qxZJSUl06dPVygU3bt3FwqFfAcFwD+cBwmNxcKFCy9durRz506+AwGL171794MHD7755purV6/mOxawBkKh8I033rhy5UpsbGxERMSJEyf4jgiAfziKDY3IuXPnRo4cefbs2WbNmvEdC1i8lJSUp59+evr06QsXLuQ7FrAeO3fufP311/v16/fNN9+4u7vzHQ4Ab7APEhqRbt26ffLJJ2PHjpXL5XzHAhavWbNmp0+f3rdv36hRowoLC/kOB6zEiBEjrl+/7urq2rJly48//hhDC55YyCChcZkzZ05UVNTUqVO1Wi3fsYDF8/X1PX78eEhISPv27U+fPs13OGAlHBwcvv3224sXL2ZlZbVo0eLDDz/EbSPhCYQMEhqdlStXlpSUzJ49G6dYQP1JJJJly5atXLly3Lhxn376KX6ZAFeCg4NXr159+fLloqKili1bvvvuu7m5uXwHBdBwkEFCo2Nra7tz585bt2698sorSCKBE4MHD7506dLx48f79u17584dvsMB6xEUFLRy5cq4uDi5XN66deu33347KyuL76AAGgIySGiMpFLp7t27k5KSRo0aVVJSwnc4YA38/PwOHTo0YsSIzp07L1y4UKFQ8B0RWI+AgIAVK1ZcvXqVZdnw8PAxY8bs378fz9kC64YMEhopZ2fnQ4cOBQYGdurUKT4+nu9wwBoIBIK33347JiYmJSWlVatWmzdv5jsisCp+fn7ffvttWlraM888s3DhwuDg4E8//TQjI4PvuADMAnfzgcZu/fr18+bN+/777ydNmsR3LGA9Tpw48eabbzo7O69YsaJNmzZ8hwNWKDY2ds2aNX///Xf37t1feOGFoUOH4lbkYE2QQYIFiI+PHzNmzKBBg5YtWyYWi/kOB6yEVqv9+eefP/300z59+rz33nsRERF8RwRWSKFQbNq0afXq1SkpKZMmTRo5cmSPHj0EAhwABIuHQQwWoG3bthcvXszOzg4PD9+/fz/f4YCVEAqFc+bMuXnzZseOHQcPHjxs2LCzZ8/yHRRYGzs7u2nTpp0+ffrEiRPOzs6vv/66v7//yy+/fPDgQbVazXd0AHWHfZBgSbZv375gwYLQ0NBly5a1bt2a73DAelRUVKxbt+6rr75q0qTJ+++//8wzz/AdEVit1NTUrVu37tixIzk5+dlnnx0zZswzzzxjZ2fHd1wApkEGCRZGrVavXr36008/HT169OLFiz08PPiOCKyHRqPZtGnTF198oVarZ8yYMX36dC8vL76DAqt17969HTt2bN++PTo6uk+fPgMGDOjfvz9+G4OlQAYJFqm4uHjJkiW///777NmzX3/9dR8fH74jAqty7ty51atX79ixo0+fPi+88MIzzzyDayDAfAoKCg4fPnzkyJHDhw8rlcr+/fv369evf//+gYGBfIcGUCNkkGDB0tLSvvnmm/Xr148aNeqtt97Cb3fglkwm27Bhw7p169LT059//vlx48ZFRUXxHRRYudTU1CNHjhw9evTo0aMuLi76bLJXr17e3t58hwbwEGSQYPEKCwtXrVr1448/tmvXbv78+QMGDOA7IrA28fHx69ev198/csyYMaNHj+7SpQvDMHzHBdaMZdn4+Hh9Knn69GkXF5euXbt26dKla9eukZGRNjY2fAcITzpkkGAlKioq/vrrr+XLlwuFwtdee23y5MlSqZTvoMDaxMXFbdu2bdu2bcXFxSNHjhw2bFjPnj0x0sDcWJZNTk6+cOFCdHT0uXPnbty40aZNG31C2aVLl5CQEL4DhCcRMkiwKizLHj16dMWKFadPn545c+acOXOaNm3Kd1BghZKTk7dt27Zv376YmJioqKi+ffv26dOna9euyCahAcjl8suXL1+4cOHcuXMXLlyoqKiIiIiIiIho165du3btwsPDJRIJ3zGC9XuSMkiWzn5Bv2fSG99Qa1u+g3lYzgH6eJuB11/6ltrbP9KOcvpzEZ0ppWc+otFNqryhpRtHacdRSikkElPT9jR6LLV0uv9m+W3atZMuJlO5jgT21KI9DR9JzRz4brnZ3L59e+XKlevWrevZs+f8+fN79OjBd0RgneRy+dmzZ48fP378+PHY2NjKbLJbt264Pws0jOzs7Li4uLi4uKtXr8bFxaWmpoaEhOgTSn1miWsNwRyQQdZIm0OfLqYun9Cz5j99OW0LfXaIOoygFlX3X4ioTVfyEFVvRcKf9P0ZIno4g9RR/F/04xnyiKR+bUmbQQePUZkvffgBBUpIfoOWLqM8O+ral5q5UfFNOniB1H704fsUaNW/VOVy+fr167/++msfH58PP/wQN/kDsyovLz937hyySeCXSqW6du1afHy8PqGMi4tjWbZly5ZhYWFhYWH6/4SEhIhEovrXBU8yDKAald+hbG0D1VVRRkQU1Ys6OT4uqkT67QyFtKHUaw+XcJv+PEMuveiDyWTPEBG1daGvjtDVHAr0p/ObKI+h596nAfpsuCeFSWjZKTqYSi+ENWy3NiypVDp79uyZM2f+888/b7/99scff/zhhx8OGzYM10CAOdjb2w8YMEB/LVdlNvnxxx/HxsZGRkbqL4CIjIxs1aoV7g0E5iORSNq3b9++ffvKV/Ly8hITE5OTk5OTk48fP56cnHzv3r2mTZtW5pShoaEhISG+vr58xw6WxMz7IFk6/xX9Vkhvz6GYbXQuiZREUh/qN56GtCYBEenoyBLaVEzzP6fmDy4sKzhGH2ykLm/TzOb3v/7mS3R+I124Szoi73Y0ZRrZX6W/99CNAiIbCh9IM4aQo4CISFdKh7bSyTjKVxAROQVQ10E0tCPZMAb2QZYm0/bddOUWKVgSSCk0ioaNohaODz5550ErRDR3GbVW1VzyI62Wl5PWYL8yZCMlySPPkry2glYk0dxl1KbWnaP649eXguiNcPpq40P7IJN+peXRNGkpPeX+yNd0lHyO7rLUo8f95JKI5LH01v+o6TR6tzs9IcmUTqfbsWPH0qVL1Wr1119//fTTT/MdETwpysvLz58/f/HixStXrsTGxt67dy88PDwyMjIqKioyMrJdu3bYQwkNrKKi4saNG0lJSfp/U1JSUlNTS0pKgg1xcXHhO15ojMy+D1IkICqhDb9S1Gj69GVi82nLD7RnFQV8RlGP299W+fWNG+ipCTTOjzJP0Hc76KfvycWRnnuDXrGjq5votz20pTnNCCPS0KEfaNs96jmaIv2JZJR0hg6uoSIbmtWuep5Ufp2W/kAlfjRqFgW7UHEq7dpOy27Q/I+ouS1FzKJp2+j3K9TnJerhRV5CE0rWFdG371N6DS0a/DGN8H/4JZbkCiIJSRgiltQqEkpIYCgxTdxKZypo+mRyi3/4LQ0lpRJ5UXPn+59kmSpRCahlD2r58DeK0ogl8vN6UtJHIhIIBKNHjx49evTOnTtfffXVDh06fP/997jFGjQAe3v7/v379+/fX/+nTCa7evXq5cuXo6Ojf/nll6SkpJCQkMgHoqKi3Nzc+A4ZrJyNjU3btm3btm1b9UWFQpGamnr7gdOnT+v/IxQKmzZtGhISUu1fW9tGdkkBNKwGOYrNkvtwGh5JDBE1oaEDKHobXckyKoPUfz1gFPUJJYaoeR9qvZeuptHozyncjYiowzO06yLdTCB1GInkdCWdpF1o3ADS79BsG0n+B0glJpYeTpU0dGIzFYloxhvU1YWIiJpTUxF9tIm2X6b5Pcjei3wciYhc/CnQm9hSo0smEjjS8/OoXGeoLQLyfvQhfCzJKoi0dHIt/RJLZURE5N2Gho+jDt7/Fa4/ft1qKnVxpZJqJWgos5gomOSX6Lu9lJhLROQRRkPHU1c/AzmiOov+PkCCpvRM04ZY/o3NiBEjnn766U8//bRdu3afffbZrFmzcFAbGpKDg0P37t27d++u/1OtVickJMTExFy5cmXnzp1xcXHu7u5t2rRp3bp1eHh469atW7dujZ2U0ADs7OzCw8PDw8OrvV5QUFCZVsbHx+/atev27dt37951d3ev3E/ZtGnTJk2aNGnSJCgoCMP1CdFA50F2bPFfHuPgQURUqjTh620DH3xdSC52RM4U8uASY6GUpESFZaQjYmzI155ux9L+K9SvLTmKiMTUfaiBAnVlFJtFTAsKd/7vRbe25LeJ0mKpogdVG/7Gl0xEJKamrUzpHR2VKYkqKFVDg2aSjx3lJtH+I7T6Jqk+oe7uRERsOW39jRRhNKUbCR4tQUsKDdEdWllMTz1DAz2p/C7t20HrPifdIurx8HHtinv06zK6aUezXiLvJ/U8WDs7u6VLl06cOHH27Nl//vnnL7/80rJly/oXC1AHYrFYf8HsjBkziIhl2dTU1KtXryYmJu7bt2/ZsmU3btzw9fWtmlO2atUKG2loMO7u7u7u7h07dqz6ok6nu3fvXmVmeerUqfT09PT09LS0NAcHh4CAgCZNmgQGBgYEBAQEBOjzSz8/P7FYzHdrgDMNkkEIyL7KmNHv7jHh9EsB2Ymq/kUkJkllQqovTb/Dz4bGvkIlq+nfVfQvkVcohbejHj2pySM3xNHKSEYk9ahSDpHQgRwYuldCSh3ZVUvTjC65LoQ0cD71IHJxIZE+nnYUFUgLf6PtB6njRJKwlLCVzshp8hRyFxgqgSEhQ6SlUW9SL32+GEatvOmj/9Huo9T5Oars/sIrtOIXuudJL71J7V25CN6StW3b9uzZsytXruzZs+dXX32l334D8IthmGbNmjVr1mzUqFH6V7RabUpKyrVr1xITE/fu3fvNN98kJycHBAToE0p9TolbAEIDEwgE+uywV69e1d7Kzc3NyMjIyMi4e/duRkZGfHx8Wlra3bt3c3JyPDw8KtPKoKCgJg/4+PjgWJDFsbZ9UPbN6fUvKDeF4q/R9Wt0bBsd20VD36ahIUac8MfWlteaULKpV9IwJHWlarchdm1LwQzduE3lOlIn0roz1HwcdXCkigoiIqWaiEijoooKEtuQQEhuUiIpNauyS9WxOQUydCOVlCyJGSKW7h6kb7eRqC198AIF4fQVIiISCASvvfbawIEDhw8ffvXq1W+++QYXyUJjIxQKW7Ro0aJFi8pXNBpNampqfHx8YmLirl27vvjii5SUlODg4DZt2kRERISHh7dt2zY4OBibZOCFl5eXl5dX1YvB9bRabXZ2tj6tzMjIuHPnzsmTJ9PT0zMyMgoLC/39/QMDA4Me0P8/MDAQZ1s2WvxnkAxDxFLVdKtCVr8SBeTVnPo3p/6jSJ5Ky7+ivVuo1zvkUrXZjuRIVJhHKva/i6m1Miojkrg+sgPysSU/vJY2+UoaIq2SynXkKP0vGWU1pGaJRCRiKPsylRKVbqK3Nj30rSNf0RH9Xcel1NyHzmaRTPvfImV1pGWJEZGAIWIp7SB9vY28+tPrY8mlpgY+qVq2bHnhwoUJEyaMGDFi8+bNOD4IjZxIJKqWU6pUqqSkpGvXrl29evWXX365fv16YWGhPpVs27ZtmzZt2rVr5+7uXo86AepLKBT6+/v7+/s/+lZFRUVGRkZ6evrdu3fv3Llz5syZDRs2pKWlpaenu7m5VWaT+uSyadOmgYGBzs7OpocAXOI7g2TI0Y5IQaUqun+JipYS4upYmPIubTlAIaOou+f9V6RNqJkTpSlJ/fAFL4wjdQygO7cpvpi6PziYmxdL2UStOtwPRP95rc60ksn0K2l0+fTFh5TelBa/89+JiVnn6DZRQCRJGfIfRPO7PpRkl8bS6iPU8Xl6ypf87YgYat2LROvoYCyFdrl/omTRVbpL5NeWbInK4umHbeTan95+7r8b+kBVLi4ue/bsmT59+siRI7dv345n04FlkUgk+ifaTZo0Sf9KcXHxtWvXrl27FhcXt2nTpmvXrkml0rZt23bo0EH/MGXciAAaDxsbG/3JG9VeZ1k2Kyvr7t27d+/eTUtLS0xMPHDgwO3bt9PS0gQCQWVCqc8vAwMDg4ODvby8+G7Nk4L/DDIoiugm7dlHfs+Qk4biD9BploioDreplDhQThydSqGsQdTciwQqSouh4yUU/DS5Ch4uUUg9xtPxZfTn9yQbSk0dqPAW7dpNTCCNibyfEEpdiYiij5J3GHl711pyNSZeSSNwp9G96buT9Pk31L8zuYkpJ4mOXiLypQk9SUgk9KLmD8+IomwiIvem1OLB/SBdOtKIk7R1LX1/mzoFkTyNDh0ljSuN6UFCFe3bQGUMRbnQxRMPlWPjQ53DCEdt9UQi0e+//z5jxoyRI0fu3LkTeyLBorm4uPTs2bNnz56Vr6SlpV29ejUmJuZ///vfjBkzXFxcuj4QGRmJSxygEWIYxs/Pz8/Pr1u3btXeKiws1J9eqd9tee7cubS0tDt37lRUVIRWoU9MDe74hHriO4Mk8upNL+TR9pO0+AiRlCL60UvDaclKUpv+PBiBO815j3bvoPNb6aCaiMjRj/o8T8N7GmintAW9N5+27aS9a0jJktCRWg2gOcMo4MHJ6F7dqFc0nTpJay7SxA9NKNlkDLWaSPP9aO9x2vcPaYkkLhT+DA0bTAHGn/4hpoFvkNMu2n+e/jxGZEOh3Wn2KGphT2wppRYREZ3cWv1LNu0pChlkFUKh8LfffpsyZcrMmTM3bNiA08jAmuh30gwdOpSIWJZNSkqKjo4+e/bsr7/+mpKSEhER0bVr127dunXt2hWbW2j83Nzc3NzcIiMjq71eXFx864ETJ06sWbPm1q1bMplMn1C2bNlSfzeDsLAwe3tOLoZ9cj1Jz8UGMI5SqezTp8/w4cM/+OADvmMBaAhlZWWXL18+c+ZMdHT0uXPnbG1tu3bt2rdv34EDB4aGhvIdHUB9lZWVpaSk3Lx588aNG9evX09MTExKSvLx8dHfyiAsLCw8PDwsLAznVpoEGSSAAZmZmd27d//5558HDRrEdywADS0lJeXs2bMnTpw4cOCASCQaNGjQoEGD+vfv7+DgwHdoANzQarW3b99OSEhITExMTEzUp5UeHh4RERHt27ePiorq0KEDdsbXDhkkgGGnTp0aP3781atXPTw86l8agIWKj4/fv3//oUOHLly40KlTp+HDhz/33HO+vr58xwXAMZZlb9++HRMTExsbe/ny5StXruh0ug4dOkRGRnbo0CEqKurRC32ecMggAWo0f/78e/fubdiwge9AAPgnl8sPHz68devWPXv2REREjB8/fsyYMfh9BVYsMzPzypUr+ieOxsTElJWV9erVq2fPnj169OjUqRMuPkMGCVAjhUIRERGxbNmyYcOG8R0LQGNRUVGxf//+f/75Z9++fV26dJk6deqYMWNsbGz4jgvAvLKzs0+dOnX69OnTp0/fuHGjQ4cOvXr16tGjR48ePRwdHfmOjgfIIAFqc/z48enTpycnJ2MDCVCNQqHYvXv3r7/+Ghsb+8ILL7z00kuBgYF8BwXQEMrKys6ePXvmzJlTp05dunQpLCxs4MCBQ4YM6dq165PzYDNkkACPMWLEiAEDBsydO5fvQAAaqZs3b65cufKPP/7o06fPnDlz+vXrhzthwZNDrVZHR0f/+++/+/btS0tLe+aZZwYPHjxo0CCrfwoUMkiAx4iLixs0aFBKSgoeVANQi/Ly8r/++uvHH3+0tbX99NNPcR8DeAJlZmbu27dv7969x44da9269dChQ4cPH96mTRu+4zILZJAAjzd+/PgOHTosWLCA70AAGjuWZbdv375w4UIPD4+lS5f26NGD74gAeKBSqU6cOLFv377Nmze7urpOmzZt4sSJfn5+fMfFJWSQAI8XHx//7LPP3r59GxffARhDp9P9+eefixYtatOmzWeffdauXTu+IwLgB8uyp06d+uOPP7Zv396pU6cpU6aMGjXKOo5oCepfBIDVa9u2bWho6K5du/gOBMAyCASCadOmJScnP/3004MGDXrvvfcqKir4DgqABwzD9O7de82aNRkZGTNmzNiwYYO/v/+MGTNOnjzJd2j1bhr2QQIYY8OGDevWrTt48CDfgQBYmNzc3FdffTUhIWHdunWdOnXiOxwAnuXm5q5fv3716tX29vbvvvvu6NGjBQKL3J2HDBLAKBUVFYGBgefPnw8ODuY7FgDL888//8ybN2/GjBmffPIJ7o0F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)
每个节点会维护自己所负责的槽位的信息。那么在管理集群状态`clusterState`的结构中，也有对应的管理槽位的信息：
```
typedef struct clusterState {
	/* 导出槽数据到目标节点，该数组记录这些节点 */
	clusterNode *migrating_slots_to[CLUSTER_SLOTS];
	/* 导入槽数据到目标节点，该数组记录这些节点 */
	clusterNode *importing_slots_from[CLUSTER_SLOTS];
	/* 槽和负责槽节点的映射 */
	clusterNode *slots[CLUSTER_SLOTS];
	/* 槽映射到键的跳跃表 */
	zskiplist *slots_to_keys;
} clusterState;
```
-   `migrating_slots_to`是一个数组，用于重新分片时保存：从当前节点**导出**的槽位的到负责该槽位的节点的映射关系。
-   `importing_slots_from`是一个数组，用于重新分片时保存：往当前节点**导入**的槽位的到负责该槽位的节点的映射关系。
-   `slots`是一个数组，保存集群中所有主节点和其负责的槽位的映射关系。
-   `slots_to_keys`是一个跳跃表，用于`CLUSTER GETKEYSINSLOT`命令可以返回多个属于槽位的键，通过遍历跳跃表实现。

#### 3.3.2 分配槽位剖析

由客户端发起命`cluster addslots <slot> [slot ...]`

当节点接收到客户端的`cluster addslots`命令后会调用对应的函数来处理命令，该命令的执行函数是`clusterCommand()`函数，该函数能够处理所有的`cluster`命令，因此我们列出处理`addslots`选项的代码：
```
if ( (!strcasecmp( c->argv[1]->ptr, "addslots" ) ||
      !strcasecmp( c->argv[1]->ptr, "delslots" ) ) && c->argc >= 3 )
{
	int		j, slot;
	unsigned char	*slots = zmalloc( CLUSTER_SLOTS );
	/* 删除操作 */
	int del = !strcasecmp( c->argv[1]->ptr, "delslots" );

	memset( slots, 0, CLUSTER_SLOTS );
	/* 遍历所有指定的槽 */
	for ( j = 2; j < c->argc; j++ )
	{
		/* 获取槽位的位置 */
		if ( (slot = getSlotOrReply( c, c->argv[j] ) ) == -1 )
		{
			zfree( slots );
			return;
		}
		/* 如果是删除操作，但是槽没有指定负责的节点，回复错误信息 */
		if ( del && server.cluster->slots[slot] == NULL )
		{
			addReplyErrorFormat( c, "Slot %d is already unassigned", slot );
			zfree( slots );
			return;
			/* 如果是添加操作，但是槽已经指定负责的节点，回复错误信息 */
		} else if ( !del && server.cluster->slots[slot] )
		{
			addReplyErrorFormat( c, "Slot %d is already busy", slot );
			zfree( slots );
			return;
		}
		/* 如果某个槽已经指定过多次了（在参数中指定了多次），那么回复错误信息 */
		if ( slots[slot]++ == 1 )
		{
			addReplyErrorFormat( c, "Slot %d specified multiple times",
					     (int) slot );
			zfree( slots );
			return;
		}
	}
	/* 上个循环保证了指定的槽的可以处理 */
	for ( j = 0; j < CLUSTER_SLOTS; j++ )
	{
		/* 如果当前槽未指定 */
		if ( slots[j] )
		{
			int retval;

			/* 如果这个槽被设置为导入状态，那么取消该状态 */
			if ( server.cluster->importing_slots_from[j] )
				server.cluster->importing_slots_from[j] = NULL;
			/* 执行删除或添加操作 */
			retval = del ? clusterDelSlot( j ) :
				 clusterAddSlot( myself, j );
			serverAssertWithInfo( c, NULL, retval == C_OK );
		}
	}
	zfree( slots );
	/* 更新集群状态和保存配置 */
	clusterDoBeforeSleep( CLUSTER_TODO_UPDATE_STATE | CLUSTER_TODO_SAVE_CONFIG );
	addReply( c, shared.ok );
}
```
首先判断当前操作是删除还是添加。

其次判断指定要加入的槽位值是否合法，符合以下条件：

-   如果是删除操作，但是槽位没有指定负责的节点，回复错误信息。
-   如果是添加操作，但是槽位已经指定负责的节点，回复错误信息。
-   如果某个槽位值已经指定过多次了（在参数中指定了多次），那么回复错误信息。

最后遍历所有参数中指定的槽位值，调用`clusterAddSlot()`将槽位指派给`myself`节点。这个函数比较简单，代码如下：
```
int clusterAddSlot( clusterNode *n, int slot )
{
	/* 如果已经指定有节点，则返回C_ERR */
	if ( server.cluster->slots[slot] )
		return(C_ERR);
	/* 设置该槽被指定 */
	clusterNodeSetSlotBit( n, slot );
	/* 设置负责该槽的节点n */
	server.cluster->slots[slot] = n;
	return(C_OK);
}
```
`clusterNodeSetSlotBit()`会将`myself`节点槽位图中对应参数指定的槽值的那些位，设置为1，表示这些槽位由`myself`节点负责。源码如下：
```
int clusterNodeSetSlotBit( clusterNode *n, int slot )
{
	/* 查看slot槽位是否被设置 */
	int old = bitmapTestBit( n->slots, slot );
	/* 将slot槽位设置为1 */
	bitmapSetBit( n->slots, slot );
	/* 如果之前没有被设置 */
	if ( !old )
	{
		/* 那么要更新n节点负责槽的个数 */
		n->numslots++;
		/*
		 * 如果主节点是第一次指定槽，即使它没有从节点，也要设置MIGRATE_TO标识
		 * 当且仅当，至少有一个其他的主节点有从节点时，主节点就是有效的迁移目标
		 */
		if ( n->numslots == 1 && clusterMastersHaveSlaves() )
			/* 设置节点迁移的标识，表示该节点可以迁移 */
			n->flags |= CLUSTER_NODE_MIGRATE_TO;
	}
	return(old);
}
```
#### 3.3.3 广播节点的槽位信息

[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)

每个节点除了保存自己负责槽位的信息还要维护自己节点视角中，集群中关于槽位分配的全部信息`server.cluster->slots`，因此，需要获取每个主节点负责槽位的信息，这是通过发送消息实现的。

在调用`clusterBuildMessageHdr()`函数构建消息包的头部时，会将发送节点的槽位信息添加进入。

在调用`clusterProcessPacket()`函数处理消息包时，会根据消息包的信息，如果出现槽位分配信息不匹配的情况，会更新当前节点视角的槽位分配的信息。该函数的处理这种情况的代码如下：
```
sender = clusterLookupNode( hdr->sender );
clusterNode	*sender_master	= NULL; /* Sender or its master if slave. */
int		dirty_slots	= 0;    /* Sender claimed slots don't match my view? */

if ( sender )
{
	/*
	 * 如果sender是从节点，那么获取其主节点信息
	 * 如果sender是主节点，那么获取sender的信息
	 */
	sender_master = nodeIsMaster( sender ) ? sender : sender->slaveof;
	if ( sender_master )
	{
		/* sender发送的槽信息和主节点的槽信息是否匹配 */
		dirty_slots = memcmp( sender_master->slots,
				      hdr->myslots, sizeof(hdr->myslots) ) != 0;
	}
}
/* 1. 如果sender是主节点，但是槽信息出现不匹配现象 */
if ( sender && nodeIsMaster( sender ) && dirty_slots )
	/* 检查当前节点对sender的槽信息，并且进行更新 */
	clusterUpdateSlotsConfigWith( sender, senderConfigEpoch, hdr->myslots );

```
`sender`变量是根据消息包中提供的发送节点在`myself`节点视角的集群中查找的节点。因此发送节点负责了一些槽位之后，将这些槽位信息通过发送包发送给`myself`节点，在`myself`节点视角的集群中查找的`sender`节点则是没有设置关于发送节点的槽位信息。所以`dirty_slots`被赋值为1，表示出现了槽位信息不匹配的情况。最终会调用`clusterUpdateSlotsConfigWith()`函数更新`myself`节点视角中，集群关于发送节点的槽位信息。该函数代码如下：
```
void clusterUpdateSlotsConfigWith( clusterNode *sender, uint64_t senderConfigEpoch, unsigned char *slots )
{
	int		j;
	clusterNode	*curmaster, *newmaster = NULL;
	uint16_t	dirty_slots[CLUSTER_SLOTS];
	int		dirty_slots_count = 0;

	/*
	 * 如果当前节点是主节点，那么获取当前节点
	 * 如果当前节点是从节点，那么获取当前从节点所从属的主节点
	 */
	curmaster = nodeIsMaster( myself ) ? myself : myself->slaveof;
	/* 如果发送消息的节点就是本节点，则直接返回 */
	if ( sender == myself )
	{
		serverLog( LL_WARNING, "Discarding UPDATE message about myself." );
		return;
	}
	/* 遍历所有槽 */
	for ( j = 0; j < CLUSTER_SLOTS; j++ )
	{
		/* 如果当前槽已经被分配 */
		if ( bitmapTestBit( slots, j ) )
		{
			/* 如果当前槽是sender负责的，那么跳过当前槽 */
			if ( server.cluster->slots[j] == sender )
				continue;
			/* 如果当前槽处于导入状态，它应该只能通过redis-trib 被手动修改，所以跳过该槽 */
			if ( server.cluster->importing_slots_from[j] )
				continue;
			/* 将槽重新绑定到新的节点，如果满足以下条件 */


			/*
			 *  1. 该槽没有被指定或者新的节点声称它有一个更大的配置纪元
			 *  2. 当前没有导入该槽
			 */
			if ( server.cluster->slots[j] == NULL ||
			     server.cluster->slots[j]->configEpoch < senderConfigEpoch )
			{
				/* 如果当前槽被当前节点所负责，而且槽中有数据，表示该槽发生冲突 */
				if ( server.cluster->slots[j] == myself &&
				     countKeysInSlot( j ) &&
				     sender != myself )
				{
					/* 将发生冲突的槽记录到脏槽中 */
					dirty_slots[dirty_slots_count] = j;
					/* 脏槽数加1 */
					dirty_slots_count++;
				}
				/* 如果当前槽属于当前节点的主节点，表示发生了故障转移 */
				if ( server.cluster->slots[j] == curmaster )
					newmaster = sender;
				/* 删除当前被指定的槽 */
				clusterDelSlot( j );
				/* 将槽分配给sender */
				clusterAddSlot( sender, j );
				clusterDoBeforeSleep( CLUSTER_TODO_SAVE_CONFIG |
						      CLUSTER_TODO_UPDATE_STATE |
						      CLUSTER_TODO_FSYNC_CONFIG );
			}
		}
	}
	/* 如果至少一个槽被重新分配，从一个节点到另一个更大配置纪元的节点，那么可能发生了： */


	/*
	 *  1. 当前节点是一个不在处理任何槽的主节点，这是应该将当前节点设置为新主节点的从节点
	 *  2. 当前节点是一个从节点，并且当前节点的主节点不在处理任何槽，这是应该将当前节点设置为新主节点的从节点
	 */
	if ( newmaster && curmaster->numslots == 0 )
	{
		serverLog( LL_WARNING,
			   "Configuration change detected. Reconfiguring myself "
			   "as a replica of %.40s", sender->name );
		/* 将sender设置为当前节点myself的主节点 */
		clusterSetMaster( sender );
		clusterDoBeforeSleep( CLUSTER_TODO_SAVE_CONFIG |
				      CLUSTER_TODO_UPDATE_STATE |
				      CLUSTER_TODO_FSYNC_CONFIG );
	} else if ( dirty_slots_count )
	{
		/*
		 * 如果执行到这里，我们接收到一个删除当前我们负责槽的所有者的更新消息，但是我们仍然负责该槽，所以主节点不能被降级为从节点
		 * 为了保持键和槽的关系，需要从我们丢失的槽中将键删除
		 */
		for ( j = 0; j < dirty_slots_count; j++ )
			/* 遍历所有的脏槽，删除槽中的键- */
			delKeysInSlot( dirty_slots[j] );
	}
}
```
该函数会遍历所有槽，然后处理已经被分配的槽（通过消息得知）

-   跳过已经被`myself`节点视角下集群中的`sender`节点所负责的槽位，没必要更新。
-   跳过处于`myself`节点视角中的集群中导入状态的槽位，因为它应该被专门的工具`redis-trib`修改。

更新槽位信息的两种情况：

-   如果`myself`节点视角下集群关于该槽没有指定负责的节点，会直接调用函数指派槽位。
-   如果发送节点的配置纪元更大，表示发送节点版本更新。这种情况需要进行两个`if`判断，判断是否发生了**槽位指派节点冲突和是否检测到了故障**。  
    -   当前槽是`myself`节点负责，并且槽中还有键，但是消息中确实发送节点负责，这样就发生了槽位指派节点冲突的情况，会将发生冲突的节点保存到`dirty_slots`数组中。
    -   这种情况的处理办法是：遍历所有发生冲突的槽位，遍历`dirty_slots`数组，将发生冲突的槽位和`myself`节点解除关系，也就是从`myself`节点负责的槽位中取消负责发生冲突的槽位。因为消息中的信息的最准确的，要以消息中的信息为准。
    -   当`myself`节点是从节点，并且当前槽是`myself`从节点的主节点负责，但是消息中显示该槽属于`sender`节点，这样检测到了故障。
    -   这种情况的处理办法是：将`sender`节点作为`myself`从节点的新的主节点`newmaster = sender`。调用`clusterSetMaster()`函数将`sender`节点设置为`myself`从节点的新主节点。

两种情况，最后都需要调用`clusterAddSlot()`函数，将当前槽位指派给`myself`节点视角下的集群中的`sender`节点。这样`myself`节点就知道了发送节点的槽分配信息。

如果时间足够，每个主节点都会将自己负责的槽位信息告知给每一个集群中的其他节点，于是，集群中的每一个节点都会知道`16384`个槽分别指派给了集群中的哪个节点。

[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)
