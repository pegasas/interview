
# Redis源码剖析和注释（二十四）--- Redis Sentinel实现(哨兵操作的深入剖析)
# Redis Sentinel实现（下）

本文是[Redis Sentinel实现（上）](http://blog.csdn.net/men_wen/article/details/72805850)篇文章的下半部分剖析。主要剖析以下内容：

## 4. 哨兵的使命

-   [Redis Sentinel实现下](https://blog.csdn.net/men_wen/article/details/72805897#redis-sentinel%E5%AE%9E%E7%8E%B0%E4%B8%8B)
    -   [哨兵的使命](https://blog.csdn.net/men_wen/article/details/72805897#4-%E5%93%A8%E5%85%B5%E7%9A%84%E4%BD%BF%E5%91%BD)
        -   [1 周期性的操作](https://blog.csdn.net/men_wen/article/details/72805897#41-%E5%91%A8%E6%9C%9F%E6%80%A7%E7%9A%84%E6%93%8D%E4%BD%9C)
            -   [11 建立连接](https://blog.csdn.net/men_wen/article/details/72805897#411-%E5%BB%BA%E7%AB%8B%E8%BF%9E%E6%8E%A5)
            -   [12 发送监控命令](https://blog.csdn.net/men_wen/article/details/72805897#412-%E5%8F%91%E9%80%81%E7%9B%91%E6%8E%A7%E5%91%BD%E4%BB%A4)
            -   [13 判断节点的主观下线状态](https://blog.csdn.net/men_wen/article/details/72805897#413-%E5%88%A4%E6%96%AD%E8%8A%82%E7%82%B9%E7%9A%84%E4%B8%BB%E8%A7%82%E4%B8%8B%E7%BA%BF%E7%8A%B6%E6%80%81)
            -   [14 判断主节点的客观下线状态](https://blog.csdn.net/men_wen/article/details/72805897#414-%E5%88%A4%E6%96%AD%E4%B8%BB%E8%8A%82%E7%82%B9%E7%9A%84%E5%AE%A2%E8%A7%82%E4%B8%8B%E7%BA%BF%E7%8A%B6%E6%80%81)
            -   [15 对主节点执行故障转移](https://blog.csdn.net/men_wen/article/details/72805897#415-%E5%AF%B9%E4%B8%BB%E8%8A%82%E7%82%B9%E6%89%A7%E8%A1%8C%E6%95%85%E9%9A%9C%E8%BD%AC%E7%A7%BB)
                -   [151 故障转移开始](https://blog.csdn.net/men_wen/article/details/72805897#4151-%E6%95%85%E9%9A%9C%E8%BD%AC%E7%A7%BB%E5%BC%80%E5%A7%8B)
                -   [152 选择一个要晋升的从节点](https://blog.csdn.net/men_wen/article/details/72805897#4152-%E9%80%89%E6%8B%A9%E4%B8%80%E4%B8%AA%E8%A6%81%E6%99%8B%E5%8D%87%E7%9A%84%E4%BB%8E%E8%8A%82%E7%82%B9)
                -   [153 使从节点变为主节点](https://blog.csdn.net/men_wen/article/details/72805897#4153-%E4%BD%BF%E4%BB%8E%E8%8A%82%E7%82%B9%E5%8F%98%E4%B8%BA%E4%B8%BB%E8%8A%82%E7%82%B9)
                -   [154 等待从节点晋升为主节点](https://blog.csdn.net/men_wen/article/details/72805897#4154-%E7%AD%89%E5%BE%85%E4%BB%8E%E8%8A%82%E7%82%B9%E6%99%8B%E5%8D%87%E4%B8%BA%E4%B8%BB%E8%8A%82%E7%82%B9)
                -   [155 从节点同步新的主节点](https://blog.csdn.net/men_wen/article/details/72805897#4155-%E4%BB%8E%E8%8A%82%E7%82%B9%E5%90%8C%E6%AD%A5%E6%96%B0%E7%9A%84%E4%B8%BB%E8%8A%82%E7%82%B9)
            -   [16 更新主节点的状态](https://blog.csdn.net/men_wen/article/details/72805897#416-%E6%9B%B4%E6%96%B0%E4%B8%BB%E8%8A%82%E7%82%B9%E7%9A%84%E7%8A%B6%E6%80%81)
        -   [2 处理主从切换](https://blog.csdn.net/men_wen/article/details/72805897#42-%E5%A4%84%E7%90%86%E4%B8%BB%E4%BB%8E%E5%88%87%E6%8D%A2)

`sentinel.c`文件详细注释：[Redis Sentinel详细注释](https://github.com/menwengit/redis_source_annotation)

我们这一部分将会详细介绍`标题3.2`小节的内容，以下分析，是站在哨兵节点的视角。

我们当时已经简单分析到

-   `sentinelHandleRedisInstance()`函数用来处理主节点、从节点和哨兵节点的周期性操作。
-   `sentinelFailoverSwitchToPromotedSlave()`函数用来处理发生主从切换的情况。

因此，先来看第一部分。

### 4.1 周期性的操作

我们先来看看`sentinelHandleRedisInstance()`函数中到底实现了哪些操作：
```
void sentinelHandleRedisInstance( sentinelRedisInstance *ri )
{
	/* ========== MONITORING HALF ============ */
	/* ========== 一半监控操作 ============ */

	/* Every kind of instance */
	/* 对所有的类型的实例进行操作 */

	/* 为Sentinel和ri实例创建一个网络连接，包括cc和pc */
	sentinelReconnectInstance( ri );
	/* 定期发送PING、PONG、PUBLISH命令到ri实例中 */
	sentinelSendPeriodicCommands( ri );

	/* ============== ACTING HALF ============= */
	/* ============== 一半故障检测 ============= */

	/* 如果Sentinel处于TILT模式，则不进行故障检测 */
	if ( sentinel.tilt )
	{
		/* 如果TILT模式的时间没到，则不执行后面的动作，直接返回 */
		if ( mstime() - sentinel.tilt_start_time < SENTINEL_TILT_PERIOD )
			return;
		/* 如果TILT模式时间已经到了，取消TILT模式的标识 */
		sentinel.tilt = 0;
		sentinelEvent( LL_WARNING, "-tilt", NULL, "#tilt mode exited" );
	}

	/* Every kind of instance */
	/* 对于各种实例进行是否下线的检测，是否处于主观下线状态 */
	sentinelCheckSubjectivelyDown( ri );

	/* Masters and slaves */
	/* 目前对主节点和从节点的实例什么都不做 */
	if ( ri->flags & (SRI_MASTER | SRI_SLAVE) )
	{
		/* Nothing so far. */
	}

	/* Only masters */
	/* 只对主节点进行操作 */
	if ( ri->flags & SRI_MASTER )
	{
		/* 检查从节点是否客观下线 */
		sentinelCheckObjectivelyDown( ri );
		/* 如果处于客观下线状态，则进行故障转移的状态设置 */
		if ( sentinelStartFailoverIfNeeded( ri ) )
			/*
			 * 强制向其他Sentinel节点发送SENTINEL IS-MASTER-DOWN-BY-ADDR给所有的Sentinel获取回复
			 * 尝试获得足够的票数，标记主节点为客观下线状态，触发故障转移
			 */
			sentinelAskMasterStateToOtherSentinels( ri, SENTINEL_ASK_FORCED );
		/* 执行故障转移操作 */
		sentinelFailoverStateMachine( ri );
		/*
		 * 主节点ri没有处于客观下线的状态，那么也要尝试发送SENTINEL IS-MASTER-DOWN-BY-ADDR给所有的Sentinel获取回复
		 * 因为ri主节点如果有回复延迟等等状况，可以通过该命令，更新一些主节点状态
		 */
		sentinelAskMasterStateToOtherSentinels( ri, SENTINEL_NO_FLAGS );
	}
}
```
很明显，该函数将周期性的操作分为两个部分，一部分是对一个的实例进行监控的操作，另一部分是对该实例执行故障检测。

**接下来就针对上面代码的每一步进行详细剖析，每一个标题对应一步。**

#### 4.1.1 建立连接

首先，执行的第一个函数就是`sentinelReconnectInstance()`函数，因为在载入配置的时候，我们将创建的主节点实例加入到`sentinel.masters`字典的时候，该主节点的连接是关闭的，所以第一件事就是为主节点和哨兵节点建立网络连接。

查看`sentinelReconnectInstance()`函数代码：
```
void sentinelReconnectInstance( sentinelRedisInstance *ri )
{
	/* 如果ri实例没有连接中断，则直接返回 */
	if ( ri->link->disconnected == 0 )
		return;
	/* ri实例地址非法 */
	if ( ri->addr->port == 0 )
		return;              /* port == 0 means invalid address. */
	instanceLink	*link	= ri->link;
	mstime_t	now	= mstime();

	/* 如果还没有最近一次重连的时间距离现在太短，小于1s，则直接返回 */
	if ( now - ri->link->last_reconn_time < SENTINEL_PING_PERIOD )
		return;
	/* 设置最近重连的时间 */
	ri->link->last_reconn_time = now;

	/* Commands connection. */
	/* cc：命令连接 */
	if ( link->cc == NULL )
	{
		/* 绑定ri实例的连接地址并建立连接 */
		link->cc = redisAsyncConnectBind( ri->addr->ip, ri->addr->port, NET_FIRST_BIND_ADDR );
		/* 命令连接失败，则事件通知，且断开cc连接 */
		if ( link->cc->err )
		{
			sentinelEvent( LL_DEBUG, "-cmd-link-reconnection", ri, "%@ #%s",
				       link->cc->errstr );
			instanceLinkCloseConnection( link, link->cc );
			/* 命令连接成功 */
		} else {
			/* 重置cc连接的属性 */
			link->pending_commands	= 0;
			link->cc_conn_time	= mstime();
			link->cc->data		= link;
			/* 将服务器的事件循环关联到cc连接的上下文中 */
			redisAeAttach( server.el, link->cc );
			/* 设置确立连接的回调函数 */
			redisAsyncSetConnectCallback( link->cc,
						      sentinelLinkEstablishedCallback );
			/* 设置断开连接的回调处理 */
			redisAsyncSetDisconnectCallback( link->cc,
							 sentinelDisconnectCallback );
			/* 发送AUTH 命令认证 */
			sentinelSendAuthIfNeeded( ri, link->cc );
			/* 发送连接名字 */
			sentinelSetClientName( ri, link->cc, "cmd" );

			/* Send a PING ASAP when reconnecting. */
			/* 立即向ri实例发送PING命令 */
			sentinelSendPing( ri );
		}
	}
	/* Pub / Sub */
	/*
	 * pc：发布订阅连接
	 * 只对主节点和从节点如果没有设置pc连接则建立一个
	 */
	if ( (ri->flags & (SRI_MASTER | SRI_SLAVE) ) && link->pc == NULL )
	{
		/* 绑定指定ri的连接地址并建立连接 */
		link->pc = redisAsyncConnectBind( ri->addr->ip, ri->addr->port, NET_FIRST_BIND_ADDR );
		/* pc连接失败，则事件通知，且断开pc连接 */
		if ( link->pc->err )
		{
			sentinelEvent( LL_DEBUG, "-pubsub-link-reconnection", ri, "%@ #%s",
				       link->pc->errstr );
			instanceLinkCloseConnection( link, link->pc );
			/* pc连接成功 */
		} else {
			int retval;

			link->pc_conn_time	= mstime();
			link->pc->data		= link;
			/* 将服务器的事件循环关联到pc连接的上下文中 */
			redisAeAttach( server.el, link->pc );
			/* 设置确立连接的回调函数 */
			redisAsyncSetConnectCallback( link->pc,
						      sentinelLinkEstablishedCallback );
			/* 设置断开连接的回调处理 */
			redisAsyncSetDisconnectCallback( link->pc,
							 sentinelDisconnectCallback );
			/*  发送AUTH 命令认证 */
			sentinelSendAuthIfNeeded( ri, link->pc );
			/* 发送连接名字 */
			sentinelSetClientName( ri, link->pc, "pubsub" );
			/*
			 * 发送订阅 __sentinel__:hello 频道的命令，设置回调函数处理回复
			 * sentinelReceiveHelloMessages是处理Pub/Sub的频道返回信息的回调函数，可以发现订阅同一master的Sentinel节点
			 */
			retval = redisAsyncCommand( link->pc,
						    sentinelReceiveHelloMessages, ri, "SUBSCRIBE %s",
						    SENTINEL_HELLO_CHANNEL );
			/* 订阅频道出错，关闭 */
			if ( retval != C_OK )
			{
				/* 关闭pc连接 */
				instanceLinkCloseConnection( link, link->pc );
				return;
			}
		}
	}
	/* 如果已经建立了新的连接，则清除断开连接的状态。表示已经建立了连接 */
	if ( link->cc && (ri->flags & SRI_SENTINEL || link->pc) )
		link->disconnected = 0;
}
```
建立连接的函数`redisAsyncConnectBind()`是`Redis`的官方`C语言客户端hiredis`的异步连接函数，当连接成功时需要调用`redisAeAttach()`函数来将服务器的事件循环(ae)与连接的上下文相关联起来（因为hiredis提供了多种适配器，包括事件ae，libev，libevent，libuv），在关联的时候，会设置了网络连接的可写可读事件的处理程序。接下来还会设置该连接的确立时和断开时的回调函数`redisAsyncSetConnectCallback()`和`redisAsyncSetDisconnectCallback()`，为什么这么做，就是因为该连接是异步的。

了解了以上这些，继续分析节点实例和当前哨兵的连接建立。从该函数中可以很明显的看出来：

-   如论是主节点、从节点还是哨兵节点，都会与当前哨兵建立**命令连接（Commands connection）**。
-   只有主节点或从节点才会建立**发布订阅连接（Pub / Sub connection）**。

**当建立了命令连接（cc）之后立即执行了三个动作**：
|执行函数|执行函数的目的|回复函数|回复函数操作|
|--|--|--|--|
|sentinelSendAuthIfNeeded()|发送 `AUTH` 命令进行认证|sentinelDiscardReplyCallback()|丢弃回复，只是将已发送但未回复的命令个数减1|
|--|--|--|--|
|sentinelSetClientName()|发送 `CLIENT SETNAME`命令设置连接的名字|sentinelDiscardReplyCallback()|丢弃回复，只是将已发送但未回复的命令个数减1|
|--|--|--|--|
|sentinelSendPing()|发送 `PING` 命令来判断连接状态|sentinelPingReplyCallback()|根据回复的内容来更新一些连接交互时间等|
**当建立了发布订阅连接（pc）之后立即执行的动作**：（前两个动作与命令连接相同，只列出不相同的第三个）
|执行函数|执行函数的目的|回复函数|回复函数操作|
|--|--|--|--|
redisAsyncCommand()|发送 `SUBSCRIBE` 命令来判订阅`__sentinel__:hello` 频道的事件提醒|sentinelReceiveHelloMessages()|哨兵节点处理从实例（主节点或从节点）发送过来的`hello`信息，来获取其他哨兵节点或主节点的从节点信息|
如果成功建立连接，之后会清除连接断开的标志，以表示连接已建立。

如果不是第一次执行，那么会判断连接是否建立，如果断开，则重新给建立，如果没有断开，那么什么都不会做直接返回。

#### 4.1.2 发送监控命令

执行完建立网络连接的函数，接下来会执行`sentinelSendPeriodicCommands()`函数，该函数就是定期发送一些监控命令到主节点或从节点或哨兵节点，这些节点会将哨兵节点作为客户端来处理，我们接下来仔细分析。
```
void sentinelSendPeriodicCommands( sentinelRedisInstance *ri )
{
	mstime_t	now = mstime();
	mstime_t	info_period, ping_period;
	int		retval;

	/* 如果ri实例连接处于关闭状态，直接返回 */
	if ( ri->link->disconnected )
		return;

	/*
	 * 对于不是发送关键命令的INFO，PING，PUBLISH，我们也有SENTINEL_MAX_PENDING_COMMANDS的限制。 我们不想使用大量的内存，只是因为连接对象无法正常工作（请注意，无论如何，还有一个冗余的保护措施，即如果检测到长时间的超时条件，连接将被断开连接并重新连接
	 * 每个实例的已发送未回复的命令个数不能超过100个，否则直接返回
	 */
	if ( ri->link->pending_commands >=
	     SENTINEL_MAX_PENDING_COMMANDS * ri->link->refcount )
		return;

	/*
	 * 如果主节点处于O_DOWN状态下，那么Sentinel默认每秒发送INFO命令给它的从节点，而不是通常的SENTINEL_INFO_PERIOD(10s)周期。在这种状态下，我们想更密切的监控从节点，万一他们被其他的Sentinel晋升为主节点
	 * 如果从节点报告和主节点断开连接，我们同样也监控INFO命令的输出更加频繁，以便我们能有一个更新鲜的断开连接的时间
	 */

	/* 如果ri是从节点，且他的主节点处于故障状态的状态或者从节点和主节点断开复制了 */
	if ( (ri->flags & SRI_SLAVE) &&
	     ( (ri->master->flags & (SRI_O_DOWN | SRI_FAILOVER_IN_PROGRESS) ) ||
	       (ri->master_link_down_time != 0) ) )
	{
		/* 设置INFO命令的周期时间为1s */
		info_period = 1000;
	} else {
		/* 否则就是默认的10s */
		info_period = SENTINEL_INFO_PERIOD;
	}

	/* 每次最后一次接收到的PONG比配置的 'down-after-milliseconds' 时间更长，但是如果 'down-after-milliseconds'大于1秒，则每秒钟进行一次ping */

	/* 获取ri设置的主观下线的时间 */
	ping_period = ri->down_after_period;
	/* 如果大于1秒，则设置为1秒 */
	if ( ping_period > SENTINEL_PING_PERIOD )
		ping_period = SENTINEL_PING_PERIOD;

	/* 如果实例不是Sentinel节点且Sentinel节点从该数据节点(主节点或从节点)没有收到过INFO回复或者收到INFO回复超时 */
	if ( (ri->flags & SRI_SENTINEL) == 0 &&
	     (ri->info_refresh == 0 ||
	      (now - ri->info_refresh) > info_period) )
	{
		/* 发送INFO命令给主节点和从节点 */
		retval = redisAsyncCommand( ri->link->cc,
					    sentinelInfoReplyCallback, ri, "INFO" );
		/* 已发送未回复的命令个数加1 */
		if ( retval == C_OK )
			ri->link->pending_commands++;

		/* 如果发送和回复PING命令超时 */
	} else if ( (now - ri->link->last_pong_time) > ping_period &&
		    (now - ri->link->last_ping_time) > ping_period / 2 )
	{
		/* 发送一个PING命令给ri实例，并且更新act_ping_time */
		sentinelSendPing( ri );

		/* 发送频道的定时命令超时 */
	} else if ( (now - ri->last_pub_time) > SENTINEL_PUBLISH_PERIOD )
	{
		/* 发布hello信息给ri实例 */
		sentinelSendHello( ri );
	}
}
```
从这个函数我们可以了解到一下信息：

-   一个连接对发送命令的个数有限制。因为连接是一个异步操作，发送了不一定会立即接收到，因此会为了节约内存而有一个限制，已发送未回复的命令个数不能超过`100`个，否则不做操作。
-   当该哨兵节点正在监控从节点时，但是从节点从属的主节点发送了故障，那么会设置发送`INFO`命令的频率为`1s`，否则就是默认的`10s`发送一次`INFO`命令。
-   `PING`命令的频率是`1s`发送一次。

接下来，就逐个分析所发送的监控命令。

-   **第一个是`INFO`命令**

哨兵节点只将`INFO`命令发送给主节点或从节点。并且设置`sentinelInfoReplyCallback()`函数来处理`INFO`命令的回复信息。

处理函数的代码有300多行，这里就不列出来了，可以上`github`查看  [sentinel.c源码详细注释](https://github.com/menwengit/redis_source_annotation/blob/master/sentinel.c)。

当`INFO`名的回复正确时，会调用`sentinelRefreshInstanceInfo()`函数来处理`INFO`命令的回复。处理`INFO`命令的回复有两部分：

-   获取该连接的节点实例最基本的信息，如：`run_id`，`role`，如果是发送给主节点，会获取到从节点信息；如果是发送给从节点，会获取到其主节点的信息。总之会获取当前整个集群网络的所有活跃的节点信息，并将其保存到当前哨兵的状态中，而且会刷新配置文件。这就是为什么在配置文件中不需要配置从节点的信息，因为通过这一操作会自动发现从节点。
-   处理角色变化的情况。当接收到`INFO`命令的回复，有可能发现当前哨兵连接的节点的角色状态发生变化，因此要处理这些情况。  
    -   连接的节点实例是主节点，但是`INFO`命令显示连接的是从节点。
    -   什么也不做。
    -   连接的节点实例是从节点，但是`INFO`命令显示连接的是主节点。
    -   连接的从节点是被晋升的从节点，且他的主节点处于等待该从节点晋升的状态，那么会更新一些属性。
    -   连接的从节点是被晋升的从节点，但是主节点在发生故障转移的超时时间限制内又重新上线，因此要将该晋升的从节点重新降级为普通的从节点，并从属原来的主节点，通过发送`slaveof`命令。
    -   连接的节点实例是从节点，`INFO`命令显示连接的也是主节点，但是发现该从节点从属的主节点地址发生了变化。
    -   发送`slaveof`命令使其从属新的主节点。
    -   连接的节点实例是从节点，`INFO`命令显示连接的也是主节点，但是该从节点处于已经接受`slaveof`命令(SRI_RECONF_SENT)或者正在根据`slaveof`命令指定的主节点执行同步操作(SRI_RECONF_INPROG)的状态。
    -   将他们的状态设置为下一步状态，表示当前状态的操作已经完成。

当这些处理只是当收到`INFO`命令的回复时才会进行处理。我们继续分析下一个发送监控的命令。

-   **第二个是`PING`命令**

这个发送的函数`sentinelSendPing()`函数和在第一次创建命令连接时执行的函数操作一样。
```
int sentinelSendPing( sentinelRedisInstance *ri )
{
	/* 异步发送一个PING命令给实例ri */
	int retval = redisAsyncCommand( ri->link->cc,
					sentinelPingReplyCallback, ri, "PING" );
	/* 发送成功 */
	if ( retval == C_OK )
	{
		/* 已发送未回复的命令个数加1 */
		ri->link->pending_commands++;
		/* 更新最近一次发送PING命令的时间 */
		ri->link->last_ping_time = mstime();
		/* 更新最近一次发送PING命令，但没有收到PONG命令的时间 */
		if ( ri->link->act_ping_time == 0 )
			ri->link->act_ping_time = ri->link->last_ping_time;
		return(1);
	} else {
		return(0);
	}
}
```
该函数，发送给实例一个`PING`并且更新所有连接的状态。设置`sentinelPingReplyCallback()`来处理`PING`命令的回复。

`PING`命令的回复有以下两种：

-   状态回复或者错误回复  
    -   `PONG`、`LOADING`、`MASTERDOWN`这三个是可以接受的回复，会更新最近的交互时间，用来判断实例和哨兵之间的网络可达。
-   忙回复  
    -   `BUSY`这个可能会是因为执行脚本而表现为下线状态。所以会发送一个`SCRIPT KILL`命令来终止脚本的执行。

无论如何，只要接受到回复，都会更新最近一次收到`PING`命令回复的状态，表示连接可达。

-   **第三个是`PUBLISH`命令**

发送`PUBLISH`命令，可以叫发送hello信息。因为这个操作像是和订阅该主节点的其他哨兵节点打招呼。

函数`sentinelSendHello()`用来发送hello信息，该函数主要做了两步操作：

-   构建hello信息的内容。hello信息的格式如下：  
    -   `sentinel_ip,sentinel_port,sentinel_runid,current_epoch,master_name,master_ip,master_port,master_config_epoch`这些信息包含有：当前哨兵的信息和主节点信息。
-   发送`PUBLISH`命令，将hello信息发布到创建连接时建立的频道。  
    -   设置`sentinelPublishReplyCallback()`函数为处理`PUBLISH`命令的回复。该命令主要就是更新通过频道进行通信的时间，以便保持发布订阅连接的可达。

通过发送`PUBLISH`命令给任意类型实例，最终都是将**主节点信息和当前哨兵信息**广播给所有的订阅指定频道的哨兵节点，这样就可以将监控相同主节点的哨兵保存在哨兵实例的`sentinels`字典中。

发送完这些命令，就会获取所有节点的新的状态。因此，要根据这些状态要判断是否出现网络故障。

#### 4.1.3 判断节点的主观下线状态

当前哨兵节点发送完所有的监控命令，有可能发送成功且顺利收到回复，也有可能发送和回复都没有成功收到等等可能，因此要对当前节点实例（所有类型都要进行判断）调用`sentinelCheckSubjectivelyDown()`函数进行主观下线判断。
```
void sentinelCheckSubjectivelyDown( sentinelRedisInstance *ri )
{
	mstime_t elapsed = 0;
	/* 获取ri实例回复命令已经过去的时长 */
	if ( ri->link->act_ping_time )
		/* 获取最近一次发送PING命令过去了多少时间 */
		elapsed = mstime() - ri->link->act_ping_time;
	/* 如果实例的连接已经断开 */
	else if ( ri->link->disconnected )
		/* 获取最近一次回复PING命令过去了多少时间 */
		elapsed = mstime() - ri->link->last_avail_time;

	/*
	 * 如果连接处于低活跃度，那么进行重新连接
	 * cc命令连接超过了1.5s，并且之前发送过PING命令但是连接活跃度很低
	 */
	if ( ri->link->cc &&
	     (mstime() - ri->link->cc_conn_time) >
	     SENTINEL_MIN_LINK_RECONNECT_PERIOD &&
	     ri->link->act_ping_time != 0 &&    /* Ther is a pending ping... */


	                                        /* The pending ping is delayed, and we did not received
	                                         * error replies as well. */
	     (mstime() - ri->link->act_ping_time) > (ri->down_after_period / 2) &&
	     (mstime() - ri->link->last_pong_time) > (ri->down_after_period / 2) )
	{                                       /* 断开ri实例的cc命令连接 */
		instanceLinkCloseConnection( ri->link, ri->link->cc );
	}

	/* 检查pc发布订阅的连接是否也处于低活跃状态 */
	if ( ri->link->pc &&
	     (mstime() - ri->link->pc_conn_time) >
	     SENTINEL_MIN_LINK_RECONNECT_PERIOD &&
	     (mstime() - ri->link->pc_last_activity) > (SENTINEL_PUBLISH_PERIOD * 3) )
	{       /* 断开ri实例的pc发布订阅连接 */
		instanceLinkCloseConnection( ri->link, ri->link->pc );
	}
	/* 更新主观下线标志，条件如下： */


	/*
	 *  1. 没有回复命令
	 *  2. Sentinel节点认为ri是主节点，但是它报告它是从节点
	 */
	/* ri实例回复命令已经过去的时长已经超过主观下线的时限，并且ri实例是主节点，但是报告是从节点 */
	if ( elapsed > ri->down_after_period ||
	     (ri->flags & SRI_MASTER &&
	      ri->role_reported == SRI_SLAVE &&
	      mstime() - ri->role_reported_time >
	      (ri->down_after_period + SENTINEL_INFO_PERIOD * 2) ) )
	{
		/* Is subjectively down */
		/* 设置主观下线的标识 */
		if ( (ri->flags & SRI_S_DOWN) == 0 )
		{
			/* 发送"+sdown"的事件通知 */
			sentinelEvent( LL_WARNING, "+sdown", ri, "%@" );
			/* 设置实例被判断主观下线的时间 */
			ri->s_down_since_time	= mstime();
			ri->flags		|= SRI_S_DOWN;
		}
	} else {
		/* Is subjectively up */
		/* 如果设置了主观下线的标识，则取消标识 */
		if ( ri->flags & SRI_S_DOWN )
		{
			sentinelEvent( LL_WARNING, "-sdown", ri, "%@" );
			ri->flags &= ~(SRI_S_DOWN | SRI_SCRIPT_KILL_SENT);
		}
	}
}
```
该函数主要做了两件事：

-   根据命令连接和发布订阅连接的活跃度来判断是否要执行断开对应连接的操作。以便下次时钟循环在重新连接，以保证可靠性。
-   获取回复`PING`命令过去的时间，然后进行判断是否已经下线。如果满足主观下线的条件，那么会设置主观下线的标识。主观下线条件有两个：  
    -   回复`PING`命令超时
    -   哨兵节点发现他的角色发生变化。认为它是主节点但是报告显示它是从节点。

当判断完主观下线厚，虽然对实例设置了主观下线的标识，但是只有该实例是主节点，才会执行进一步的判断。否则对于其他类型节点来说，他们的周期性操作已经执行完成。

#### 4.1.4 判断主节点的客观下线状态

客观下线状态的判断只针对主节点而言。之前已经判断过主观下线，因此只有被当前哨兵节点判断为主观下线的主节点才会继续执行客观下线的判断。
```
void sentinelCheckObjectivelyDown( sentinelRedisInstance *master )
{
	dictIterator	*di;
	dictEntry	*de;
	unsigned int	quorum = 0, odown = 0;
	/* 如果该master实例已经被当前Sentinel节点判断为主观下线 */
	if ( master->flags & SRI_S_DOWN )
	{
		/* Is down for enough sentinels? */
		/* 当前Sentinel节点认为下线投1票 */
		quorum = 1; /* the current sentinel. */
		/* Count all the other sentinels. */
		di = dictGetIterator( master->sentinels );
		/* 遍历监控该master实例的所有的Sentinel节点 */
		while ( (de = dictNext( di ) ) != NULL )
		{
			sentinelRedisInstance *ri = dictGetVal( de );
			/* 如果Sentinel也认为master实例主观下线，那么增加投票数 */
			if ( ri->flags & SRI_MASTER_DOWN )
				quorum++;
		}
		dictReleaseIterator( di );
		/* 如果超过master设置的客观下线票数，则设置客观下线标识 */
		if ( quorum >= master->quorum )
			odown = 1;
	}

	/* Set the flag accordingly to the outcome. */
	/* 如果被判断为客观下线 */
	if ( odown )
	{
		/* master没有客观下线标识则要设置 */
		if ( (master->flags & SRI_O_DOWN) == 0 )
		{
			/* 发送"+odown"事件通知 */
			sentinelEvent( LL_WARNING, "+odown", master, "%@ #quorum %d/%d",
				       quorum, master->quorum );
			/* 设置master客观下线标识 */
			master->flags |= SRI_O_DOWN;
			/* 设置master被判断客观下线的时间 */
			master->o_down_since_time = mstime();
		}
		/* master实例没有客观下线 */
	} else {
		/* 取消master客观下线标识 */
		if ( master->flags & SRI_O_DOWN )
		{
			/* 发送"-odown"事件通知 */
			sentinelEvent( LL_WARNING, "-odown", master, "%@" );
			master->flags &= ~SRI_O_DOWN;
		}
	}
}
```
该函数做了两个工作：

-   遍历监控该主节点的所有其他的哨兵节点，如果这些哨兵节点也认为当前主节点下线（SRI_MASTER_DOWN），那么投票数加1，当超过设置的投票数，标识客观下线的标志。
-   如果客观下线的标志（odown）为真，那么打开主节点的客观下线的表示，否则取消主节点客观下线的标识。

这种方法存在一个缺陷，那么就是**客观下线意味这有足够多的Sentinel节点报告该主节点在一个时间范围内不可达。但是信息可能被延迟，不能保证N个实例在同一时间都同意该实例进入下线状态。**

执行完的客观下线判断，如果发现主节点打开了客观下线的状态标识，那么就进一步进行判断，否则就执行跳过判断。执行这进一步判断的函数是：`sentinelStartFailoverIfNeeded()`。该函数用来判断能不能进行故障转移：

-   主节点必须处于客观下线状态。如果没有打开客观下线的标识，就会直接返回0。
-   没有正在对主节点进行故障转移。
-   一段时间内没有尝试进行故障转移，防止频繁执行故障转移。

如果以上条件都满足，那么会调用`sentinelStartFailover()`函数，将更新主节点的故障转移状态，会执行下面这句关键的代码
```
master->failover_state	= SENTINEL_FAILOVER_STATE_WAIT_START;
master->flags		|= SRI_FAILOVER_IN_PROGRESS;
```
并且返回`1`，执行`if`条件中的代码：
```
sentinelAskMasterStateToOtherSentinels(ri,SENTINEL_ASK_FORCED);
```
由于指定了一个`SENTINEL_ASK_FORCED`标识，因此会强制发送一个`SENTINEL is-master-down-by-addr`命令来真正判断是否主节点下线，不会被时间条件所拒绝执行。
```
void sentinelAskMasterStateToOtherSentinels( sentinelRedisInstance *master, int flags )
{
	dictIterator	*di;
	dictEntry	*de;

	di = dictGetIterator( master->sentinels );
	/* 遍历监控master的所有的Sentinel节点 */
	while ( (de = dictNext( di ) ) != NULL )
	{
		sentinelRedisInstance *ri = dictGetVal( de );
		/* 当前Sentinel实例最近一个回复SENTINEL IS-MASTER-DOWN-BY-ADDR命令所过去的时间 */
		mstime_t	elapsed = mstime() - ri->last_master_down_reply_time;
		char		port[32];
		int		retval;

		/* If the master state from other sentinel is too old, we clear it. */
		/* 如果master状态太旧没有更新，则清除它保存的主节点状态 */
		if ( elapsed > SENTINEL_ASK_PERIOD * 5 )
		{
			ri->flags &= ~SRI_MASTER_DOWN;
			sdsfree( ri->leader );
			ri->leader = NULL;
		}
		/* 满足以下条件向其他Sentinel节点询问主节点是否下线 */


		/*
		 *  1. 当前Sentinel节点认为它已经下线，并且处于故障转移状态
		 *  2. 其他Sentinel与当前Sentinel保持连接状态
		 *  3. 在SENTINEL_ASK_PERIOD毫秒内没有收到INFO回复
		 */
		/* 主节点没有处于客观下线状态，则跳过当前Sentinel节点 */
		if ( (master->flags & SRI_S_DOWN) == 0 )
			continue;
		/* 如果当前Sentinel节点断开连接，也跳过 */
		if ( ri->link->disconnected )
			continue;
		/* 最近回复SENTINEL IS-MASTER-DOWN-BY-ADDR命令在SENTINEL_ASK_PERIODms时间内已经回复过了，则跳过 */
		if ( !(flags & SENTINEL_ASK_FORCED) &&
		     mstime() - ri->last_master_down_reply_time < SENTINEL_ASK_PERIOD )
			continue;

		/* Ask */
		/* 发送SENTINEL IS-MASTER-DOWN-BY-ADDR命令 */
		ll2string( port, sizeof(port), master->addr->port );
		/* 异步发送命令 */
		retval = redisAsyncCommand( ri->link->cc,
					    sentinelReceiveIsMasterDownReply, ri,
					    "SENTINEL is-master-down-by-addr %s %s %llu %s",
					    master->addr->ip, port,
					    sentinel.current_epoch,
		                                /*
		                                 * 如果主节点处于故障转移的状态，那么发送该Sentinel的ID，让收到命令的Sentinel节点选举自己为领头
		                                 * 否则发送"*"表示发送投票
		                                 */
					    (master->failover_state > SENTINEL_FAILOVER_STATE_NONE) ?
					    sentinel.myid : "*" );
		/* 已发送未回复的命令个数加1 */
		if ( retval == C_OK )
			ri->link->pending_commands++;
	}
	dictReleaseIterator( di );
}
```
该函数遍历所有监控该主节点的哨兵节点，跳过三种不符合下线的条件的哨兵节点，然后就发送`SENTINEL is-master-down-by-addr`命令，之前在`if`判断时，就设置了主节点的故障转移状态为`SENTINEL_FAILOVER_STATE_WAIT_START`，因此发送的`SENTINEL`命令中会加上自己的`runid`，用来请求所有收到命令的哨兵节点将自己选举为执行故障转移的领头。

由于发送的是异步命令，所以会设置回调函数`sentinelReceiveIsMasterDownReply()`来处理命令回复。

如果收到的回复的第一个整型值为`1`则打开该哨兵节点的主节点下线标识（SRI_MASTER_DOWN）。这里就是前面说的那么缺陷，因为是回调函数，该主节点下线标识（SRI_MASTER_DOWN）不会立即打开，可能存在延迟。

至此，主节点的客观下线判断完毕，如果确认了客观下线，那么就会执行故障转移操作。

#### 4.1.5 对主节点执行故障转移

故障转移操作的过程非常清晰，正如函数`sentinelFailoverStateMachine()`所写的那样。`sentinel.c`文件详细注释：[Redis Sentinel详细注释](https://github.com/menwengit/redis_source_annotation)
```
void sentinelFailoverStateMachine( sentinelRedisInstance *ri )
{
	/* ri实例必须是主节点 */
	serverAssert( ri->flags & SRI_MASTER );
	/* 如果主节点不处于进行故障转移操作的状态，则直接返回 */
	if ( !(ri->flags & SRI_FAILOVER_IN_PROGRESS) )
		return;
	/* 根据故障转移的状态，执行合适的操作 */
	switch ( ri->failover_state )
	{
	/* 故障转移开始 */
	case SENTINEL_FAILOVER_STATE_WAIT_START:
		sentinelFailoverWaitStart( ri );
		break;
	/* 选择一个要晋升的从节点 */
	case SENTINEL_FAILOVER_STATE_SELECT_SLAVE:
		sentinelFailoverSelectSlave( ri );
		break;
	/* 发送slaveof no one命令，使从节点变为主节点 */
	case SENTINEL_FAILOVER_STATE_SEND_SLAVEOF_NOONE:
		sentinelFailoverSendSlaveOfNoOne( ri );
		break;
	/* 等待被选择的从节点晋升为主节点，如果超时则重新选择晋升的从节点 */
	case SENTINEL_FAILOVER_STATE_WAIT_PROMOTION:
		sentinelFailoverWaitPromotion( ri );
		break;
	/* 给所有的从节点发送slaveof命令，同步新的主节点 */
	case SENTINEL_FAILOVER_STATE_RECONF_SLAVES:
		sentinelFailoverReconfNextSlave( ri );
		break;
	}
}
```
在之前判断主节点客观下线的时候，会将故障转移的状态打开，就是下面这样：
```
master->failover_state	= SENTINEL_FAILOVER_STATE_WAIT_START;
master->flags		|= SRI_FAILOVER_IN_PROGRESS;
```
所以，主节点如果没有被判断为主观下线，就不会判断为客观下线，因此也就不会执行故障转移操作。

之前设置的这些状态正好可以执行故障转移操作。这个过程分为五部：

-   故障转移开始。  
    -   `SENTINEL_FAILOVER_STATE_WAIT_START`
-   选择一个要晋升的从节点。  
    -   `SENTINEL_FAILOVER_STATE_SELECT_SLAVE`
-   发送`slaveof no one`命令，使从节点变为主节点。  
    -   `SENTINEL_FAILOVER_STATE_SEND_SLAVEOF_NOONE`
-   等待被选择的从节点晋升为主节点，如果超时则重新选择晋升的从节点。  
    -   `SENTINEL_FAILOVER_STATE_WAIT_PROMOTION`
-   给所有的从节点发送`slaveof`命令，同步新的主节点。  
    -   `SENTINEL_FAILOVER_STATE_RECONF_SLAVES`

这五部是连续的，成功执行完一步操作，都会将状态设置为下一步状态。而且这五部是分开执行的，意思是，每一次时间事件处理只处理一步，倘若已经执行了几部故障转移操作，但是在接下来的故障检测时，发现主节点是可达的，因此在之前的下线判断中都会将下线标识取消，会中断执行故障转移。

> 以上现象在`标题4.1.2 发送监控命令`中有提到，处理接收到的`INFO`命令时的一个场景：连接的从节点是被晋升的从节点，但是主节点在发生故障转移的超时时间限制内又重新上线，因此要将该晋升的从节点重新降级为普通的从节点，并从属原来的主节点。

##### 4.1.5.1 故障转移开始

`sentinelFailoverWaitStart()`函数会处理故障转移的开始，主要是选举出一个领头哨兵节点，用来领导故障转移，并且更新故障转移的状态。

sentinel 自动故障迁移使用`Raft`算法来选举领头（leader） Sentinel ，从而确保在一个给定的纪元（epoch）里，只有一个领头产生。

> [动画演示Raft算法](http://thesecretlivesofdata.com/raft/)
> 
> [有关Raft算法的通俗易懂的解释](http://www.cnblogs.com/mindwind/p/5231986.html)

这表示在同一个纪元中，不会有两个 Sentinel 同时被选中为领头，并且各个 Sentinel 在同一个纪元中只会对一个领头进行投票。

更高的配置纪元总是优于较低的纪元，因此每个 Sentinel 都会主动使用更新的纪元来代替自己的配置。

简单来说，我们可以将 Sentinel 配置看作是一个带有版本号的状态。一个状态会以最后写入者胜出（last-write-wins）的方式（也即是，最新的配置总是胜出）传播至所有其他 Sentinel 。

##### 4.1.5.2 选择一个要晋升的从节点

`sentinelFailoverSelectSlave()`函数用来选择一个要晋升的从节点。该函数调用`sentinelSelectSlave()`函数来选则一个晋升的从节点。
```
sentinelRedisInstance *sentinelSelectSlave( sentinelRedisInstance *master )
{
	/* 从节点数组 */
	sentinelRedisInstance **instance =
		zmalloc( sizeof(instance[0]) * dictSize( master->slaves ) );
	sentinelRedisInstance	*selected	= NULL;
	int			instances	= 0;
	dictIterator		*di;
	dictEntry		*de;
	mstime_t		max_master_down_time = 0;
	/* master主节点处于主观下线，计算出主节点被判断为处于主观下线的最大时长 */
	if ( master->flags & SRI_S_DOWN )
		max_master_down_time += mstime() - master->s_down_since_time;
	max_master_down_time += master->down_after_period * 10;

	di = dictGetIterator( master->slaves );
	/* 迭代下线的主节点的所有从节点 */
	while ( (de = dictNext( di ) ) != NULL )
	{
		sentinelRedisInstance	*slave = dictGetVal( de );
		mstime_t		info_validity_time;
		/* 跳过下线的从节点 */
		if ( slave->flags & (SRI_S_DOWN | SRI_O_DOWN) )
			continue;
		/* 跳过已经断开主从连接的从节点 */
		if ( slave->link->disconnected )
			continue;
		/* 跳过回复PING命令过于久远的从节点 */
		if ( mstime() - slave->link->last_avail_time > SENTINEL_PING_PERIOD * 5 )
			continue;
		/* 跳过优先级为0的从节点 */
		if ( slave->slave_priority == 0 )
			continue;

		/*
		 * 如果主节点处于主观下线状态，Sentinel每秒发送INFO命令给从节点，否则以默认的频率发送。
		 * 为了检查命令是否合法，因此计算一个延迟值
		 */
		if ( master->flags & SRI_S_DOWN )
			info_validity_time = SENTINEL_PING_PERIOD * 5;
		else
			info_validity_time = SENTINEL_INFO_PERIOD * 3;
		/* 如果从节点接受到INFO命令的回复已经过期，跳过该从节点 */
		if ( mstime() - slave->info_refresh > info_validity_time )
			continue;
		/* 跳过下线时间过长的从节点 */
		if ( slave->master_link_down_time > max_master_down_time )
			continue;
		/* 否则将选中的节点保存到数组中 */
		instance[instances++] = slave;
	}
	dictReleaseIterator( di );
	/* 如果有选中的从节点 */
	if ( instances )
	{
		/* 将数组中的从节点排序 */
		qsort( instance, instances, sizeof(sentinelRedisInstance*),
		       compareSlavesForPromotion );
		/* 将排序最低的从节点返回 */
		selected = instance[0];
	}
	zfree( instance );
	return(selected);
}
```
总结一下就是这些条件：
```
1. 不选有以下状态的从节点： S_DOWN, O_DOWN, DISCONNECTED.
2. 最近一次回复PING命令超过5s的从节点
3. 最近一次获取INFO命令回复的时间不超过`info_refresh`的三倍时间长度
4. 主从节点之间断开操作的时间不超过：从当前的Sentinel节点来看，主节点处于下线状态，从节点和主节点断开连接的时间不能超过down-after-period的10倍，这看起来非常魔幻（black magic），但是实际上，当主节点不可达时，主从连接会断开，但是必然不超过一定时间。意思是，主从断开，一定是主节点造成的，而不是从节点。无论如何，我们将根据复制偏移量选择最佳的从节点。
5. 从节点的优先级不能为0，优先级为0的从节点被抛弃。
```
如果以上条件都满足，那么按照一下顺序排序，`compareSlavesForPromotion()`函数指定排序方法：

-   最低的优先级的优先。
-   复制偏移量较大的优先。
-   运行`runid`字典序小的优先。
-   如果`runid`相同，那么选择执行命令更多的从节点。

因此，当选择出一个适合晋升的从节点后，`sentinelFailoverSelectSlave()`会打开该从节点的`SRI_PROMOTED`晋升标识，并且保存起来，最后更新故障转移到下一步状态。

##### 4.1.5.3 使从节点变为主节点

函数`sentinelFailoverSendSlaveOfNoOne()`会调用`sentinelSendSlaveOf()`函数发送一个`slaveof no one`命令，使从晋升的节点和原来的主节点断绝主从关系，成为新的主节点。
```
void sentinelFailoverSendSlaveOfNoOne( sentinelRedisInstance *ri )
{
	int retval;

	/* 如果要晋升的从节点处于断开连接的状态，那么不能发送命令。在当前状态，在规定的故障转移超时时间内可以重试。 */
	if ( ri->promoted_slave->link->disconnected )
	{
		/* 如果超出 配置的故障转移超时时间，那么中断本次故障转移后返回 */
		if ( mstime() - ri->failover_state_change_time > ri->failover_timeout )
		{
			sentinelEvent( LL_WARNING, "-failover-abort-slave-timeout", ri, "%@" );
			sentinelAbortFailover( ri );
		}
		return;
	}
	retval = sentinelSendSlaveOf( ri->promoted_slave, NULL, 0 );
	if ( retval != C_OK )
		return;
	/* 命令发送成功，发送事件通知 */
	sentinelEvent( LL_NOTICE, "+failover-state-wait-promotion",
		       ri->promoted_slave, "%@" );
	/* 设置故障转移状态为等待从节点晋升为主节点 */
	ri->failover_state = SENTINEL_FAILOVER_STATE_WAIT_PROMOTION;
	/* 更新故障转移操作状态改变时间 */
	ri->failover_state_change_time = mstime();
}
```
发送成功后，会更新故障转移状态到下一步状态。

##### 4.1.5.4 等待从节点晋升为主节点

调用`sentinelFailoverWaitPromotion()`来等待从节点晋升为主节点，但是该函数只是处理故障转移操作超时的情况。
```
void sentinelFailoverWaitPromotion( sentinelRedisInstance *ri )
{
	/* 所以，在这里只是处理故障转移超时的情况 */
	if ( mstime() - ri->failover_state_change_time > ri->failover_timeout )
	{
		/* 如果超出配置的故障转移超时时间，那么中断本次故障转移后返回 */
		sentinelEvent( LL_WARNING, "-failover-abort-slave-timeout", ri, "%@" );
		sentinelAbortFailover( ri );
	}
}
```
这个函数并没有更改故障转移操作的状态，因为，当从节点晋升为主节点时，故障转移状态的改变在处理`INFO`命令的回复时发生。

> `标题4.1.2`中的场景：连接的节点实例是从节点，但是`INFO`命令显示连接的是主节点。

所以，当下个时间事件发生（在故障转移设置的超时时间内），就会处理下一个故障转移的状态。如果等待从节点晋升为主节点超时，那么会调用`sentinelAbortFailover()`函数中止当前的故障转移操作，清空所有故障转移的状态，下个时间事件发生时重新执行。

##### 4.1.5.5 从节点同步新的主节点

调用`sentinelFailoverReconfNextSlave()`函数，给所有没有同步新主节点的从节点发送`SLAVE OF <new master address>`命令。
```
void sentinelFailoverReconfNextSlave( sentinelRedisInstance *master )
{
	dictIterator	*di;
	dictEntry	*de;
	int		in_progress = 0;

	di = dictGetIterator( master->slaves );
	/* 遍历所有的从节点 */
	while ( (de = dictNext( di ) ) != NULL )
	{
		sentinelRedisInstance *slave = dictGetVal( de );
		/* 计算处于已经发送同步命令或者已经正在同步的从节点 */
		if ( slave->flags & (SRI_RECONF_SENT | SRI_RECONF_INPROG) )
			in_progress++;
	}
	dictReleaseIterator( di );

	di = dictGetIterator( master->slaves );
	/* 如果已经发送同步命令或者已经正在同步的从节点个数小于设置的同步个数限制，那么遍历所有的从节点 */
	while ( in_progress < master->parallel_syncs &&
		(de = dictNext( di ) ) != NULL )
	{
		sentinelRedisInstance	*slave = dictGetVal( de );
		int			retval;

		/* 跳过被晋升的从节点和已经完成同步的从节点 */
		if ( slave->flags & (SRI_PROMOTED | SRI_RECONF_DONE) )
			continue;

		/* 如果从节点设置了发送slaveof命令，但是故障转移更新到下一个状态超时 */
		if ( (slave->flags & SRI_RECONF_SENT) &&
		     (mstime() - slave->slave_reconf_sent_time) >
		     SENTINEL_SLAVE_RECONF_TIMEOUT )
		{
			sentinelEvent( LL_NOTICE, "-slave-reconf-sent-timeout", slave, "%@" );
			/* 清除已发送slaveof命令的标识 */
			slave->flags &= ~SRI_RECONF_SENT;
			/* 设置为完成同步的标识，随后重新发送SLAVEOF命令，进行同步 */
			slave->flags |= SRI_RECONF_DONE;
		}

		/* 跳过已经发送了命令或者已经正在同步的从节点 */
		if ( slave->flags & (SRI_RECONF_SENT | SRI_RECONF_INPROG) )
			continue;
		/* 跳过连接断开的从节点 */
		if ( slave->link->disconnected )
			continue;

		/* Send SLAVEOF <new master>. */
		/* 发送 SLAVEOF <new master> 命令给从节点，包括刚才超时的从节点 */
		retval = sentinelSendSlaveOf( slave,
					      master->promoted_slave->addr->ip,
					      master->promoted_slave->addr->port );
		/* 如果发送成功 */
		if ( retval == C_OK )
		{
			/* 设置已经发送了SLAVEOF命令标识 */
			slave->flags |= SRI_RECONF_SENT;
			/* 设置发送slaveof命令的时间 */
			slave->slave_reconf_sent_time = mstime();
			sentinelEvent( LL_NOTICE, "+slave-reconf-sent", slave, "%@" );
			in_progress++;
		}
	}
	dictReleaseIterator( di );

	/* 判断故障转移是否结束 */
	sentinelFailoverDetectEnd( master );
}
```
主要是给**没有被发送同步新主节点命令的从节点**和**虽然发送但是同步超时的从节点**重新发送`SLAVEOF <new master>`命令。

函数最后调用了`sentinelFailoverDetectEnd()`函数来判断故障转移是否结束，但是结束的情况有两种：

-   故障转移超时被动结束
-   从节点已经完成同步新晋升的主节点结束
```
void sentinelFailoverDetectEnd( sentinelRedisInstance *master )
{
	int		not_reconfigured = 0, timeout = 0;
	dictIterator	*di;
	dictEntry	*de;
	/* 自从上次更新故障转移状态的时间差 */
	mstime_t elapsed = mstime() - master->failover_state_change_time;


	/* We can't consider failover finished if the promoted slave is
	 * not reachable. */
	/* 如果被晋升的从节点不可达，直接返回 */
	if ( master->promoted_slave == NULL ||
	     master->promoted_slave->flags & SRI_S_DOWN )
		return;


	/* The failover terminates once all the reachable slaves are properly
	 * configured. */
	/* 遍历所有的从节点，找出还没有完成同步从节点 */
	di = dictGetIterator( master->slaves );
	while ( (de = dictNext( di ) ) != NULL )
	{
		sentinelRedisInstance *slave = dictGetVal( de );
		/* 如果是被晋升为主节点的从节点或者是完成同步的从节点，则跳过 */
		if ( slave->flags & (SRI_PROMOTED | SRI_RECONF_DONE) )
			continue;
		/* 如果从节点处于客观下线，则跳过 */
		if ( slave->flags & SRI_S_DOWN )
			continue;
		/* 没有完成同步的节点数加1 */
		not_reconfigured++;
	}
	dictReleaseIterator( di );

	/* 强制结束故障转移超时的节点 */
	if ( elapsed > master->failover_timeout )
	{
		/* 忽略未完成同步的从节点 */
		not_reconfigured = 0;
		/* 设置超时标识 */
		timeout = 1;
		sentinelEvent( LL_WARNING, "+failover-end-for-timeout", master, "%@" );
	}

	/* 如果所有的从节点完成了同步，那么表示故障转移结束 */
	if ( not_reconfigured == 0 )
	{
		sentinelEvent( LL_WARNING, "+failover-end", master, "%@" );
		/* 监控晋升的主节点，更新配置 */
		master->failover_state = SENTINEL_FAILOVER_STATE_UPDATE_CONFIG;
		/* 更新故障转移操作状态改变时间 */
		master->failover_state_change_time = mstime();
	}

	/* 如果是因为超时导致故障转移结束 */
	if ( timeout )
	{
		dictIterator	*di;
		dictEntry	*de;

		di = dictGetIterator( master->slaves );
		/* 遍历所有的从节点 */
		while ( (de = dictNext( di ) ) != NULL )
		{
			sentinelRedisInstance	*slave = dictGetVal( de );
			int			retval;
			/* 跳过完成同步和发送同步slaveof命令的从节点 */
			if ( slave->flags & (SRI_RECONF_DONE | SRI_RECONF_SENT) )
				continue;
			/* 跳过连接断开的从节点 */
			if ( slave->link->disconnected )
				continue;
			/* 给没有被发送同步命令的从节点发送同步新晋升主节点的slaveof IP port 命令 */
			retval = sentinelSendSlaveOf( slave,
						      master->promoted_slave->addr->ip,
						      master->promoted_slave->addr->port );
			/* 如果发送成功，将这些从节点设置为已经发送slaveof命令的标识 */
			if ( retval == C_OK )
			{
				sentinelEvent( LL_NOTICE, "+slave-reconf-sent-be", slave, "%@" );
				slave->flags |= SRI_RECONF_SENT;
			}
		}
		dictReleaseIterator( di );
	}
}
```
该函数先会寻找没有完成同步的从节点，如果存在，则会强制将主节点的故障状态更新，如下：
```
master->failover_state = SENTINEL_FAILOVER_STATE_UPDATE_CONFIG;
```
这么做是为了让主进程继续向下执行，不要总是在此等待故障状态的变化。

虽然强制将主节点的故障状态更新，但是还是要将没有完成同步的从节点发送`slaveof IP port`让他们重新同步。

执行完这五步故障转移操作后，回到`sentinelHandleRedisInstance()`函数，该函数就剩最后一步操作了。

#### 4.1.6 更新主节点的状态

执行该函数，尝试发送`SENTINEL IS-MASTER-DOWN-BY-ADDR`给所有的哨兵节点获取回复，更新一些主节点状态。
```
sentinelAskMasterStateToOtherSentinels(ri,SENTINEL_NO_FLAGS);
```
> 上次在`标题4.1.4`时也执行过该函数，不过当时是指定强制发送命令的标识`SENTINEL_ASK_FORCED`。

这次则是有时间限制的发送`SENTINEL`命令，来更新其他哨兵节点对主节点是否下线的判断。有可能发生主节点在故障转移时重新上线的情况。

> 在`标题 4.1.5`时讨论过该情况。

当执行完这一步，`sentinelHandleRedisInstance()`的所有操作全部剖析完成。

----------

以上就是标题`4.1周期性操作`的全部内容，还记得`sentinelHandleDictOfRedisInstances`函数递归的对当前哨兵所监控的所有主节点`sentinel.masters`，和所有主节点的所有从节点`ri->slaves`，和所有监控该主节点的其他所有哨兵节点`ri->sentinels`执行周期性操作。因此，执行完所有类型的节点的周期性任务之后，会接下来处理主从切换的情况。

### 4.2 处理主从切换

`sentinel.c`文件详细注释：[Redis Sentinel详细注释](https://github.com/menwengit/redis_source_annotation)

当执行完所有类型的节点的周期性任务之后，会执行`sentinelHandleDictOfRedisInstances()`下面的代码：
```
while ( (de = dictNext( di ) ) != NULL )
{
	sentinelRedisInstance *ri = dictGetVal( de );
	/* 递归对所有节点执行周期性操作 */
	... ...
	/* 如果ri实例处于完成故障转移操作的状态，所有从节点已经完成对新主节点的同步 */
	if ( ri->failover_state == SENTINEL_FAILOVER_STATE_UPDATE_CONFIG )
	{
		/* 设置主从转换的标识 */
		switch_to_promoted = ri;
	}
}
/* 如果主从节点发生了转换 */
if ( switch_to_promoted )
	/*
	 * 将原来的主节点从主节点表中删除，并用晋升的主节点替代
	 * 意味着已经用新晋升的主节点代替旧的主节点，包括所有从节点和旧的主节点从属当前新的主节点
	 */
	sentinelFailoverSwitchToPromotedSlave( switch_to_promoted );
dictReleaseIterator( di );
```
还记得当前强制将主节点的故障状态更新的状态吗？对，就是`SENTINEL_FAILOVER_STATE_UPDATE_CONFIG`状态。这个状态表示已经完成**在故障转移状态下**，所有从节点对新主节点的同步操作。因此需要调用`sentinelFailoverSwitchToPromotedSlave()`函数特殊处理发送`主从切换`的情况。

该函数会发送事件通知然后调用`sentinelResetMasterAndChangeAddress()`来用新晋升的主节点代替旧的主节点，包括所有从节点和旧的主节点从属当前新的主节点。
```
int sentinelResetMasterAndChangeAddress( sentinelRedisInstance *master, char *ip, int port )
{
	sentinelAddr	*oldaddr, *newaddr;
	sentinelAddr	**slaves	= NULL;
	int		numslaves	= 0, j;
	dictIterator	*di;
	dictEntry	*de;

	/* 创建ip:port地址字符串 */
	newaddr = createSentinelAddr( ip, port );
	if ( newaddr == NULL )
		return(C_ERR);

	/*
	 * 创建一个从节点表，将重置后的主节点添加到该表中
	 * 不包含有我们要转换地址的那一个从节点
	 */
	di = dictGetIterator( master->slaves );
	/* 遍历所有的从节点 */
	while ( (de = dictNext( di ) ) != NULL )
	{
		sentinelRedisInstance *slave = dictGetVal( de );
		/* 如果当前从节点的地址和指定的地址相同，说明该从节点是要晋升为主节点的，因此跳过该从节点 */
		if ( sentinelAddrIsEqual( slave->addr, newaddr ) )
			continue;
		/* 否则将该从节点加入到一个数组中 */
		slaves			= zrealloc( slaves, sizeof(sentinelAddr*) * (numslaves + 1) );
		slaves[numslaves++]	= createSentinelAddr( slave->addr->ip,
							      slave->addr->port );
	}
	dictReleaseIterator( di );

	/* 如果指定的地址和主节点地址不相同，说明，该主节点是要被替换的，那么将该主节点地址加入到从节点数组中 */
	if ( !sentinelAddrIsEqual( newaddr, master->addr ) )
	{
		slaves			= zrealloc( slaves, sizeof(sentinelAddr*) * (numslaves + 1) );
		slaves[numslaves++]	= createSentinelAddr( master->addr->ip,
							      master->addr->port );
	}

	/* 重置主节点，但不删除所有监控自己的Sentinel节点 */
	sentinelResetMaster( master, SENTINEL_RESET_NO_SENTINELS );
	/* 备份旧地址 */
	oldaddr = master->addr;
	/* 设置新地址 */
	master->addr = newaddr;
	/* 下线时间清零 */
	master->o_down_since_time	= 0;
	master->s_down_since_time	= 0;

	/* Add slaves back. */
	/* 为新的主节点加入从节点 */
	for ( j = 0; j < numslaves; j++ )
	{
		sentinelRedisInstance *slave;
		/* 遍历所有的从节点表，创建从节点实例，并将该实例从属到当前新的主节点中 */
		slave = createSentinelRedisInstance( NULL, SRI_SLAVE, slaves[j]->ip,
						     slaves[j]->port, master->quorum, master );
		/* 释放原有的表中的从节点 */
		releaseSentinelAddr( slaves[j] );
		/* 事件通知 */
		if ( slave )
			sentinelEvent( LL_NOTICE, "+slave", slave, "%@" );
	}
	/* 释放从节点表 */
	zfree( slaves );

	/* 将原主节点地址释放 */
	releaseSentinelAddr( oldaddr );
	/* 刷新配置文件 */
	sentinelFlushConfig();
	return(C_OK);
}
```
