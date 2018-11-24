
# Redis源码剖析和注释（二十七）--- Redis 故障转移流程和原理剖析
# Redis 故障转移流程和原理
## 1. 故障转移介绍

`Redis`集群自身实现了高可用。高可用首先要解决集群部分失败的场景：当集群内少量节点出现故障时通过自动故障转移保证集群可以正常对外提供服务。接下来就介绍故障转移的细节，分析故障检测和故障转移。

-   故障检测
-   故障转移

[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)

## 2. 故障检测

### 2.1 主观故障的检测

当一个节点出现问题，需要使用一种健壮的方法保证识别出节点是否发生了故障。在之前的  [Redis Cluster 通信流程深入剖析](http://blog.csdn.net/men_wen/article/details/72871618)  一文中，介绍了`Redis`的`gossip`协议，集群节点通过`PING/PONG`消息实现节点通信，消息不但可以传播节点槽信息，还可以传播主从状态、节点故障信息等。因此故障检测也是就是通过消息传播机制实现的。

首先`Redis`集群节点每隔`1s`会随机向一个最有可能发生故障的节点发送`PING`消息。执行该操作的函数是集群的定时函数`clusterCron()`。[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)
```
if ( !(iteration % 10) )
{
	int j;

	/* 随机抽查5个节点，向pong_received值最小的发送PING消息 */
	for ( j = 0; j < 5; j++ )
	{
		/* 随机抽查一个节点 */
		de			= dictGetRandomKey( server.cluster->nodes );
		clusterNode *this	= dictGetVal( de );

		/* 跳过无连接或已经发送过PING的节点 */
		if ( this->link == NULL || this->ping_sent != 0 )
			continue;
		/* 跳过myself节点和处于握手状态的节点 */
		if ( this->flags & (CLUSTER_NODE_MYSELF | CLUSTER_NODE_HANDSHAKE) )
			continue;
		/* 查找出这个5个随机抽查的节点，接收到PONG回复过去最久的节点 */
		if ( min_pong_node == NULL || min_pong > this->pong_received )
		{
			min_pong_node	= this;
			min_pong	= this->pong_received;
		}
	}
	/* 向接收到PONG回复过去最久的节点发送PING消息，判断是否可达 */
	if ( min_pong_node )
	{
		serverLog( LL_DEBUG, "Pinging node %.40s", min_pong_node->name );
		clusterSendPing( min_pong_node->link, CLUSTERMSG_TYPE_PING );
	}
```
最有可以能发生故障的节点的判断方法是：随机抽取`5`个节点，根据`pong_received`值的大小来判断，这个变量代表最后一次接收到`PONG`消息回复的时间，所以会向随机选取的`5`个节点中，最久没有接收到`PONG`消息回复的节点发送`PING`消息，来回复该节点的`PONG`消息。发送`PING`消息会更新最近一次发送`PING`消息的时间信息`ping_sent`。

这两个时间信息对于判断节点故障扮演非常重要的作用。

如果这个节点真的发生了故障，当发送了它`PING`消息后，就不会接收到`PONG`消息作为回复，因此会触发超时判断。

当前以`myself`节点为主视角，如果向一个节点发送了`PING`消息，但是在一定时间内没有收到`PONG`回复，那么会检测到该节点可能疑似下线。处理该情况的代码在`clusterCron()`函数中。
```
while ( (de = dictNext( di ) ) != NULL )
{
	clusterNode *node = dictGetVal( de );
	now = mstime(); /* Use an updated time at every iteration. */
	mstime_t delay;
	/* 跳过myself节点，无地址NOADDR节点，和处于握手状态的节点 */
	if ( node->flags &
	     (CLUSTER_NODE_MYSELF | CLUSTER_NODE_NOADDR | CLUSTER_NODE_HANDSHAKE) )
		continue;
	... ...

	/* 如果当前还没有发送PING消息，则跳过，只要发送了PING消息之后，才会执行以下操作 */
	if ( node->ping_sent == 0 )
		continue;

	/* 计算已经等待接收PONG回复的时长 */
	delay = now - node->ping_sent;
	/* 如果等待的时间超过了限制 */
	if ( delay > server.cluster_node_timeout )
	{
		/* Timeout reached. Set the node as possibly failing if it is
		 * not already in this state. */
		/* 设置该节点为疑似下线的标识 */
		if ( !(node->flags & (CLUSTER_NODE_PFAIL | CLUSTER_NODE_FAIL) ) )
		{
			serverLog( LL_DEBUG, "*** NODE %.40s possibly failing",
				   node->name );
			node->flags |= CLUSTER_NODE_PFAIL;
			/* 设置更新状态的标识 */
			update_state = 1;
		}
	}
}
```
这个循环会迭代所有的节点，来检测是否需要将某个节点标记为下线的状态。还会做一些其他的操作，例如：

-   判断孤立的主节点的个数，如果存在孤立的主节点并且某些条件满足，之后会为其迁移一个其他主节点的从节点。
-   释放回复`PONG`消息过慢（超过超时时间的一半）的节点连接，等待下个周期重新建立连接。这样做是为了连接更加健壮。
-   触发第一次`PING`消息发送。当节点第一次加入集群时，发送完`MEET`消息，也接受`PONG`回复后，会触发该条件，来执行第一次`PING`消息通信。
-   如果一个从节点请求了手动故障转移，发送给请求节点一个`PING`消息。
-   最后，则是对节点的故障检测。

如果发送`PING`消息的时间已经超过了`cluster_node_timeout`限制，默认是`15S`，那么会将迭代的该节点的`flags`打开`CLUSTER_NODE_PFAIL`标识，表示`myself`节点主观判断该节点下线。但是这不代表最终的故障判定。

### 2.2 客观故障的检测

当`myself`节点检测到一个节点疑似下线后，就会打开该节点的`CLUSTER_NODE_PFAIL`标识，表示判断该节点主观下线，但是可能存在误判的情况，因此为了真正的标记该节点的下线状态，会进行客观故障的检测。

客观故障的检测仍然依赖`PING/PONG`消息的传播，每次发送`PING/PONG`消息，总会携带集群节点个数的十分之一个节点信息，发送`PING/PONG`消息的函数`clusterSendPing()`具体代码如下：[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)
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

> **十分之一的由来**：如果有`N`个主节点，那么`wanted`就是`N/10`，我们认为，在一个`node_timeout`的时间内，我们会接收到任意一个节点的4个消息包，因为，发送一个消息包，最慢被接收也不过`node_timeout/2`的时间，如果超过这个时间，那么接收回复的消息包就会超时，所以一个`node_timeout`时间内，当前节点会发送两个`PING`包，同理，接收当前节点的`PING`包，也会发送两个`PING`包给当前节点，并且会回复两个`PONG`包，这样一来，在一个`node_timeout`时间内，当前节点就会接收到4个包。
> 
> 但是`Redis Cluster`中计算故障转移超时时间是`server.cluster_node_timeout*2`，是两倍的`node_timeout`时间，那么当前节点会接收到8个消息包。
> 
> 因为`N`个主节点，那么`wanted`就是`N/10`，所以收到集群下线报告的概率就是`8*N/10`，也就是`80％`，这样就收到了大部分集群节点发送来的下线报告。

然后计算消息的总的大小，也就是`totlen`变量，消息包头部加上`wanted`个节点信息。

为消息分配空间，并调用`clusterBuildMessageHdr()`函数来构建消息包头部，将发送节点的信息填充进去。

接着使用`while`循环，选择`wanted`个集群节点，选择节点有一下几个特点：

-   当然不会选择`myself`节点，因为，在包头中已经包含了`myself`节点也就是发送节点的信息。
-   偏爱选择处于下线状态或疑似下线状态的节点，这样有利于进行故障检测。
-   不选，处于握手状态或没有地址状态的节点，还有就是因为不负责任何槽而断开连接的节点。

如果满足了上述条件，就会将节点的信息加入到`gossip`中，如果节点不够最少的3个，那么重复选择时会提前跳出循环。

最后，更新一下消息的总长度，然后调用`clusterSendMessage()`函数发送消息。

因此，可以得知，在发送`PING/PONG`消息时，会将处于`CLUSTER_NODE_PFAIL`状态的节点处于消息的流言部分。

无论是集群中的哪个主节点接收到了消息，无论就接收到`PING`消息，还是接收到`PONG`回复，都会调用`clusterReadHandler()`函数来读取收到的消息，并且判断读取的消息合法性和完整性等等。

如果消息可读，会调用`clusterProcessPacket()`函数来处理读取到的消息。该函数能够处理所有类型的消息，但是我们主要关注处理`PING/PONG`消息包的流言部分的代码
```
if ( sender )
	clusterProcessGossipSection( hdr, link );
```
在接收节点的视角下的集群中，`sender`是消息的发送节点，如果`sender`节点处于当前集群中，那么会调用`clusterProcessGossipSection()`函数来处理流言部分的信息。
```
void clusterProcessGossipSection( clusterMsg *hdr, clusterLink *link )
{
	/* 获取该条消息包含的节点数信息 */
	uint16_t count = ntohs( hdr->count );
	/* clusterMsgDataGossip数组的地址 */
	clusterMsgDataGossip *g = (clusterMsgDataGossip *) hdr->data.ping.gossip;
	/* 发送消息的节点 */
	clusterNode *sender = link->node ? link->node : clusterLookupNode( hdr->sender );

	/* 遍历所有节点的信息 */
	while ( count-- )
	{
		/* 获取节点的标识信息 */
		uint16_t	flags = ntohs( g->flags );
		clusterNode	*node;
		sds		ci;
		/* 根据获取的标识信息，生成用逗号连接的sds字符串ci */
		ci = representClusterNodeFlags( sdsempty(), flags );
		/* 打印到日志中 */
		serverLog( LL_DEBUG, "GOSSIP %.40s %s:%d %s",
			   g->nodename,
			   g->ip,
			   ntohs( g->port ),
			   ci );
		sdsfree( ci );

		/* 根据指定name从集群中查找并返回节点 */
		node = clusterLookupNode( g->nodename );
		/* 如果node存在 */
		if ( node )
		{
			/* 如果发送者是主节点，且不是node本身 */
			if ( sender && nodeIsMaster( sender ) && node != myself )
			{
				/* 如果标识中指定了关于下线的状态 */
				if ( flags & (CLUSTER_NODE_FAIL | CLUSTER_NODE_PFAIL) )
				{
					/* 将sender的添加到node的故障报告中 */
					if ( clusterNodeAddFailureReport( node, sender ) )
					{
						serverLog( LL_VERBOSE,
							   "Node %.40s reported node %.40s as not reachable.",
							   sender->name, node->name );
					}
					/* 判断node节点是否处于真正的下线FAIL状态 */
					markNodeAsFailingIfNeeded( node );
					/* 如果标识表示节点处于正常状态 */
				} else {
					/* 将sender从node的故障报告中删除 */
					if ( clusterNodeDelFailureReport( node, sender ) )
					{
						serverLog( LL_VERBOSE,
							   "Node %.40s reported node %.40s is back online.",
							   sender->name, node->name );
					}
				}
			}

			/*
			 * 虽然node存在，但是node已经处于下线状态
			 * 但是消息中的标识却反应该节点不处于下线状态，并且实际的地址和消息中的地址发生变化
			 * 这些表明该节点换了新地址，尝试进行握手
			 */
			if ( node->flags & (CLUSTER_NODE_FAIL | CLUSTER_NODE_PFAIL) &&
			     !(flags & CLUSTER_NODE_NOADDR) &&
			     !(flags & (CLUSTER_NODE_FAIL | CLUSTER_NODE_PFAIL) ) &&
			     (strcasecmp( node->ip, g->ip ) || node->port != ntohs( g->port ) ) )
			{
				/* 释放原来的集群连接对象 */
				if ( node->link )
					freeClusterLink( node->link );
				/* 设置节点的地址为消息中的地址 */
				memcpy( node->ip, g->ip, NET_IP_STR_LEN );
				node->port = ntohs( g->port );
				/* 清除无地址的标识 */
				node->flags &= ~CLUSTER_NODE_NOADDR;
			}
			/* node不存在，没有在当前集群中找到 */
		} else {
			/*
			 * 如果node不处于NOADDR状态，并且集群中没有该节点，那么向node发送一个握手的消息
			 * 注意，当前sender节点必须是本集群的众所周知的节点（不在集群的黑名单中），否则有加入另一个集群的风险
			 */
			if ( sender &&
			     !(flags & CLUSTER_NODE_NOADDR) &&
			     !clusterBlacklistExists( g->nodename ) )
			{
				/* 开始进行握手 */
				clusterStartHandshake( g->ip, ntohs( g->port ) );
			}
		}

		/* Next node */
		/* 下一个节点 */
		g++;
	}
}
```
该函数会根据消息头中的`count`数，来遍历`count`次流言携带的节点信息，这些节点信息都是可能处于下线或疑似下线的节点。

那么首先找到节点信息所描述的集群节点，

-   **如果消息中附带的节点信息所对应的节点`node`存在**  
    -   **如果发送消息的节点是主节点，且附带节点信息的节点不是`myself`节点**
    -   **那么如果附带的节点信息显示该`node`节点处于下线或疑似下线状态，那么会调用`clusterNodeAddFailureReport()`函数将`sender`节点添加到`node`的故障报告的链表中。然后调用`markNodeAsFailingIfNeeded()`函数来判断该`node`节点是否真正的处于客观下线状态。**
    -   否则，节点则是处于正常状态，则调用`clusterNodeDelFailureReport()`函数将`sender`节点从`node`节点的故障报告链表中删除。
    -   如果`node`存在，在`myself`集群中的视角中，该节点处于下线或疑似下线的状态，但是消息中的却反馈不处于下线的状态，且节点更换了地址
    -   释放原来的节点的连接，设置消息中新提供的地址，重新尝试连接新地址。这用来处理节点重新上线的情况。
-   `node`节点不存在当前集群中  
    -   确保`sender`节点在当前集群中，防止加入另一个集群。且消息中显示`node`节点有地址，且该节点不在集群黑名单中。黑名单是在集群收缩时，将要下线的节点加入黑名单，然后让集群所有节点都忘记该节点。
    -   如果满足上面的条件，则开始进行握手操作。

我们重点关注`node`下线的情况，在`标题2.1`时，`myself`节点将疑似下线的节点设置为`CLUSTER_NODE_PFAIL`标识，因此，接收消息的节点，调用`clusterNodeAddFailureReport()`函数，将`sender`节点添加到`node`的故障报告的链表中。代码如下：
```
int clusterNodeAddFailureReport( clusterNode *failing, clusterNode *sender )
{
	/* 获取故障报告的链表 */
	list			*l = failing->fail_reports;
	listNode		*ln;
	listIter		li;
	clusterNodeFailReport	*fr;

	listRewind( l, &li );
	/* 遍历故障报告链表 */
	while ( (ln = listNext( &li ) ) != NULL )
	{
		fr = ln->value;
		/* 如果存在sender之前发送的故障报告 */
		if ( fr->node == sender )
		{
			/* 那么只更新时间戳 */
			fr->time = mstime();
			return(0);
		}
	}

	/* 否则创建新的故障报告 */
	fr = zmalloc( sizeof(*fr) );
	/* 设置发送该报告的节点 */
	fr->node = sender;
	/* 设置时间 */
	fr->time = mstime();
	/* 添加到故障报告的链表中 */
	listAddNodeTail( l, fr );
	return(1);
}
```
函数很简单，遍历下线节点的`fail_reports`故障报告链表，如果`sender`节点之前就已经报告该节点下线，那么更新报告的时间戳，否则创建新的报告，加入到该链表中。

然后调用`markNodeAsFailingIfNeeded()`函数来判断该函数是否处于客观下线状态。代码如下：
```
void markNodeAsFailingIfNeeded( clusterNode *node )
{
	int failures;
	/* 需要大多数的票数，超过一半的节点数量 */
	int needed_quorum = (server.cluster->size / 2) + 1;
	/* 不处于pfail（需要确认是否故障）状态，则直接返回 */
	if ( !nodeTimedOut( node ) )
		return;         /* We can reach it. */
	/* 处于fail（已确认为故障）状态，则直接返回 */
	if ( nodeFailed( node ) )
		return;         /* Already FAILing. */
	/* 返回认为node节点下线（标记为 PFAIL or FAIL 状态）的其他节点数量 */
	failures = clusterNodeFailureReportsCount( node );
	/* 如果当前节点是主节点，也投一票 */
	if ( nodeIsMaster( myself ) )
		failures++;
	/* 如果报告node故障的节点数量不够总数的一半，无法判定node是否下线，直接返回 */
	if ( failures < needed_quorum )
		return;     /* No weak agreement from masters. */

	serverLog( LL_NOTICE, "Marking node %.40s as failing (quorum reached).", node->name );

	/* 取消PFAIL，设置为FAIL */
	node->flags	&= ~CLUSTER_NODE_PFAIL;
	node->flags	|= CLUSTER_NODE_FAIL;
	/* 并设置下线时间 */
	node->fail_time = mstime();

	/* 广播下线节点的名字给所有的节点，强制所有的其他可达的节点为该节点设置FAIL标识 */
	if ( nodeIsMaster( myself ) )
		clusterSendFail( node->name );
	clusterDoBeforeSleep( CLUSTER_TODO_UPDATE_STATE | CLUSTER_TODO_SAVE_CONFIG );
}
```
该函数用来判断`node`节点是否处于客观下线的状态，`Redis`认为，如果集群中过半数的主节点都认为该节点处于下线的状态，那么该节点就处于客观下线的状态，因为`needed_quorum`就是计算的票数。

如果该节点不处于`FAIL`或`PFAIL`状态，则直接返回。

然后调用`clusterNodeFailureReportsCount()`函数来计算集群中有多少个认为该节点下线的主节点数量。**该函数就是将计算故障报告的链表的长度，因为如果有节点下线，那么其他正常的主节点每次发送`PING/PONG`消息时，会将下线节点附加到消息中，当接收节点处理消息时，则会将发送消息的节点加入到下线节点的故障报告链表中，这样就可以计算集群中有多少个主节点认为该节点处于下线状态。**

如果`myself`节点也是主节点，那么也有一个投票的权利。

如果所有投票的主节点个数小于需要的票数`needed_quorum`，则直接返回，表示无法判断该节点是否处于客观下线状态。

如果达到了`needed_quorum`，那么会取消`CLUSTER_NODE_PFAIL`状态，设置为`CLUSTER_NODE_FAIL`状态。并设置该节点被判断为客观下线的时间。

最后一步，就是将客观下线的节点广播给集群中所有的节点。通过发送`FAIL`消息，调用`clusterSendFail()`函数，代码如下：
```
void clusterSendFail( char *nodename )
{
	unsigned char	buf[sizeof(clusterMsg)];
	clusterMsg	*hdr = (clusterMsg *) buf;
	/* 构建FAIL的消息包包头 */
	clusterBuildMessageHdr( hdr, CLUSTERMSG_TYPE_FAIL );
	/* 设置下线节点的名字 */
	memcpy( hdr->data.fail.about.nodename, nodename, CLUSTER_NAMELEN );
	/* 发送给所有集群中的节点 */
	clusterBroadcastMessage( buf, ntohl( hdr->totlen ) );
}
```
`FAIL`消息和`PING/PONG`消息一样，首先调用`clusterBuildMessageHdr`设置消息包的头部，设置`CLUSTERMSG_TYPE_FAIL`标识，表示该消息包是一个`FAIL`消息。

该消息包的头部设置完毕后，要设置消息包的内容，`FAIL`消息包的数据部分很简单，就是`clusterMsgDataFail`类型，该结构体只包含一个成员，就是客观下线的节点名字。

最后调用`clusterBroadcastMessage()`函数将消息发送给整个集群的所有节点，该函数就是遍历集群中的所有节点，除了处于握手状态的节点和`myself`节点不发送消息，还有没有连接的节点不发送，其他所有的节点都发送`FAIL`消息。

这样一来，集群中所有节点就知道了客观下线的节点。

## 3. 故障转移

当故障节点客观下线了以后，那么就要自动选举出一个可以替代他的从节点，从而保证高可用。

当集群中的节点接收到发来的`FAIL`消息，会执行如下的代码处理：[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)
```
if ( type == CLUSTERMSG_TYPE_FAIL )
{
	clusterNode *failing;

	if ( sender )
	{
		/* 获取下线节点的地址 */
		failing = clusterLookupNode( hdr->data.fail.about.nodename );
		/* 如果下线节点不是myself节点也不是处于下线状态 */
		if ( failing &&
		     !(failing->flags & (CLUSTER_NODE_FAIL | CLUSTER_NODE_MYSELF) ) )
		{
			serverLog( LL_NOTICE,
				   "FAIL message received from %.40s about %.40s",
				   hdr->sender, hdr->data.fail.about.nodename );
			/* 设置FAIL标识 */
			failing->flags |= CLUSTER_NODE_FAIL;
			/* 设置下线时间 */
			failing->fail_time = mstime();
			/* 取消PFAIL标识 */
			failing->flags &= ~CLUSTER_NODE_PFAIL;
			clusterDoBeforeSleep( CLUSTER_TODO_SAVE_CONFIG |
					      CLUSTER_TODO_UPDATE_STATE );
		}
	} else {
		serverLog( LL_NOTICE,
			   "Ignoring FAIL message from unknown node %.40s about %.40s",
			   hdr->sender, hdr->data.fail.about.nodename );
	}
}
```
所有收到`FAIL`消息的节点，都会根据传来的客观下线的内容，将当前视角中的集群中的下线节点设置`CLUSTER_NODE_FAIL`

标识，并且设置下线时间。

然后下线节点的从节点等待下一个周期执行`clusterCron()`函数，来开始故障转移操作。具体的代码如下：
```
if ( nodeIsSlave( myself ) )
{
	/* 设置手动故障转移的状态 */
	clusterHandleManualFailover();
	/* 执行从节点的自动或手动故障转移，从节点获取其主节点的哈希槽，并传播新配置 */
	clusterHandleSlaveFailover();

	/* 如果存在孤立的主节点，并且集群中的某一主节点有超过2个正常的从节点，并且该主节点正好是myself节点的主节点 */
	if ( orphaned_masters && max_slaves >= 2 && this_slaves == max_slaves )
		/* 给孤立的主节点迁移一个从节点 */
		clusterHandleSlaveMigration( max_slaves );
}
```
如果`myself`节点是从节点，才会执行这一部分的代码。这一部分主要执行了三个动作：

-   调用`clusterHandleManualFailover`来设置手动故障转移的状态。用于执行了`CLUSTER FAILOVER [FORCE|TAKEOVER]`命令的情况。
-   调用`clusterHandleSlaveFailover()`函数来执行故障转移操作。重点关注该函数。
-   为孤立的节点迁移个从节点。满足以下条件，则可以调用`clusterHandleSlaveMigration()`函数为孤立的主节点迁移一个从节点。  
    -   存在孤立的节点，孤立的节点判断条件如下：
    -   该节点没有正常的从节点
    -   并且该节点负责一部分槽位。
    -   并且该节点处于可以将槽位导出的状态。
    -   集群中的一个主节点有超过`2`个正常的从节点。
    -   上面一个条件中的主节点正好是`myself`节点的主节点。

我们关注第二点，调用`clusterHandleSlaveFailover()`函数来执行故障转移操作。该函数可以分为这几个部分：

-   选举资格检测和准备工作
-   准备选举的时间
-   发起选举
-   选举投票
-   替换主节点
-   主从切换广播给集群

我们将`clusterHandleSlaveFailover()`函数分割为几部分，一一剖析。

### 3.1 选举资格检测和准备工作

[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)

首先函数需要判断当前执行`clusterHandleSlaveFailover()`函数的节点是否具有选举的资格。部分代码如下：
```
void clusterHandleSlaveFailover( void )
{
	mstime_t data_age;
	/* 计算上次选举所过去的时间 */
	mstime_t auth_age = mstime() - server.cluster->failover_auth_time;
	/* 计算胜选需要的票数 */
	int needed_quorum = (server.cluster->size / 2) + 1;
	/* 手动故障转移的标志 */
	int manual_failover = server.cluster->mf_end != 0 &&
			      server.cluster->mf_can_start;
	mstime_t auth_timeout, auth_retry_time;

	server.cluster->todo_before_sleep &= ~CLUSTER_TODO_HANDLE_FAILOVER;

	/* 计算故障转移超时时间 */
	auth_timeout = server.cluster_node_timeout * 2;
	if ( auth_timeout < 2000 )
		auth_timeout = 2000;
	/* 重试的超时时间 */
	auth_retry_time = auth_timeout * 2;

	/* 运行函数的前提条件，在自动或手动故障转移的情况下都必须满足： */


	/*
	 *  1. 当前节点是从节点
	 *  2. 该从节点的主节点被标记为FAIL状态，或者是一个手动故障转移状态
	 *  3. 当前从节点有负责的槽位
	 */
	/* 如果不能满足以上条件，则直接返回 */
	if ( nodeIsMaster( myself ) ||
	     myself->slaveof == NULL ||
	     (!nodeFailed( myself->slaveof ) && !manual_failover) ||
	     myself->slaveof->numslots == 0 )
	{
		/* 设置故障转移失败的原因：CLUSTER_CANT_FAILOVER_NONE */
		server.cluster->cant_failover_reason = CLUSTER_CANT_FAILOVER_NONE;
		return;
	}

	/* 如果当前节点正在和主节点保持连接状态，计算从节点和主节点断开的时间 */
	if ( server.repl_state == REPL_STATE_CONNECTED )
	{
		data_age = (mstime_t) (server.unixtime - server.master->lastinteraction)
			   * 1000;
	} else {
		data_age = (mstime_t) (server.unixtime - server.repl_down_since) * 1000;
	}

	/* 从data_age删除一个cluster_node_timeout的时长，因为至少以从节点和主节点断开连接开始，因为超时的时间不算在内 */
	if ( data_age > server.cluster_node_timeout )
		data_age -= server.cluster_node_timeout;

	/* 检查这个从节点的数据是否比较新 */
	if ( server.cluster_slave_validity_factor &&
	     data_age >
	     ( ( (mstime_t) server.repl_ping_slave_period * 1000) +
	       (server.cluster_node_timeout * server.cluster_slave_validity_factor) ) )
	{
		if ( !manual_failover )
		{
			clusterLogCantFailover( CLUSTER_CANT_FAILOVER_DATA_AGE );
			return;
		}
	}
```
函数一开始，先计算了一些之后需要的变量。

-   `auth_age`：表示距离上次选举所过去的时间。
-   `needed_quorum`：表示胜选所需要的票数。
-   `manual_failover`：手动故障转移的标识。`1`表示是手动故障转移，`0`表示是自发的故障转移。
-   `auth_timeout`：故障转移的超时时间，最少为`2s`。
-   `auth_retry_time`：重试的周期时长。

如果`myself`节点是主节点或`myself`节点没有主节点，或者主节点不处于`FAIL`状态，且不是手动故障转移，或者`myself`节点的主节点没有负责的槽位，则不能执行该函数。直接返回。

根据当前`myself`从节点是否处于复制的连接状态，进行计算数据的时间`data_age`，用来表示数据的新旧。如果处于连接状态，则计算距离最后因此与主节点交互的时间，否则计算距离复制断开的时间。

如果复制处于断开连接的状态，那么`data_age`一定会大于集群节点超时时间，因此要减去一个超时时间，用来准确的描述距离最后一次复制主节点的数据所经过的时间。

最后如果当前服务器设置了`cluster_slave_validity_factor`值，该变量表示：故障转移时，从节点最后一次复制主节点数据所经过的时间。如果`data_age`超过了规定的时间，那么表示该从节点的复制的数据太旧，太少，不具备执行故障转移的资格。

### 3.2 设置选举时间

[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)

如果当前从节点符合故障转移的资格，更新选举开始的时间，只有达到改时间才能执行后续的流程。
```
/* 如果先前的尝试故障转移超时并且重试时间已过，我们可以设置一个新的。 */
if ( auth_age > auth_retry_time )
{
	/* 设置新的故障转移属性 */
	server.cluster->failover_auth_time = mstime() +
					     500 +              /* Fixed delay of 500 milliseconds, let FAIL msg propagate. */
					     random() % 500;    /* Random delay between 0 and 500 milliseconds. */
	server.cluster->failover_auth_count	= 0;
	server.cluster->failover_auth_sent	= 0;
	server.cluster->failover_auth_rank	= clusterGetSlaveRank();
	server.cluster->failover_auth_time	+=
		server.cluster->failover_auth_rank * 1000;
	/* However if this is a manual failover, no delay is needed. */
	/* 手动故障转移的情况 */
	if ( server.cluster->mf_end )
	{
		server.cluster->failover_auth_time	= mstime();
		server.cluster->failover_auth_rank	= 0;
	}
	serverLog( LL_WARNING,
		   "Start of election delayed for %lld milliseconds "
		   "(rank #%d, offset %lld).",
		   server.cluster->failover_auth_time - mstime(),
		   server.cluster->failover_auth_rank,
		   replicationGetSlaveOffset() );
	/* 发送一个PONG消息包给所有的从节点，携带有当前的复制偏移量 */
	clusterBroadcastPong( CLUSTER_BROADCAST_LOCAL_SLAVES );
	return;
}
/* 如果没有开始故障转移，则调用clusterGetSlaveRank()获取当前从节点的最新排名。因为在故障转移之前可能会收到其他节点发送来的心跳包，因而可以根据心跳包的复制偏移量更新本节点的排名，获得新排名newrank，如果newrank比之前的排名靠后，则需要增加故障转移开始时间的延迟，然后将newrank记录到server.cluster->failover_auth_rank中 */
if ( server.cluster->failover_auth_sent == 0 &&
     server.cluster->mf_end == 0 )
{
	/* 获取新排名 */
	int newrank = clusterGetSlaveRank();
	/* 新排名比之前的靠后 */
	if ( newrank > server.cluster->failover_auth_rank )
	{
		/* 计算延迟故障转移时间 */
		long long added_delay =
			(newrank - server.cluster->failover_auth_rank) * 1000;
		/* 更新下一次故障转移的时间和排名 */
		server.cluster->failover_auth_time	+= added_delay;
		server.cluster->failover_auth_rank	= newrank;
		serverLog( LL_WARNING,
			   "Slave rank updated to #%d, added %lld milliseconds of delay.",
			   newrank, added_delay );
	}
}
/* 如果还没有到故障转移选举的时间，直接返回 */
if ( mstime() < server.cluster->failover_auth_time )
{
	clusterLogCantFailover( CLUSTER_CANT_FAILOVER_WAITING_DELAY );
	return;
}
/* 如果距离故障转移的时间过了很久，那么不在执行故障转移，直接返回 */
if ( auth_age > auth_timeout )
{
	/* 故障转移过期 */
	clusterLogCantFailover( CLUSTER_CANT_FAILOVER_EXPIRED );
	return;
}
```
如果上次选举所过去的时间`auth_age`大于`auth_retry_time`重试的周期，表示这一次故障转移超时，那么就重新设置一个。

更新一些故障操作的时间信息，然后发送一个`PONG`消息给所有集群的从节点，这些从节点和`myself`节点有同一个下线的主节点。然后就返回，等待触发下一次故障转移。

然后，处理了关于手动故障节点排名的情况。

接下来，如果没有到达故障转移的时间，直接返回。

最后，如果上次选举所过去的时间`auth_age`大于故障转移的超时时间`auth_timeout`。那么不在执行故障转移，直接返回。

### 3.3 发起选举

如果当前达到了故障转移的时间，那么就会先发起选举操作，选出一个执行故障转移的从节点。

具体代码：[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)
```
if ( server.cluster->failover_auth_sent == 0 )
{
	/* 增加当前纪元 */
	server.cluster->currentEpoch++;
	/* 设置发其故障转移的纪元 */
	server.cluster->failover_auth_epoch = server.cluster->currentEpoch;
	serverLog( LL_WARNING, "Starting a failover election for epoch %llu.",
		   (unsigned long long) server.cluster->currentEpoch );
	/* 发送一个FAILOVE_AUTH_REQUEST消息给所有的节点，判断这些节点是否同意该从节点为它的主节点执行故障转移操作 */
	clusterRequestFailoverAuth();
	/* 设置为真，表示本节点已经向其他节点发送了投票请求 */
	server.cluster->failover_auth_sent = 1;
	/* 进入下一个事件循环执行的操作，保存配置文件，更新节点状态，同步配置 */
	clusterDoBeforeSleep( CLUSTER_TODO_SAVE_CONFIG |
			      CLUSTER_TODO_UPDATE_STATE |
			      CLUSTER_TODO_FSYNC_CONFIG );
	return; /* Wait for replies. */
}
```
如果还没有向集群其他节点发起投票请求，那么将当前纪元`currentEpoch`加一，然后该当前纪元设置发起故障转移的纪元`failover_auth_epoch`。调用`clusterRequestFailoverAuth()`函数，发送一个`FAILOVE_AUTH_REQUEST`消息给其他所有集群节点，等待其他节点回复是否同意该从节点为它的主节点执行故障转移操作。最后设置`failover_auth_sent`为真，表示本节点已经向其他集群节点发送投票请求了。然后就之久返回，等待其他节点的回复。

我们看一下发送`FAILOVE_AUTH_REQUEST`消息的函数`clusterRequestFailoverAuth()`
```
void clusterRequestFailoverAuth( void )
{
	unsigned char	buf[sizeof(clusterMsg)];
	clusterMsg	*hdr = (clusterMsg *) buf;
	uint32_t	totlen;
	/* 建立REQUEST消息包包头 */
	clusterBuildMessageHdr( hdr, CLUSTERMSG_TYPE_FAILOVER_AUTH_REQUEST );
	/* 如果是一个手动的故障转移，设置CLUSTERMSG_FLAG0_FORCEACK，表示即使主节点在线，也要认证故障转移 */
	if ( server.cluster->mf_end )
		hdr->mflags[0] |= CLUSTERMSG_FLAG0_FORCEACK;
	/* 计算REQUEST消息包的长度 */
	totlen		= sizeof(clusterMsg) - sizeof(union clusterMsgData);
	hdr->totlen	= htonl( totlen );
	/* 广播这个消息包 */
	clusterBroadcastMessage( buf, totlen );
}
```
`REQUEST`消息用来从节点请求是否可以进行故障转移，因此先建立`CLUSTERMSG_TYPE_FAILOVER_AUTH_REQUEST`标识的消息包，表示这是一个`REQUEST`消息，然后将该消息广播给集群中的所有节点。

### 3.4 选举投票

[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)

当集群中所有的节点接收到`REQUEST`消息后，会执行`clusterProcessPacket()`函数的这部分代码：
```
if ( type == CLUSTERMSG_TYPE_FAILOVER_AUTH_REQUEST )
{
	if ( !sender )
		return(1);      /* We don't know that node. */
	/* 如果条件允许，向sender投票，支持它进行故障转移 */
	clusterSendFailoverAuthIfNeeded( sender, hdr );
}
```
如果发送消息包的节点`sender`不是当前集群的节点，直接返回。否则调用`clusterSendFailoverAuthIfNeeded()`函数向`sender`节点发起投票。该函数的代码如下：
```
void clusterSendFailoverAuthIfNeeded( clusterNode *node, clusterMsg *request )
{
	/* 获取该请求从节点的主节点 */
	clusterNode *master = node->slaveof;
	/* 获取请求的当前纪元和配置纪元 */
	uint64_t	requestCurrentEpoch	= ntohu64( request->currentEpoch );
	uint64_t	requestConfigEpoch	= ntohu64( request->configEpoch );
	/* 获取该请求从节点的槽位图信息 */
	unsigned char *claimed_slots = request->myslots;
	/* 是否指定强制认证故障转移的标识 */
	int	force_ack = request->mflags[0] & CLUSTERMSG_FLAG0_FORCEACK;
	int	j;

	/* 如果myself是从节点，或者myself没有负责的槽信息，那么myself节点没有投票权，直接返回 */
	if ( nodeIsSlave( myself ) || myself->numslots == 0 )
		return;

	/*
	 * 如果请求的当前纪元小于集群的当前纪元，直接返回。该节点有可能是长时间下线后重新上线，导致版本落后于就集群的版本
	 * 因为该请求节点的版本小于集群的版本，每次有选举或投票都会更新每个节点的版本，使节点状态和集群的状态是一致的。
	 */
	if ( requestCurrentEpoch < server.cluster->currentEpoch )
	{
		serverLog( LL_WARNING,
			   "Failover auth denied to %.40s: reqEpoch (%llu) < curEpoch(%llu)",
			   node->name,
			   (unsigned long long) requestCurrentEpoch,
			   (unsigned long long) server.cluster->currentEpoch );
		return;
	}

	/* 如果最近一次投票的纪元和当前纪元相同，表示集群已经投过票了 */
	if ( server.cluster->lastVoteEpoch == server.cluster->currentEpoch )
	{
		serverLog( LL_WARNING,
			   "Failover auth denied to %.40s: already voted for epoch %llu",
			   node->name,
			   (unsigned long long) server.cluster->currentEpoch );
		return;
	}
	/* 指定的node节点必须为从节点且它的主节点处于下线状态，否则打印日志后返回 */
	if ( nodeIsMaster( node ) || master == NULL ||
	     (!nodeFailed( master ) && !force_ack) )
	{
		/* 故障转移的请求必须由从节点发起 */
		if ( nodeIsMaster( node ) )
		{
			serverLog( LL_WARNING,
				   "Failover auth denied to %.40s: it is a master node",
				   node->name );
			/* 从节点找不到他的主节点 */
		} else if ( master == NULL )
		{
			serverLog( LL_WARNING,
				   "Failover auth denied to %.40s: I don't know its master",
				   node->name );
			/* 从节点的主节点没有处于下线状态 */
		} else if ( !nodeFailed( master ) )
		{
			serverLog( LL_WARNING,
				   "Failover auth denied to %.40s: its master is up",
				   node->name );
		}
		return;
	}
	/* 在cluster_node_timeout * 2时间内只能投1次票 */
	if ( mstime() - node->slaveof->voted_time < server.cluster_node_timeout * 2 )
	{
		serverLog( LL_WARNING,
			   "Failover auth denied to %.40s: "
			   "can't vote about this master before %lld milliseconds",
			   node->name,
			   (long long) ( (server.cluster_node_timeout * 2) -
					 (mstime() - node->slaveof->voted_time) ) );
		return;
	}

	/* 请求投票的从节点必须有一个声明负责槽位的配置纪元，这些配置纪元必须比负责相同槽位的主节点的配置纪元要大 */
	for ( j = 0; j < CLUSTER_SLOTS; j++ )
	{
		/* 跳过没有指定的槽位 */
		if ( bitmapTestBit( claimed_slots, j ) == 0 )
			continue;
		/* 如果请求从节点的配置纪元大于槽的配置纪元，则跳过 */
		if ( server.cluster->slots[j] == NULL ||
		     server.cluster->slots[j]->configEpoch <= requestConfigEpoch )
		{
			continue;
		}
		/* 如果请求从节点的配置纪元小于槽的配置纪元，那么表示该从节点的配置纪元已经过期，不能给该从节点投票，直接返回 */
		serverLog( LL_WARNING,
			   "Failover auth denied to %.40s: "
			   "slot %d epoch (%llu) > reqEpoch (%llu)",
			   node->name, j,
			   (unsigned long long) server.cluster->slots[j]->configEpoch,
			   (unsigned long long) requestConfigEpoch );
		return;
	}

	/* 发送一个FAILOVER_AUTH_ACK消息给指定的节点，表示支持该从节点进行故障转移 */
	clusterSendFailoverAuth( node );
	/* 设置最近一次投票的纪元，防止给多个节点投多次票 */
	server.cluster->lastVoteEpoch = server.cluster->currentEpoch;
	/* 设置最近一次投票的时间 */
	node->slaveof->voted_time = mstime();
	serverLog( LL_WARNING, "Failover auth granted to %.40s for epoch %llu",
		   node->name, (unsigned long long) server.cluster->currentEpoch );
}
```
当一个节点收到`REQUEST`消息包，首先判断当前节点是否是主节点，如果是从节点或者是主节点但是没有负责槽位，那么直接返回，没有投票的权利。

在发送`REQUEST`消息包之前，发送消息的节点会将自己的当前纪元加一，表示要发起一次选举操作，正常情况下，发送消息的节点的当前纪元会大于集群中的当前纪元，如果出现发起请求的从节点中途重新上线的话，就可能错过一次更新纪元的操作，会导致从节点当前版本落后于集群的最新版本，因此直接返回。

如果集群最近一次投票的纪元`lastVoteEpoch`和当前集群的纪元相同，表示当前接收消息的节点已经投过票了，直接返回。

如果发起投票请求的节点是主节点，或者如果请求节点是没有主节点的从节点，或者是请求节点是主节点没有下线的从节点会打印对应日志后返回。

如果发起投票没有成功选举出胜选的节点，过一定的时间会重新发起选举，因此，在`cluster_node_timeout * 2`不能投`2`次票。

遍历所有的槽位，跳过没有指派的槽位，如果当前槽位已经被指派，并且负责该槽位的配置纪元大于当前发送消息的节点的配置纪元，表示发送消息的从节点错过了槽位迁移的操作，导致槽位信息和主节点槽位信息不匹配。因此会直接返回。

如果满足以上条件，调用`clusterSendFailoverAuth()`发送`FAILOVER_AUTH_ACK`消息，表示支持该从节点进行故障转移操作。

发送`ACK`消息的函数`clusterSendFailoverAuth()`和发送`REQUEST`消息的函数相似，构建一个`ACK`包头，然后发送消息。

最后会更新集群最近一次投票的纪元`lastVoteEpoch`表示，已经透过一票，并且更新投票的时间。

### 3.5 替换主节点

[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)

当请求投票的从节点收到其他主节点的`ACK`消息后，会执行`clusterProcessPacket()`函数的这部分代码：
```
if ( type == CLUSTERMSG_TYPE_FAILOVER_AUTH_ACK )
{
	if ( !sender )
		return(1);      /* We don't know that node. */
	/* 如果sender是主节点，且sender有负责的槽对象，sender的配置纪元大于等于当前节点的配置纪元 */
	if ( nodeIsMaster( sender ) && sender->numslots > 0 &&
	     senderCurrentEpoch >= server.cluster->failover_auth_epoch )
	{
		/* 增加节点获得票数 */
		server.cluster->failover_auth_count++;
		clusterDoBeforeSleep( CLUSTER_TODO_HANDLE_FAILOVER );
	}
}
```
如果是主节点发送过来的`ACK`消息，并且是负责有槽位的主节点，并且该发送消息主节点的纪元大于等于当前纪元的故障转移纪元。才会算为有效的投票，将`failover_auth_count`加`1`。

从节点刚在执行到`clusterHandleSlaveFailover()`函数发送`REQUEST`消息后，就直接返回，是因为等待集群其他主节点回复是否同意该从节点进行故障转移操作。

因此，当再次执行`clusterHandleSlaveFailover()`函数时，如果集群其他节点投票数`failover_auth_count`大于所需的票数，则从节点可以对主节点进行故障转移。具体代码如下：
```
if ( server.cluster->failover_auth_count >= needed_quorum )
{
	serverLog( LL_WARNING, "Failover election won: I'm the new master." );

	if ( myself->configEpoch < server.cluster->failover_auth_epoch )
	{
		myself->configEpoch = server.cluster->failover_auth_epoch;
		serverLog( LL_WARNING,
			   "configEpoch set to %llu after successful failover",
			   (unsigned long long) myself->configEpoch );
	}
	/* 执行自动或手动故障转移，从节点获取其主节点的哈希槽，并传播新配置 */
	clusterFailoverReplaceYourMaster();
} else {
	clusterLogCantFailover( CLUSTER_CANT_FAILOVER_WAITING_VOTES );
}
```
票数足够，调用`clusterFailoverReplaceYourMaster()`函数进行故障转移，替换主节点。函数代码如下：
```
void clusterFailoverReplaceYourMaster( void )
{
	int j;
	/* 获取myself的主节点 */
	clusterNode *oldmaster = myself->slaveof;

	/* 如果myself节点是主节点，直接返回 */
	if ( nodeIsMaster( myself ) || oldmaster == NULL )
		return;

	/* 1) Turn this node into a master. */
	/* 将指定的myself节点重新配置为主节点 */
	clusterSetNodeAsMaster( myself );
	/* 取消复制操作，设置myself为主节点 */
	replicationUnsetMaster();

	/* 2) Claim all the slots assigned to our master. */
	/* 将所有之前主节点声明负责的槽位指定给现在的主节点myself节点。 */
	for ( j = 0; j < CLUSTER_SLOTS; j++ )
	{
		/* 如果当前槽已经指定 */
		if ( clusterNodeGetSlotBit( oldmaster, j ) )
		{
			/* 将该槽设置为未分配的 */
			clusterDelSlot( j );
			/* 将该槽指定给myself节点 */
			clusterAddSlot( myself, j );
		}
	}

	/* 3) Update state and save config. */
	/* 更新节点状态 */
	clusterUpdateState();
	/* 写配置文件 */
	clusterSaveConfigOrDie( 1 );


	/* 4) Pong all the other nodes so that they can update the state
	 *    accordingly and detect that we switched to master role. */
	/*
	 * 发送一个PONG消息包给所有已连接不处于握手状态的的节点
	 * 以便能够其他节点更新状态
	 */
	clusterBroadcastPong( CLUSTER_BROADCAST_ALL );

	/* 5) If there was a manual failover in progress, clear the state. */
	/* 重置与手动故障转移的状态 */
	resetManualFailover();
}
```
如果执行故障转移的节点是主节点或这是没有主节点的从节点，则直接返回。

1.  然后调用`clusterSetNodeAsMaster()`函数，将当前从节点设置为主节点，该函数会断开主从节点的双方的关系。
2.  调用`replicationUnsetMaster`取消从前从节点的复制操作，并将从节点设置为主节点。
3.  遍历所有的槽位，将之前由主节点负责的槽位指派给该新晋升的主节点。
4.  调用`clusterUpdateState()`函数更新集群的状态。
5.  调用`clusterSaveConfigOrDie()`函数，将当前的配置重写并同步到配置文件中
6.  调用`clusterBroadcastPong()`函数，给集群所有的节点发送`PONG`消息，将当前主从切换的信息告知给所有节点。
7.  调用`resetManualFailover()`函数，重置所有故障转移的状态，以便下次故障转移的开始。

### 3.6 主从切换广播给集群

当从节点替换主节点之后，调用`clusterBroadcastPong()`函数，将主从切换的信息发送给集群中的所有节点。该函数就是遍历集群中的所有节点，发送`PONG`消息，消息包中会包含最新的晋升为主节点的节点信息。[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)

接收到消息的函数会调用`clusterProcessPacket()`函数来处理，具体处理代码如下：
```
if ( sender )
{
	/* 如果消息头的slaveof为空名字，那么说明sender节点是主节点 */
	if ( !memcmp( hdr->slaveof, CLUSTER_NODE_NULL_NAME,
		      sizeof(hdr->slaveof) ) )
	{
		/* 将指定的sender节点重新配置为主节点 */
		clusterSetNodeAsMaster( sender );
		/* sender是从节点 */
	} else {
		/* 根据名字从集群中查找并返回sender从节点的主节点 */
		clusterNode *master = clusterLookupNode( hdr->slaveof );
		/* sender标识自己为主节点，但是消息中显示它为从节点 */
		if ( nodeIsMaster( sender ) )
		{
			/* 删除主节点所负责的槽 */
			clusterDelNodeSlots( sender );
			/* 消息主节点标识和导出标识 */
			sender->flags &= ~(CLUSTER_NODE_MASTER |
					   CLUSTER_NODE_MIGRATE_TO);
			/* 设置为从节点标识 */
			sender->flags |= CLUSTER_NODE_SL
			                 /* 更新配置和状态 */
					 clusterDoBeforeSleep( CLUSTER_TODO_SAVE_CONFIG |
							       CLUSTER_TODO_UPDATE_STATE );
		}

		/* sender的主节点发生改变 */
		if ( master && sender->slaveof != master )
		{
			/* 如果sender有新的主节点，将sender从旧的主节点保存其从节点字典中删除 */
			if ( sender->slaveof )
				clusterNodeRemoveSlave( sender->slaveof, sender );
			/* 将sender添加到新的主节点的从节点字典中 */
			clusterNodeAddSlave( master, sender );
			/* 设置sender的主节点 */
			sender->slaveof = master;

			/* Update config. */
			/* 更新配置 */
			clusterDoBeforeSleep( CLUSTER_TODO_SAVE_CONFIG );
		}
	}
}
```
接收到`PONG`消息的节点首先根据消息包头部的信息找到当前集群视角中的`sender`节点。

如果当前消息包中的`slaveof`为空，表示`sender`节点是主节点，那么调用`clusterSetNodeAsMaster()`函数将接收消息的节点视角中的集群关于`sender`节点配置为主节点。这应该是晋升的主节点发送而来的消息。

如果`sender`是主节点，但是消息包中显示的确实从节点，因此要更新接收消息的节点视角中的集群关于`sender`节点的角色。

如果消息包中显示`sender`的主节点`master`存在，但是`sender`节点的主节点不是`master`那么就要更新当前接收节点视角中集群关于`sender`节点的主节点信息。将消息包中显示的`master`节点设置为`sender`节点的主节点。

每一个接收到`PONG`消息的节点都会将进行主从切换，更新各自视角中的节点信息。故障转移操作完毕！

[Redis Cluster文件详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/cluster.c)
