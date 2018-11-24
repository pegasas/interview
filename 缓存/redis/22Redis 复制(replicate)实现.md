
# Redis源码剖析和注释（二十二）--- Redis 复制(replicate)源码详细解析
# Redis 复制(replicate)实现

## 1. 复制的介绍

`Redis`为了解决单点数据库问题，会把数据复制多个副本部署到其他节点上，通过复制，实现`Redis`的**高可用性**，实现对数据的**冗余备份**，保证**数据和服务的高度可靠性**。

关于复制的详细配置和如何建立复制，请参考：[Redis 复制功能详解](http://blog.csdn.net/men_wen/article/details/72590550)  。

-   [Redis 复制replicate实现](https://blog.csdn.net/men_wen/article/details/72628439#redis-%E5%A4%8D%E5%88%B6replicate%E5%AE%9E%E7%8E%B0)
    -   [复制的介绍](https://blog.csdn.net/men_wen/article/details/72628439#1-%E5%A4%8D%E5%88%B6%E7%9A%84%E4%BB%8B%E7%BB%8D)
    -   [复制的实现](https://blog.csdn.net/men_wen/article/details/72628439#2-%E5%A4%8D%E5%88%B6%E7%9A%84%E5%AE%9E%E7%8E%B0)
        -   [1 主从关系的建立](https://blog.csdn.net/men_wen/article/details/72628439#21-%E4%B8%BB%E4%BB%8E%E5%85%B3%E7%B3%BB%E7%9A%84%E5%BB%BA%E7%AB%8B)
        -   [2 主从网络连接建立](https://blog.csdn.net/men_wen/article/details/72628439#22-%E4%B8%BB%E4%BB%8E%E7%BD%91%E7%BB%9C%E8%BF%9E%E6%8E%A5%E5%BB%BA%E7%AB%8B)
        -   [3 发送PING命令](https://blog.csdn.net/men_wen/article/details/72628439#23-%E5%8F%91%E9%80%81ping%E5%91%BD%E4%BB%A4)
        -   [4 认证权限](https://blog.csdn.net/men_wen/article/details/72628439#24-%E8%AE%A4%E8%AF%81%E6%9D%83%E9%99%90)
        -   [5 发送端口号](https://blog.csdn.net/men_wen/article/details/72628439#25-%E5%8F%91%E9%80%81%E7%AB%AF%E5%8F%A3%E5%8F%B7)
        -   [6 发送 IP 地址](https://blog.csdn.net/men_wen/article/details/72628439#26-%E5%8F%91%E9%80%81-ip-%E5%9C%B0%E5%9D%80)
        -   [7 发送能力capability](https://blog.csdn.net/men_wen/article/details/72628439#27-%E5%8F%91%E9%80%81%E8%83%BD%E5%8A%9Bcapability)
        -   [8 发送PSYNC命令](https://blog.csdn.net/men_wen/article/details/72628439#28-%E5%8F%91%E9%80%81psync%E5%91%BD%E4%BB%A4)
        -   [9 发送输出缓冲区数据](https://blog.csdn.net/men_wen/article/details/72628439#29-%E5%8F%91%E9%80%81%E8%BE%93%E5%87%BA%E7%BC%93%E5%86%B2%E5%8C%BA%E6%95%B0%E6%8D%AE)
        -   [10 命令传播](https://blog.csdn.net/men_wen/article/details/72628439#210-%E5%91%BD%E4%BB%A4%E4%BC%A0%E6%92%AD)
    -   [部分重同步实现](https://blog.csdn.net/men_wen/article/details/72628439#3-%E9%83%A8%E5%88%86%E9%87%8D%E5%90%8C%E6%AD%A5%E5%AE%9E%E7%8E%B0)
        -   [1 心跳机制](https://blog.csdn.net/men_wen/article/details/72628439#31-%E5%BF%83%E8%B7%B3%E6%9C%BA%E5%88%B6)
        -   [2 复制积压缓冲区backlog](https://blog.csdn.net/men_wen/article/details/72628439#32-%E5%A4%8D%E5%88%B6%E7%A7%AF%E5%8E%8B%E7%BC%93%E5%86%B2%E5%8C%BAbacklog)

## 2. 复制的实现

本文主要剖析：

-   **第一次执行复制所进行全量同步的全过程**
-   **部分重同步的实现**
> `replication.c`文件详细注释：[Redis 复制代码注释](https://github.com/menwengit/redis_source_annotation)

### 2.1 主从关系的建立

复制的建立方法有三种。

1.  在`redis.conf`文件中配置`slaveof <masterip> <masterport>`选项，然后指定该配置文件启动`Redis`生效。
2.  在`redis-server`启动命令后加上`--slaveof <masterip> <masterport>`启动生效。
3.  直接使用  `slaveof <masterip> <masterport>`命令在从节点执行生效。

无论是通过哪一种方式来建立主从复制，都是**从节点**来执行`slaveof`命令，那么从节点执行了这个命令到底做了什么，我们上源码：
```
/* SLAVEOF host port命令实现 */
void slaveofCommand( client *c )
{
	/* 如果当前处于集群模式，不能进行复制操作 */
	if ( server.cluster_enabled )
	{
		addReplyError( c, "SLAVEOF not allowed in cluster mode." );
		return;
	}

	/* SLAVEOF NO ONE命令使得这个从节点关闭复制功能，并从从节点转变回主节点，原来同步所得的数据集不会被丢弃。 */
	if ( !strcasecmp( c->argv[1]->ptr, "no" ) &&
	     !strcasecmp( c->argv[2]->ptr, "one" ) )
	{
		/* 如果保存了主节点IP */
		if ( server.masterhost )
		{
			/* 取消复制操作，设置服务器为主服务器 */
			replicationUnsetMaster();
			/* 获取client的每种信息，并以sds形式返回，并打印到日志中 */
			sds client = catClientInfoString( sdsempty(), c );
			serverLog( LL_NOTICE, "MASTER MODE enabled (user request from '%s')",
				   client );
			sdsfree( client );
		}
		/* SLAVEOF host port */
	} else {
		long port;

		/* 获取端口号 */
		if ( (getLongFromObjectOrReply( c, c->argv[2], &port, NULL ) != C_OK) )
			return;

		/* 如果已存在从属于masterhost主节点且命令参数指定的主节点和masterhost相等，端口也相等，直接返回 */
		if ( server.masterhost && !strcasecmp( server.masterhost, c->argv[1]->ptr )
		     && server.masterport == port )
		{
			serverLog( LL_NOTICE, "SLAVE OF would result into synchronization with the master we are already connected with. No operation performed." );
			addReplySds( c, sdsnew( "+OK Already connected to specified master\r\n" ) );
			return;
		}
		/*
		 * 第一次执行设置端口和ip，或者是重新设置端口和IP
		 * 设置服务器复制操作的主节点IP和端口
		 */
		replicationSetMaster( c->argv[1]->ptr, port );
		/* 获取client的每种信息，并以sds形式返回，并打印到日志中 */
		sds client = catClientInfoString( sdsempty(), c );
		serverLog( LL_NOTICE, "SLAVE OF %s:%d enabled (user request from '%s')",
			   server.masterhost, server.masterport, client );
		sdsfree( client );
	}
	/* 回复ok */
	addReply( c, shared.ok );
}
```
当从节点的client执行`SLAVEOF`命令后，该命令会被构建成`Redis`协议格式，发送给从节点服务器，然后节点服务器会调用`slaveofCommand()`函数执行该命令。

> 具体的命令接受和回复请参考：[Redis 网络连接库剖析](http://blog.csdn.net/men_wen/article/details/72084617)

而`SLAVEOF`命令做的操作并不多，主要以下三步：

-   判断当前环境是否在集群模式下，因为集群模式下不行执行该命令。
-   是否执行的是`SLAVEOF NO ONE`命令，该命令会断开主从的关系，设置当前节点为主节点服务器。
-   设置从节点所属主节点的`IP`和`port`。调用了`replicationSetMaster()`函数。

`SLAVEOF`命令能做的只有这么多，我们来具体看下`replicationSetMaster()`函数的代码，看看它做了哪些与复制相关的操作。
```
/* 设置复制操作的主节点IP和端口 */
void replicationSetMaster( char *ip, int port )
{
	/* 按需清除原来的主节点信息 */
	sdsfree( server.masterhost );
	/* 设置ip和端口 */
	server.masterhost	= sdsnew( ip );
	server.masterport	= port;
	/*
	 * 如果有其他的主节点，在释放
	 * 例如服务器1是服务器2的主节点，现在服务器2要同步服务器3，服务器3要成为服务器2的主节点，因此要释放服务器1
	 */
	if ( server.master )
		freeClient( server.master );
	/* 解除所有客户端的阻塞状态 */
	disconnectAllBlockedClients();          /* Clients blocked in master, now slave. */
	/* 关闭所有从节点服务器的连接，强制从节点服务器进行重新同步操作 */
	disconnectSlaves();                     /* Force our slaves to resync with us as well. */
	/* 释放主节点结构的缓存，不会执行部分重同步PSYNC */
	replicationDiscardCachedMaster();       /* Don't try a PSYNC. */
	/* 释放复制积压缓冲区 */
	freeReplicationBacklog();               /* Don't allow our chained slaves to PSYNC. */
	/* 取消执行复制操作 */
	cancelReplicationHandshake();
	/* 设置复制必须重新连接主节点的状态 */
	server.repl_state = REPL_STATE_CONNECT;
	/* 初始化复制的偏移量 */
	server.master_repl_offset = 0;
	/* 清零连接断开的时长 */
	server.repl_down_since = 0;
}
```
由代码知，`replicationSetMaster()`函数执行操作的也很简单，总结为两步：

-   清理之前所属的主节点的信息。
-   设置新的主节点`IP`和`port`等。

因为，当前从节点有可能之前从属于另外的一个主节点服务器，因此要清理所有关于之前主节点的缓存、关闭旧的连接等等。然后设置该从节点的新主节点，设置了`IP`和`port`，还设置了以下状态：
```
/* 设置复制必须重新连接主节点的状态 */
server.repl_state = REPL_STATE_CONNECT;
/* 初始化全局复制的偏移量 */
server.master_repl_offset = 0;
```
然后，就没有然后了，然后就会执行复制操作吗？这也没有什么关于复制的操作执行了，那么复制操作是怎么开始的呢？

### 2.2 主从网络连接建立

`slaveof`**命令是一个异步命令，执行命令时，从节点保存主节点的信息，确立主从关系后就会立即返回，后续的复制流程在节点内部异步执行。**那么，如何触发复制的执行呢？

周期性执行的函数：`replicationCron()`函数，该函数被服务器的时间事件的回调函数`serverCron()`所调用，而`serverCron()`函数在`Redis`服务器初始化时，被设置为时间事件的处理函数。
```
/* void initServer(void) Redis服务器初始化 */
aeCreateTimeEvent( server.el, 1, serverCron, NULL, NULL )
```
> [Redis 单机服务器实现源码剖析](http://blog.csdn.net/men_wen/article/details/72455944)
> 
> [Redis 文件事件和时间事件处理实现源码剖析](http://blog.csdn.net/men_wen/article/details/71514524)

`replicationCron()`函数执行频率为1秒一次：
```
/*
 * 节选自serverCron函数
 * 周期性执行复制的任务
 */
run_with_period( 1000 ) replicationCron();
```
主从关系建立后，从节点服务器的`server.repl_state`被设置为`REPL_STATE_CONNECT`，而`replicationCron()`函数会被每秒执行一次，该函数会发现**我（从节点）现在有主节点了**，而且**我要的状态是要连接主节点（REPL_STATE_CONNECT）**。

`replicationCron()`函数处理这以情况的代码如下：
```
/* Check if we should connect to a MASTER */
/* 如果处于要必须连接主节点的状态，尝试连接 */
if ( server.repl_state == REPL_STATE_CONNECT )
{
	serverLog( LL_NOTICE, "Connecting to MASTER %s:%d",
		   server.masterhost, server.masterport );
	/* 以非阻塞的方式连接主节点 */
	if ( connectWithMaster() == C_OK )
	{
		serverLog( LL_NOTICE, "MASTER <-> SLAVE sync started" );
	}
}
```
`replicationCron()`函数根据从节点的状态，调用`connectWithMaster()`非阻塞连接主节点。代码如下：
```
/* 以非阻塞的方式连接主节点 */
int connectWithMaster( void )
{
	int fd;

	/* 连接主节点 */
	fd = anetTcpNonBlockBestEffortBindConnect( NULL,
						   server.masterhost, server.masterport, NET_FIRST_BIND_ADDR );
	if ( fd == -1 )
	{
		serverLog( LL_WARNING, "Unable to connect to MASTER: %s",
			   strerror( errno ) );
		return(C_ERR);
	}

	/* 监听主节点fd的可读和可写事件的发生，并设置其处理程序为syncWithMaster */
	if ( aeCreateFileEvent( server.el, fd, AE_READABLE | AE_WRITABLE, syncWithMaster, NULL ) == AE_ERR )
	{
		close( fd );
		serverLog( LL_WARNING, "Can't create readable event for SYNC" );
		return(C_ERR);
	}

	/* 最近一次读到RDB文件内容的时间 */
	server.repl_transfer_lastio = server.unixtime;
	/* 从节点和主节点的同步套接字 */
	server.repl_transfer_s = fd;
	/* 处于和主节点正在连接的状态 */
	server.repl_state = REPL_STATE_CONNECTING;
	return(C_OK);
}
```
`connectWithMaster()`函数执行的操作可以总结为：

-   根据`IP`和`port`非阻塞的方式连接主节点，得到主从节点进行通信的`文件描述符fd`，并保存到从节点服务器`server.repl_transfer_s`中，并且将刚才的`REPL_STATE_CONNECT`状态设置为`REPL_STATE_CONNECTING`。
-   监听`fd`的可读和可写事件，并且设置事件发生的处理程序`syncWithMaster()`函数。

至此，主从网络建立就完成了。

### 2.3 发送PING命令

主从建立网络时，同时注册`fd`的`AE_READABLE|AE_WRITABLE`事件，因此会触发一个`AE_WRITABLE`事件，调用`syncWithMaster()`函数，处理写事件。

根据当前的`REPL_STATE_CONNECTING`状态，从节点向主节点发送`PING`命令，`PING`命令的目的有：

1.  检测主从节点之间的网络是否可用。
2.  检查主从节点当前是否接受处理命令。

`syncWithMaster()`函数中相关操作的代码如下：
```
/* Send a PING to check the master is able to reply without errors. */
/* 如果复制的状态为REPL_STATE_CONNECTING，发送一个PING去检查主节点是否能正确回复一个PONG */
if ( server.repl_state == REPL_STATE_CONNECTING )
{
	serverLog( LL_NOTICE, "Non blocking connect for SYNC fired the event." );
	/* 暂时取消接听fd的写事件，以便等待PONG回复时，注册可读事件 */
	aeDeleteFileEvent( server.el, fd, AE_WRITABLE );
	/* 设置复制状态为等待PONG回复 */
	server.repl_state = REPL_STATE_RECEIVE_PONG;
	/* 发送一个PING命令 */
	err = sendSynchronousCommand( SYNC_CMD_WRITE, fd, "PING", NULL );
	if ( err )
		goto write_error;
	return;
}
```
发送`PING`命令主要的操作是：

-   先取消监听`fd`的写事件，因为接下来要**读主节点服务器发送过来的`PONG`回复**，因此只监听可读事件的发生。
-   设置从节点的复制状态为`REPL_STATE_RECEIVE_PONG`。等待一个主节点回复一个`PONG`命令。
-   以写的方式调用`sendSynchronousCommand()`函数发送一个`PING`命令给主节点。

主节点服务器从`fd`会读到一个`PING`命令，然后会回复一个`PONG`命令到`fd`中，执行的命令就是`addReply(c,shared.pong);`。

此时，会触发`fd`的可读事件，调用`syncWithMaster()`函数来处理，此时从节点的复制状态为`REPL_STATE_RECEIVE_PONG`，等待主节点回复`PONG`。`syncWithMaster()`函数中处理这一状态的代码如下：
```
/* Receive the PONG command. */
/* 如果复制的状态为REPL_STATE_RECEIVE_PONG，等待接受PONG命令 */
if ( server.repl_state == REPL_STATE_RECEIVE_PONG )
{
	/* 从主节点读一个PONG命令sendSynchronousCommand */
	err = sendSynchronousCommand( SYNC_CMD_READ, fd, NULL );

	/*
	 * 只接受两种有效的回复。一种是 "+PONG"，一种是认证错误"-NOAUTH"。
	 * 旧版本的返回有"-ERR operation not permitted"
	 */
	if ( err[0] != '+' &&
	     strncmp( err, "-NOAUTH", 7 ) != 0 &&
	     strncmp( err, "-ERR operation not permitted", 28 ) != 0 )
	{       /* 没有收到正确的PING命令的回复 */
		serverLog( LL_WARNING, "Error reply to PING from master: '%s'", err );
		sdsfree( err );
		goto error;
	} else {
		serverLog( LL_NOTICE, "Master replied to PING, replication can continue..." );
	}
	sdsfree( err );
	/* 已经收到PONG，更改状态设置为发送认证命令AUTH给主节点 */
	server.repl_state = REPL_STATE_SEND_AUTH;
}
```
在这里，以读的方式调用`sendSynchronousCommand()`，并将读到的`"+PONG\r\n"`返回到`err`中，如果从节点正确接收到主节点发送的`PONG`命令，会将从节点的复制状态设置为`server.repl_state = REPL_STATE_SEND_AUTH`。等待进行权限的认证。

### 2.4 认证权限

权限认证会在`syncWithMaster()`函数继续执行，紧接着刚才的代码：
```
/* AUTH with the master if required. */
/* 如果需要，发送AUTH认证给主节点 */
if ( server.repl_state == REPL_STATE_SEND_AUTH )
{
	/* 如果服务器设置了认证密码 */
	if ( server.masterauth )
	{
		/* 写AUTH给主节点 */
		err = sendSynchronousCommand( SYNC_CMD_WRITE, fd, "AUTH", server.masterauth, NULL );
		if ( err )
			goto write_error;
		/* 设置状态为等待接受认证回复 */
		server.repl_state = REPL_STATE_RECEIVE_AUTH;
		return;
		/* 如果没有设置认证密码，直接设置复制状态为发送端口号给主节点 */
	} else {
		server.repl_state = REPL_STATE_SEND_PORT;
	}
}
```
如果从节点的服务器设置了认证密码，则会以写方式调用`sendSynchronousCommand()`函数，将`AUTH`命令和密码写到`fd`中，并且将从节点的复制状态设置为`server.repl_state = REPL_STATE_RECEIVE_AUTH`，接受`AUTH`的验证。

如果从节点服务器没有设置认证密码，就直接将从节点的复制状态设置为`server.repl_state = REPL_STATE_SEND_PORT`，准发发送一个端口号。

主节点会读取到`AUTH`命令，调用`authCommand()`函数来处理，主节点服务器会比较从节点发送过来的`server.masterauth`和主节点服务器保存的`server.requirepass`是否一致，如果一致，会回复一个`"+OK\r\n"`。

当主节点将回复写到`fd`时，又会触发从节点的可读事件，紧接着调用`syncWithMaster()`函数来处理接收`AUTH`认证结果：
```
/* Receive AUTH reply. */
/* 接受AUTH认证的回复 */
if ( server.repl_state == REPL_STATE_RECEIVE_AUTH )
{
	/* 从主节点读回复 */
	err = sendSynchronousCommand( SYNC_CMD_READ, fd, NULL );
	/* 回复错误，认证失败 */
	if ( err[0] == '-' )
	{
		serverLog( LL_WARNING, "Unable to AUTH to MASTER: %s", err );
		sdsfree( err );
		goto error;
	}
	sdsfree( err );
	/* 设置复制状态为发送端口号给主节点 */
	server.repl_state = REPL_STATE_SEND_PORT;
}
```
以读方式从`fd`中读取一个回复，判断认证是否成功，认证成功，则会将从节点的复制状态设置为`server.repl_state = REPL_STATE_SEND_PORT`表示要发送一个端口号给主节点。这和没有设置认证的情况结果相同。

### 2.5 发送端口号

从节点在认证完权限后，会继续在`syncWithMaster()`函数执行，处理发送端口号的状态。
```
/* Set the slave port, so that Master's INFO command can list the
 * slave listening port correctly. */
/* 如果复制状态是，发送从节点端口号给主节点，主节点的INFO命令就能够列出从节点正在监听的端口号 */
if ( server.repl_state == REPL_STATE_SEND_PORT )
{
	/* 获取端口号 */
	sds port = sdsfromlonglong( server.slave_announce_port ?
				    server.slave_announce_port : server.port );
	/* 将端口号写给主节点 */
	err = sendSynchronousCommand( SYNC_CMD_WRITE, fd, "REPLCONF", "listening-port", port, NULL );
	sdsfree( port );
	if ( err )
		goto write_error;
	sdsfree( err );
	/* 设置复制状态为接受端口号 */
	server.repl_state = REPL_STATE_RECEIVE_PORT;
	return;
}
```
发送端口号，以`REPLCONF listening-port`命令的方式，写到`fd`中。然后将复制状态设置为`server.repl_state = REPL_STATE_RECEIVE_PORT`，等待接受主节点的回复。

主节点从`fd`中读到`REPLCONF listening-port <port>`命令，调用`replconfCommand()`命令来处理，而`replconfCommand()`函数的定义就在`replication.c`文件中，`REPLCONF`命令可以设置多种不同的选项，解析到端口号后，将端口号保存从节点对应client状态的`c->slave_listening_port = port`中。最终回复一个`"+OK\r\n"`状态的回复，写在`fd`中。

当主节点将回复写到`fd`时，又会触发从节点的可读事件，紧接着调用`syncWithMaster()`函数来处理接受端口号，验证主节点是否正确的接收到从节点的端口号。
```
/* Receive REPLCONF listening-port reply. */
/* 复制状态为接受端口号 */
if ( server.repl_state == REPL_STATE_RECEIVE_PORT )
{
	/* 从主节点读取端口号 */
	err = sendSynchronousCommand( SYNC_CMD_READ, fd, NULL );


	/* Ignore the error if any, not all the Redis versions support
	 * REPLCONF listening-port. */
	/* 忽略所有的错误，因为不是所有的Redis版本都支持REPLCONF listening-port命令 */
	if ( err[0] == '-' )
	{
		serverLog( LL_NOTICE, "(Non critical) Master does not understand REPLCONF listening-port: %s", err );
	}
	sdsfree( err );
	/* 设置复制状态为发送IP */
	server.repl_state = REPL_STATE_SEND_IP;
}
```
如果主节点正确的接收到从节点的端口号，会将从节点的复制状态设置为`server.repl_state = REPL_STATE_SEND_IP`表示要送一个`IP`给主节点。

### 2.6 发送 IP 地址

从节点发送完端口号并且正确收到主节点的回复后，紧接着`syncWithMaster()`函数执行发送`IP`的代码。发送`IP`和发送端口号过程几乎一致。
```
/* 复制状态为发送IP */
if ( server.repl_state == REPL_STATE_SEND_IP )
{
	/* 将IP写给主节点 */
	err = sendSynchronousCommand( SYNC_CMD_WRITE, fd, "REPLCONF", "ip-address", server.slave_announce_ip, NULL );
	if ( err )
		goto write_error;
	sdsfree( err );
	/* 设置复制状态为接受IP */
	server.repl_state = REPL_STATE_RECEIVE_IP;
	return;
}
```
同样是以`REPLCONF ip-address`命令的方式，将从节点的`IP`写到`fd`中。并且设置从节点的复制状态为`server.repl_state = REPL_STATE_RECEIVE_IP`，等待接受主节点的回复。然后就直接返回，等待`fd`可读发生。

主节点仍然会调用`replication.c`文件中实现的`replconfCommand()`函数来处理`REPLCONF`命令，解析出`REPLCONF ip-address ip`命令，保存从节点的`ip`到主节点的对应从节点的client的`c->slave_ip`中。然后回复`"+OK\r\n"`状态，写到`fd`中。

此时，从节点监听到`fd`触发了可读事件，会调用`syncWithMaster()`函数来处理，验证主节点是否正确接收到从节点的`IP`。
```
/* Receive REPLCONF ip-address reply. */
/* 复制状态为接受IP回复 */
if ( server.repl_state == REPL_STATE_RECEIVE_IP )
{
	/* 从主节点读一个IP回复 */
	err = sendSynchronousCommand( SYNC_CMD_READ, fd, NULL );
	/* 错误回复 */
	if ( err[0] == '-' )
	{
		serverLog( LL_NOTICE, "(Non critical) Master does not understand REPLCONF ip-address: %s", err );
	}
	sdsfree( err );
	/* 设置复制状态为发送一个capa（能力？能否解析出RDB文件的EOF流格式） */
	server.repl_state = REPL_STATE_SEND_CAPA;
}
```
如果主节点正确接收了从节点`IP`，就会设置从节点的复制状态`server.repl_state = REPL_STATE_SEND_CAPA`表示发送从节点的能力（capability）。

### 2.7 发送能力（capability）

发送能力和发送端口和`IP`也是如出一辙，紧接着`syncWithMaster()`函数执行发送`capa`的代码。
```
/* 复制状态为发送capa，通知主节点从节点的能力 */
if ( server.repl_state == REPL_STATE_SEND_CAPA )
{
	/* 将从节点的capa写给主节点 */
	err = sendSynchronousCommand( SYNC_CMD_WRITE, fd, "REPLCONF", "capa", "eof", NULL );
	if ( err )
		goto write_error;
	sdsfree( err );
	/* 设置复制状态为接受从节点的capa */
	server.repl_state = REPL_STATE_RECEIVE_CAPA;
	return;
}
```
从节点将`REPLCONF capa eof`命令发送给主节点，写到`fd`中。

> 目前只支持一种能力，就是能够解析出RDB文件的EOF流格式。用于无盘复制的方式中。

主节点仍然会调用`replication.c`文件中实现的`replconfCommand()`函数来处理`REPLCONF`命令，解析出`REPLCONF capa eof`命令，将`eof`对应的标识，**按位与**到主节点的对应从节点的client的`c->slave_capa`中。然后回复`"+OK\r\n"`状态，写到`fd`中。

此时，从节点监听到`fd`触发了可读事件，会调用`syncWithMaster()`函数来处理，验证主节点是否正确接收到从节点的`capa`。
```
/* Receive CAPA reply. */
/* 复制状态为接受从节点的capa回复 */
if ( server.repl_state == REPL_STATE_RECEIVE_CAPA )
{
	/* 从主节点读取capa回复 */
	err = sendSynchronousCommand( SYNC_CMD_READ, fd, NULL );
	/* 错误回复 */
	if ( err[0] == '-' )
	{
		serverLog( LL_NOTICE, "(Non critical) Master does not understand REPLCONF capa: %s", err );
	}
	sdsfree( err );
	/* 设置复制状态为发送PSYNC命令 */
	server.repl_state = REPL_STATE_SEND_PSYNC;
}
```
如果主节点正确接收了从节点`capa`，就会设置从节点的复制状态`server.repl_state = REPL_STATE_SEND_PSYNC`表示发送一个`PSYNC`命令。

### 2.8 发送PSYNC命令

> `replication.c`文件详细注释：[Redis 复制代码注释](https://github.com/menwengit/redis_source_annotation)

从节点发送`PSYNC`命令给主节点，尝试进行同步主节点的数据集。同步分为两种：

-   全量同步：第一次执行复制的场景。
-   部分同步：用于主从复制因为网络中断等原因造成数据丢失的场景。

**因为这是第一次执行同步，因此会进行全量同步。**
```
/*
 * 复制状态为发送PSYNC命令。尝试进行部分重同步。
 * 如果没有缓冲主节点的结构，slaveTryPartialResynchronization()函数将会至少尝试使用PSYNC去进行一个全同步，这样就能得到主节点的运行runid和全局复制偏移量。并且在下次重连接时可以尝试进行部分重同步。
 */
if ( server.repl_state == REPL_STATE_SEND_PSYNC )
{
	/* 向主节点发送一个部分重同步命令PSYNC，参数0表示不读主节点的回复，只获取主节点的运行runid和全局复制偏移量 */
	if ( slaveTryPartialResynchronization( fd, 0 ) == PSYNC_WRITE_ERROR )
	{
		/* 发送PSYNC出错 */
		err = sdsnew( "Write error sending the PSYNC command." );
		goto write_error;
	}
	/* 设置复制状态为等待接受一个PSYNC回复 */
	server.repl_state = REPL_STATE_RECEIVE_PSYNC;
	return;
}
```
从节点调用`slaveTryPartialResynchronization()`函数尝试进行重同步，注意第二个参数是`0`。因为`slaveTryPartialResynchronization()`分成两部分，一部分是写，一部分是读，因为第二个参数是`0`，因此执行写的一部分，发送一个`PSYNC`命令给主节点。只列举出写的部分
```
/* Writing half */
/* 如果read_reply为0，则该函数往socket上会写入一个PSYNC命令 */
if ( !read_reply )
{
	/*
	 * 将repl_master_initial_offset设置为-1表示主节点的run_id和全局复制偏移量是无效的。
	 * 如果能使用PSYNC命令执行一个全量同步，会正确设置全复制偏移量，以便这个信息被正确传播主节点的所有从节点中
	 */
	server.repl_master_initial_offset = -1;

	/* 主节点的缓存不为空，可以尝试进行部分重同步。PSYNC <master_run_id> <repl_offset> */
	if ( server.cached_master )
	{
		/* 保存缓存runid */
		psync_runid = server.cached_master->replrunid;
		/* 获取已经复制的偏移量 */
		snprintf( psync_offset, sizeof(psync_offset), "%lld", server.cached_master->reploff + 1 );
		serverLog( LL_NOTICE, "Trying a partial resynchronization (request %s:%s).", psync_runid, psync_offset );
		/* 主节点的缓存为空，发送PSYNC ? -1。请求全量同步 */
	} else {
		serverLog( LL_NOTICE, "Partial resynchronization not possible (no cached master)" );
		psync_runid = "?";
		memcpy( psync_offset, "-1", 3 );
	}

	/* Issue the PSYNC command */
	/* 发送一个PSYNC命令给主节点 */
	reply = sendSynchronousCommand( SYNC_CMD_WRITE, fd, "PSYNC", psync_runid, psync_offset, NULL );
	/* 写成功失败会返回一个"-"开头的字符串 */
	if ( reply != NULL )
	{
		serverLog( LL_WARNING, "Unable to send PSYNC to master: %s", reply );
		sdsfree( reply );
		/* 删除文件的可读事件，返回写错误PSYNC_WRITE_ERROR */
		aeDeleteFileEvent( server.el, fd, AE_READABLE );
		return(PSYNC_WRITE_ERROR);
	}
	/* 返回等待回复的标识PSYNC_WAIT_REPLY，调用者会将read_reply设置为1，然后再次调用该函数，执行下面的读部分。 */
	return(PSYNC_WAIT_REPLY);
}
```
由于从节点是第一次和主节点进行同步操作，因此从节点缓存的主节点client状态`erver.cached_master`为空，所以就会发送一个`PSYNC ? -1`命令给主节点，表示进行一次全量同步。

**主节点**会接收到`PSYNC ? -1`命令，然后调用`replication.c`文件中实现的`syncCommand()`函数处理`PSYNC`命令。

`syncCommand()`函数先会判断执行的是`PSYNC`还是`SYNC`命令，如果是`PSYNC`命令会调用`masterTryPartialResynchronization()`命令执行部分同步，但是由于这是第一次执行复制操作，所以会执行失败。进而执行全量同步。

`syncCommand()`函数的代码如下：
```
/* SYNC and PSYNC command implemenation. */
/* SYNC and PSYNC 命令实现 */
void syncCommand( client *c )
{
	... ... .... /* 为了简洁，删除一些判断条件的代码 */

	/*
	 * 尝试执行一个部分同步PSYNC的命令，则masterTryPartialResynchronization()会回复一个 "+FULLRESYNC <runid> <offset>",如果失败则执行全量同步
	 * 所以，从节点会如果和主节点连接断开，从节点会知道runid和offset，随后会尝试执行PSYNC
	 * 如果是执行PSYNC命令
	 */
	if ( !strcasecmp( c->argv[0]->ptr, "psync" ) )
	{
		/* 主节点尝试执行部分重同步，执行成功返回C_OK */
		if ( masterTryPartialResynchronization( c ) == C_OK )
		{
			/* 可以执行PSYNC命令，则将接受PSYNC命令的个数加1 */
			server.stat_sync_partial_ok++;
			/* 不需要执行后面的全量同步，直接返回 */
			return; /* No full resync needed, return. */
			/* 不能执行PSYNC部分重同步，需要进行全量同步 */
		} else {
			char *master_runid = c->argv[1]->ptr;
			/* 从节点以强制全量同步为目的，所以不能执行部分重同步，因此增加PSYNC命令失败的次数 */
			if ( master_runid[0] != '?' )
				server.stat_sync_partial_err++;
		}
		/* 执行SYNC命令 */
	} else {
		/* 设置标识，执行SYNC命令，不接受REPLCONF ACK */
		c->flags |= CLIENT_PRE_PSYNC;
	}
	/* 全量重同步次数加1 */
	server.stat_sync_full++;

	/* 设置client状态为：从服务器节点等待BGSAVE节点的开始 */
	c->replstate = SLAVE_STATE_WAIT_BGSAVE_START;
	/* 执行SYNC命令后是否关闭TCP_NODELAY */
	if ( server.repl_disable_tcp_nodelay )
		/* 是的话，则启用nagle算法 */
		anetDisableTcpNoDelay( NULL, c->fd );  /* Non critical if it fails. */
	/* 保存主服务器传来的RDB文件的fd，设置为-1 */
	c->repldbfd = -1;
	/* 设置client状态为从节点，标识client是一个从服务器 */
	c->flags |= CLIENT_SLAVE;
	/* 添加到服务器从节点链表中 */
	listAddNodeTail( server.slaves, c );

	/* CASE 1: BGSAVE is in progress, with disk target. */
	/* 情况1. 正在执行 BGSAVE ，且是同步到磁盘上 */
	if ( server.rdb_child_pid != -1 &&
	     server.rdb_child_type == RDB_CHILD_TYPE_DISK )
	{
		client		*slave;
		listNode	*ln;
		listIter	li;

		listRewind( server.slaves, &li );
		/* 遍历从节点链表 */
		while ( (ln = listNext( &li ) ) )
		{
			slave = ln->value;
			/*
			 * 如果有从节点已经创建子进程执行写RDB操作，等待完成，那么退出循环
			 * 从节点的状态为 SLAVE_STATE_WAIT_BGSAVE_END 在情况三中被设置
			 */
			if ( slave->replstate == SLAVE_STATE_WAIT_BGSAVE_END )
				break;
		}
		/* 对于这个从节点，我们检查它是否具有触发当前BGSAVE操作的能力 */
		if ( ln && ( (c->slave_capa & slave->slave_capa) == slave->slave_capa) )
		{
			/* 将slave的输出缓冲区所有内容拷贝给c的所有输出缓冲区中 */
			copyClientOutputBuffer( c, slave );
			/* 设置全量重同步从节点的状态，设置部分重同步的偏移量 */
			replicationSetupSlaveForFullResync( c, slave->psync_initial_offset );
			serverLog( LL_NOTICE, "Waiting for end of BGSAVE for SYNC" );
		} else {
			serverLog( LL_NOTICE, "Can't attach the slave to the current BGSAVE. Waiting for next BGSAVE for SYNC" );
		}

		/* CASE 2: BGSAVE is in progress, with socket target. */
		/* 情况2. 正在执行BGSAVE，且是无盘同步，直接写到socket中 */
	} else if ( server.rdb_child_pid != -1 &&
		    server.rdb_child_type == RDB_CHILD_TYPE_SOCKET )
	{
		/* 虽然有子进程在执行写RDB，但是它直接写到socket中，所以等待下次执行BGSAVE */
		serverLog( LL_NOTICE, "Current BGSAVE has socket target. Waiting for next BGSAVE for SYNC" );

		/* CASE 3: There is no BGSAVE is progress. */
		/* 情况3：没有执行BGSAVE的进程 */
	} else {
		/* 服务器支持无盘同步 */
		if ( server.repl_diskless_sync && (c->slave_capa & SLAVE_CAPA_EOF) )
		{
			/* 无盘同步复制的子进程被创建在replicationCron()中，因为想等待更多的从节点可以到来而延迟 */
			if ( server.repl_diskless_sync_delay )
				serverLog( LL_NOTICE, "Delay next BGSAVE for diskless SYNC" );
			/* 服务器不支持无盘复制 */
		} else {
			/* 如果没有正在执行BGSAVE，且没有进行写AOF文件，则开始为复制执行BGSAVE，并且是将RDB文件写到磁盘上 */
			if ( server.aof_child_pid == -1 )
			{
				startBgsaveForReplication( c->slave_capa );
			} else {
				serverLog( LL_NOTICE,
					   "No BGSAVE in progress, but an AOF rewrite is active. BGSAVE for replication delayed" );
			}
		}
	}

	/* 只有一个从节点，且backlog为空，则创建一个新的backlog */
	if ( listLength( server.slaves ) == 1 && server.repl_backlog == NULL )
		createReplicationBacklog();
	return;
}
```
首先先明确，主节点执行处理从节点发来`PSYNC`命令的操作。所以**主节点会将从节点视为自己的从节点客户端来操作**。会将从节点的复制设置为`SLAVE_STATE_WAIT_BGSAVE_START`状态表示

主节点执行全量同步的情况有三种：

1.  主节点服务器正在执行`BGSAVE`命令，且将RDB文件写到磁盘上。  
    -   这种情况，如果有已经设置过全局重同步偏移量的从节点，可以共用输出缓冲区的数据。
2.  主节点服务器正在执行`BGSAVE`命令，且将RDB文件写到网络socket上，无盘同步。  
    -   由于本次`BGSAVE`命令直接将RDB写到socket中，因此只能等待下一`BGSAVE`。
3.  主节点服务器没有正在执行`BGSAVE`。  
    -   如果也没有进行AOF持久化的操作，那么开始为复制操作执行`BGSAVE`，生成一个写到磁盘上的RDB文件。

**我们针对第三种情况来分析。**调用了`startBgsaveForReplication()`来开始执行`BGSAVE`命令。我们贴出主要的代码：
```
/*
 * 开始为复制执行BGSAVE，根据配置选择磁盘或套接字作为RDB发送的目标，在开始之前确保冲洗脚本缓存
 * mincapa参数是SLAVE_CAPA_*按位与的结果
 */
int startBgsaveForReplication( int mincapa )
{
	int retval;
	/* 是否直接写到socket */
	int		socket_target = server.repl_diskless_sync && (mincapa & SLAVE_CAPA_EOF);
	listIter	li;
	listNode	*ln;

	if ( socket_target )
		/*
		 * 直接写到socket中
		 * fork一个子进程将rdb写到 状态为等待BGSAVE开始 的从节点的socket中
		 */
		retval = rdbSaveToSlavesSockets();
	else
		/* 否则后台进行RDB持久化BGSAVE操作，保存到磁盘上 */
		retval = rdbSaveBackground( server.rdb_filename );

	... ...

	/*
	 * 如果是直接写到socket中，rdbSaveToSlavesSockets()已经会设置从节点为全量复制
	 * 否则直接写到磁盘上，执行以下代码
	 */
	if ( !socket_target )
	{
		listRewind( server.slaves, &li );
		/* 遍历从节点链表 */
		while ( (ln = listNext( &li ) ) )
		{
			client *slave = ln->value;
			/* 设置等待全量同步的从节点的状态 */
			if ( slave->replstate == SLAVE_STATE_WAIT_BGSAVE_START )
			{
				/* 设置要执行全量重同步从节点的状态 */
				replicationSetupSlaveForFullResync( slave,
								    getPsyncInitialOffset() );
			}
		}
	}
}
```
该函数主要干了两件事：

-   调用`rdbSaveBackground()`函数为复制操作生成一个RDB文件，我们分析的情况，该文件是被保存在磁盘上。
-   调用`replicationSetupSlaveForFullResync()`函数，将等待开始的从节点设置为全量同步的状态，并且发送给从节点`+FULLRESYNC`命令，还发送了主节点的运行ID`server.runid`和主节点的全局复制偏移量`server.master_repl_offset`

`replicationSetupSlaveForFullResync()`函数源码如下：
```
int replicationSetupSlaveForFullResync( client *slave, long long offset )
{
	char	buf[128];
	int	buflen;

	/* 设置全量重同步的偏移量 */
	slave->psync_initial_offset = offset;
	/* 设置从节点复制状态，开始累计差异数据 */
	slave->replstate = SLAVE_STATE_WAIT_BGSAVE_END;
	/* 将slaveseldb设置为-1，是为了强制发送一个select命令在复制流中 */
	server.slaveseldb = -1;

	/*
	 * 如果从节点的状态是CLIENT_PRE_PSYNC，则表示是Redis是2.8之前的版本，则不将这些信息发送给从节点。
	 * 因为在2.8之前只支持SYNC的全量复制同步，而在之后的版本提供了部分的重同步
	 */
	if ( !(slave->flags & CLIENT_PRE_PSYNC) )
	{
		buflen = snprintf( buf, sizeof(buf), "+FULLRESYNC %s %lld\r\n",
				   server.runid, offset );
		/* 否则会将全量复制的信息写给从节点 */
		if ( write( slave->fd, buf, buflen ) != buflen )
		{
			freeClientAsync( slave );
			return(C_ERR);
		}
	}
	return(C_OK);
}
```
哇，主节点终于回复从节点的`PSYNC`命令了，回复了一个`+FULLRESYNC`，写到主从同步的`fd`。表示要进行全量同步啊！！！

此时，从节点的复制状态一定为`REPL_STATE_RECEIVE_PSYNC`，`fd`的读事件发生，调用`syncWithMaster()`函数进行处理。

处理这种情况的代码如下：
```
// 那么尝试进行第二次部分重同步，从主节点读取指令来决定执行部分重同步还是全量同步 psync_result = slaveTryPartialResynchronization(fd,1);
```
这次的第二个参数是`1`，因此会执行该函数的读部分。(因为这个函数有两个部分，上一次执行了写部分，因为第二个参数是`0`)
```
/* Reading half */
/* 从主节点读一个命令保存在reply中 */
reply = sendSynchronousCommand( SYNC_CMD_READ, fd, NULL );
if ( sdslen( reply ) == 0 )
{
	/* 主节点为了保持连接的状态，可能会在接收到PSYNC命令后发送一个空行 */
	sdsfree( reply );
	/* 所以就返回PSYNC_WAIT_REPLY，调用者会将read_reply设置为1，然后再次调用该函数。 */
	return(PSYNC_WAIT_REPLY);
}
/* 如果读到了一个命令，删除fd的可读事件 */
aeDeleteFileEvent( server.el, fd, AE_READABLE );

/* 接受到的是"+FULLRESYNC"，表示进行一次全量同步 */
if ( !strncmp( reply, "+FULLRESYNC", 11 ) )
{
	char *runid = NULL, *offset = NULL;
	/* 解析回复中的内容，将runid和复制偏移量提取出来 */
	runid = strchr( reply, ' ' );
	if ( runid )
	{
		runid++;                /* 定位到runid的地址 */
		offset = strchr( runid, ' ' );
		if ( offset )
			offset++;       /* 定位offset */
	}
	/* 如果runid和offset任意为空，那么发生不期望错误 */
	if ( !runid || !offset || (offset - runid - 1) != CONFIG_RUN_ID_SIZE )
	{
		serverLog( LL_WARNING, "Master replied with wrong +FULLRESYNC syntax." );
		/* 将主节点的运行ID重置为0 */
		memset( server.repl_master_runid, 0, CONFIG_RUN_ID_SIZE + 1 );
		/* runid和offset获取成功 */
	} else {
		/* 设置服务器保存的主节点的运行ID */
		memcpy( server.repl_master_runid, runid, offset - runid - 1 );
		server.repl_master_runid[CONFIG_RUN_ID_SIZE] = '\0';
		/* 主节点的偏移量 */
		server.repl_master_initial_offset = strtoll( offset, NULL, 10 );
		serverLog( LL_NOTICE, "Full resync from master: %s:%lld", server.repl_master_runid, server.repl_master_initial_offset );
	}
	/* 执行全量同步，所以缓存的主节点结构没用了，将其清空 */
	replicationDiscardCachedMaster();
	sdsfree( reply );
	/* 返回执行的状态 */
	return(PSYNC_FULLRESYNC);
}

/* 接受到的是"+CONTINUE"，表示进行一次部分重同步 */
if ( !strncmp( reply, "+CONTINUE", 9 ) )
{
	serverLog( LL_NOTICE, "Successful partial resynchronization with master." );
	sdsfree( reply );
	/* 因为执行部分重同步，因此要使用缓存的主节点结构，所以将其设置为当前的主节点，被同步的主节点 */
	replicationResurrectCachedMaster( fd );
	/* 返回执行的状态 */
	return(PSYNC_CONTINUE);
}

/*
 * 接收到了错误，两种情况。
 * 1. 主节点不支持PSYNC命令，Redis版本低于2.8
 * 2. 从主节点读取了一个不期望的回复
 */
if ( strncmp( reply, "-ERR", 4 ) )
{
	/* If it's not an error, log the unexpected event. */
	serverLog( LL_WARNING, "Unexpected reply to PSYNC from master: %s", reply );
} else {
	serverLog( LL_NOTICE, "Master does not support PSYNC or is in error state (reply: %s)", reply );
}
sdsfree( reply );
replicationDiscardCachedMaster();
/* 发送不支持PSYNC命令的状态 */
return(PSYNC_NOT_SUPPORTED);
```
至此，从节点监听主节点的读命令事件已经完成，所以取消监听了读事件。等到主节点开始传送数据给从节点时，从节点会新创建读事件。

该函数可以解析出主节点发过来的命令是哪一个，一共有三种：

1.  “+FULLRESYNC”：代表要进行一次全量复制。
2.  “+CONTINUE”：代表要进行一次部分重同步。
3.  “-ERR”：发生了错误。有两种可能：Redis版本过低不支持`PSYNC`命令和从节点读到一个错误回复。

我们关注第一个全量同步的操作。如果读到了主节点发来的`"+FULLRESYNC"`，那么会将同时发来的**主节点运行ID和全局的复制偏移量**保存到从节点的服务器属性中`server.repl_master_runid`和`server.repl_master_initial_offset`。然后返回`PSYNC_FULLRESYNC`。

回到`syncWithMaster`函数，**继续**处理全量同步。由于要进行全量同步，如果当前从节点还作为其他节点的主节点，因此要断开所有从节点的连接，让他们也重新同步当前节点。
```
/*
 * 执行到这里，psync_result == PSYNC_FULLRESYNC或PSYNC_NOT_SUPPORTED
 * 准备一个合适临时文件用来写入和保存主节点传来的RDB文件数据
 */
while ( maxtries-- )
{
	/* 设置文件的名字 */
	snprintf( tmpfile, 256,
		  "temp-%d.%ld.rdb", (int) server.unixtime, (long int) getpid() );
	/* 以读写，可执行权限打开临时文件 */
	dfd = open( tmpfile, O_CREAT | O_WRONLY | O_EXCL, 0644 );
	/* 打开成功，跳出循环 */
	if ( dfd != -1 )
		break;
	sleep( 1 );
}
/* Setup the non blocking download of the bulk file. */
/* 监听一个fd的读事件，并设置该事件的处理程序为readSyncBulkPayload */
if ( aeCreateFileEvent( server.el, fd, AE_READABLE, readSyncBulkPayload, NULL )
     == AE_ERR )
{
	serverLog( LL_WARNING,
		   "Can't create readable event for SYNC: %s (fd=%d)",
		   strerror( errno ), fd );
	goto error;
}

/* 复制状态为正从主节点接受RDB文件 */
server.repl_state = REPL_STATE_TRANSFER;
/* 初始化RDB文件的大小 */
server.repl_transfer_size = -1;
/* 已读的大小 */
server.repl_transfer_read = 0;
/* 最近一个执行fsync的偏移量为0 */
server.repl_transfer_last_fsync_off = 0;
/* 传输RDB文件的临时fd */
server.repl_transfer_fd = dfd;
/* 最近一次读到RDB文件内容的时间 */
server.repl_transfer_lastio = server.unixtime;
/* 保存RDB文件的临时文件名 */
server.repl_transfer_tmpfile = zstrdup( tmpfile );
return;
```
准备好了所有，接下来就要等待主节点来发送RDB文件了。因此上面做了这三件事：

-   打开一个临时文件，用来保存主节点发来的RDB文件数据的。
-   监听`fd`的读事件，等待主节点发送RDB文件数据，触发可读事件执行`readSyncBulkPayload()`函数，该**函数就会把主节点发来的数据读到一个缓冲区中，然后将缓冲区的数据写到刚才打开的临时文件中，接着要载入到从节点的数据库中，最后同步到磁盘中。**
-   设置复制操作的状态为`server.repl_state = REPL_STATE_TRANSFER`。并且初始化复制的信息，例如：RDB文件的大小，偏移量，等等。（具体看上面的代码）

主节点要发送RDB文件，但是回复完”+FULLRESYNC”就再也没有操作了。而子节点创建了监听主节点写RDB文件的事件，等待主节点来写，才调用`readSyncBulkPayload()`函数来处理。这又有问题了，**到底主节点什么时候发送RDB文件呢？如果不是主动执行，那么一定就在周期性函数内被执行。**

它的调用关系如下：

`serverCron()`->`backgroundSaveDoneHandler()`->`backgroundSaveDoneHandlerDisk()`->`updateSlavesWaitingBgsave()`

`updateSlavesWaitingBgsave()`函数定义在`replication.c`中，主要操作有两步，我们简单介绍：

-   只读打开主节点的临时RDB文件，然后设置从节点client复制状态为`SLAVE_STATE_SEND_BULK`。
-   立刻创建监听可写的事件，并设置`sendBulkToSlave()`函数为可写事件的处理程序。

当主节点执行周期性函数时，主节点会先清除之前监听的可写事件，然后立即监听新的可写事件，这样就会触发可写的事件，调用`sendBulkToSlave()`函数将RDB文件写入到`fd`中，触发从节点的读事件，从节点调用`readSyncBulkPayload()`函数，来将RDB文件的数据载入数据库中，至此，就保证了主从同步了。

我们来简单介绍`sendBulkToSlave()`函数在写RDB文件时做了什么：

-   将RDB文件的大小写给从节点，以协议格式的字符串表示的大小。
-   从RDB文件的`repldbfd`中读出RDB文件数据，然后写到主从同步的`fd`中。
-   **写入完成后**，又一次取消监听文件可写事件，等待下一次发送缓冲区数据时在监听触发，并且调用`putSlaveOnline()`函数将从节点client的复制状态设置为`SLAVE_STATE_ONLINE`。表示已经发送RDB文件完毕，发送缓存更新。

### 2.9 发送输出缓冲区数据

主节点发送完RDB文件后，调用`putSlaveOnline()`函数将从节点client的复制状态设置为`SLAVE_STATE_ONLINE`，表示已经发送RDB文件完毕，要发送缓存更新了。于是会新创建一个事件，监听写事件的发生，设置`sendReplyToClient`为可写的处理程序，而且会将从节点client当做私有数据闯入`sendReplyToClient()`当做发送缓冲区的对象。
```
aeCreateFileEvent( server.el, slave->fd, AE_WRITABLE, sendReplyToClient, slave )
```
创建可写事件的时候，就会触发第一次可写，执行`sendReplyToClient()`，该函数还直接调用了`riteToClient(fd,privdata,1)`函数，于是将从节点client输出缓冲区的数据发送给了从节点服务器。

`riteToClient()`函数数据Redis网络连接库的函数，定义在`network.c`中，具体分析请看：[Redis 网络连接库源码分析](http://blog.csdn.net/men_wen/article/details/72084617)

这样就保证主从服务器的数据库状态一致了。

### 2.10 命令传播

主从节点在第一次全量同步之后就达到了一致，但是之后主节点如果执行了**写命令**，主节点的数据库状态就又可能发生变化，导致主从再次不一致。为了让主从节点回到一致状态，主机的执行命令后都需要将命令**传播**到从节点。

传播时会调用`server.c`中的`propagate()`函数，如果传播到从节点会调用`replicationFeedSlaves(server.slaves,dbid,argv,argc)`函数，**该函数则会将执行的命令以协议的传输格式写到从节点client的输出缓冲区中，这就是为什么主节点会将从节点client的输出缓冲区发送到从节点（具体见标题2.9），也会添加到`server.repl_backlog`中。**

我们来看看`replicationFeedSlaves()`函数的实现：
```
/* 将参数列表中的参数发送给从服务器 */
void replicationFeedSlaves( list *slaves, int dictid, robj **argv, int argc )
{
	listNode	*ln;
	listIter	li;
	int		j, len;
	char		llstr[LONG_STR_SIZE];

	/* 如果没有backlog且没有从节点服务器，直接返回 */
	if ( server.repl_backlog == NULL && listLength( slaves ) == 0 )
		return;

	/* We can't have slaves attached and no backlog. */
	serverAssert( !(listLength( slaves ) != 0 && server.repl_backlog == NULL) );

	/* 如果当前从节点使用的数据库不是目标的数据库，则要生成一个select命令 */
	if ( server.slaveseldb != dictid )
	{
		robj *selectcmd;

		/* 0 <= id < 10 ，可以使用共享的select命令对象 */
		if ( dictid >= 0 && dictid < PROTO_SHARED_SELECT_CMDS )
		{
			selectcmd = shared.select[dictid];
			/* 否则自行按照协议格式构建select命令对象 */
		} else {
			int dictid_len;

			dictid_len	= ll2string( llstr, sizeof(llstr), dictid );
			selectcmd	= createObject( OBJ_STRING,
							sdscatprintf( sdsempty(),
								      "*2\r\n$6\r\nSELECT\r\n$%d\r\n%s\r\n",
								      dictid_len, llstr ) );
		}
		/* 将select 命令添加到backlog中 */
		if ( server.repl_backlog )
			feedReplicationBacklogWithObject( selectcmd );

		/* 发送给从服务器 */
		listRewind( slaves, &li );
		/* 遍历所有的从服务器节点 */
		while ( (ln = listNext( &li ) ) )
		{
			client *slave = ln->value;
			/* 从节点服务器状态为等待BGSAVE的开始，因此跳过回复，遍历下一个节点 */
			if ( slave->replstate == SLAVE_STATE_WAIT_BGSAVE_START )
				continue;
			/* 添加select命令到当前从节点的回复中 */
			addReply( slave, selectcmd );
		}
		/* 释放临时对象 */
		if ( dictid < 0 || dictid >= PROTO_SHARED_SELECT_CMDS )
			decrRefCount( selectcmd );
	}
	/* 设置当前从节点使用的数据库ID */
	server.slaveseldb = dictid;

	/* 将命令写到backlog中 */
	if ( server.repl_backlog )
	{
		char aux[LONG_STR_SIZE + 3];

		/*
		 * 将参数个数构建成协议标准的字符串
		 * *<argc>\r\n
		 */
		aux[0]		= '*';
		len		= ll2string( aux + 1, sizeof(aux) - 1, argc );
		aux[len + 1]	= '\r';
		aux[len + 2]	= '\n';
		/* 添加到backlog中 */
		feedReplicationBacklog( aux, len + 3 );

		/* 遍历所有的参数 */
		for ( j = 0; j < argc; j++ )
		{
			/* 返回参数对象的长度 */
			long objlen = stringObjectLen( argv[j] );

			/*
			 * 构建成协议标准的字符串，并添加到backlog中
			 * $<len>\r\n<argv>\r\n
			 */
			aux[0]		= '$';
			len		= ll2string( aux + 1, sizeof(aux) - 1, objlen );
			aux[len + 1]	= '\r';
			aux[len + 2]	= '\n';
			/* 添加$<len>\r\n */
			feedReplicationBacklog( aux, len + 3 );
			/* 添加参数对象<argv> */
			feedReplicationBacklogWithObject( argv[j] );
			/* 添加\r\n */
			feedReplicationBacklog( aux + len + 1, 2 );
		}
	}
	/* 将命令写到每一个从节点中 */
	listRewind( server.slaves, &li );
	/* 遍历从节点链表 */
	while ( (ln = listNext( &li ) ) )
	{
		client *slave = ln->value;

		/* 从节点服务器状态为等待BGSAVE的开始，因此跳过回复，遍历下一个节点 */
		if ( slave->replstate == SLAVE_STATE_WAIT_BGSAVE_START )
			continue;

		/* 将命令写给正在等待初次SYNC的从节点（所以这些命令在输出缓冲区中排队，直到初始SYNC完成），或已经与主节点同步 */
		/* Add the multi bulk length. */
		/* 添加回复的长度 */
		addReplyMultiBulkLen( slave, argc );

		/* 将所有的参数列表添加到从节点的输出缓冲区 */
		for ( j = 0; j < argc; j++ )
			addReplyBulk( slave, argv[j] );
	}
}
```
和`AOF`持久化一样，再给从节点client写命令时，会将`SELECT`命令强制写入，以保证命令正确读到数据库中。

不仅写入了从节点client的输出缓冲区，而且还会将命令记录到主节点服务器的复制积压缓冲区`server.repl_backlog`中，这是为了网络闪断后进行部分重同步。

## 3. 部分重同步实现

> `replication.c`文件详细注释：[Redis 复制代码注释](https://github.com/menwengit/redis_source_annotation)

刚才剖析完全量同步，但是没有考虑特殊的情况。如果在传输RDB文件的过程中，网络发生故障，主节点和从节点的连接中断，Redis会咋么做呢？

Redis 2.8 版本之前会在进行一次连接然后进行全量复制，但是这样效率非常地下，之后的版本都提供了部分重同步的实现。那么我们就分析一下部分重同步的实现过程。

**部分重同步在复制的过程中，相当于`标题2.8`的发送`PSYNC`命令的部分，其他所有的部分都要进行，他只是主节点回复从节点的命令不同，回复`+CONTINUE`则执行部分重同步，回复`+FULLRESYNC`则执行全量同步。**

### 3.1 心跳机制

主节点是如何发现和从节点连接中断？在主从节点建立连接后，他们之间都维护者**长连接**并彼此发送心跳命令。主从节点彼此都有心跳机制，各自模拟成对方的客户端进行通信。

-   主节点默认**每隔10秒**发送`PING`命令，判断从节点的连接状态。  
    -   文件配置项：`repl-ping-salve-period`，默认是10
```
/*
 * 首先，根据当前节点发送PING命令给从节点的频率发送PING命令
 * 如果当前节点是某以节点的 主节点 ，那么发送PING给从节点
 */
if ( (replication_cron_loops % server.repl_ping_slave_period) == 0 )
{
	/* 创建PING命令对象 */
	ping_argv[0] = createStringObject( "PING", 4 );
	/* 将PING发送给从服务器 */
	replicationFeedSlaves( server.slaves, server.slaveseldb, ping_argv, 1 );
	decrRefCount( ping_argv[0] );
}
```
-   从节点在主线程中**每隔1秒**发送`REPLCONF ACK <offset>`命令，给主节点报告自己当前复制偏移量。
```
/* 定期发送ack给主节点，旧版本的Redis除外 */
if ( server.masterhost && server.master && !(server.master->flags & CLIENT_PRE_PSYNC) )
	/* 发送一个REPLCONF ACK命令给主节点去报告关于当前处理的offset。 */
	replicationSendAck();

```
在周期性函数`replicationCron()`，每次都要检查和主节点处于连接状态的从节点和主节点的交互时间是否超时，如果超时则会调用`cancelReplicationHandshake()`函数，取消和主节点的连接。等到下一个周期在和主节点重新建立连接，进行复制。

### 3.2 复制积压缓冲区(backlog)

复制积压缓冲区是一个大小为`1M`的循环队列。主节点在命令传播时，不仅会将命令发送给所有的从节点，还会将命令写入复制积压缓冲区中（具体请看标题2.10）。

也就是说，**复制积压缓冲区最多可以备份`1M`大小的数据，如果主从节点断线时间过长，复制积压缓冲区的数据会被新数据覆盖，那么当从主从中断连接起，主节点接收到的数据超过`1M`大小，那么从节点就无法进行部分重同步，只能进行全量复制。**

在标题2.8，介绍的`syncCommand()`命令中，调用`masterTryPartialResynchronization()`函数会进行尝试部分重同步，在我们之前分析的第一次全量同步时，该函数会执行失败，然后返回`syncCommand()`函数执行全量同步，而在进行恢复主从连接后，则会进行部分重同步，`masterTryPartialResynchronization()`函数代码如下:
```
/*
 * 该函数从主节点接收到部分重新同步请求的角度处理PSYNC命令
 * 成功返回C_OK，否则返回C_ERR
 */
int masterTryPartialResynchronization( client *c )
{
	long long	psync_offset, psync_len;
	char		*master_runid = c->argv[1]->ptr; /* 主节点的运行ID */
	char		buf[128];
	int		buflen;

	/*
	 * 主节点的运行ID是否和从节点执行PSYNC的参数提供的运行ID相同。
	 * 如果运行ID发生了改变，则主节点是一个不同的实例，那么就不能进行继续执行原有的复制进程
	 */
	if ( strcasecmp( master_runid, server.runid ) )
	{
		/* Run id "?" is used by slaves that want to force a full resync. */
		/* 如果从节点的运行ID是"?"，表示想要强制进行一个全量同步 */
		if ( master_runid[0] != '?' )
		{
			serverLog( LL_NOTICE, "Partial resynchronization not accepted: "
				   "Runid mismatch (Client asked for runid '%s', my runid is '%s')",
				   master_runid, server.runid );
		} else {
			serverLog( LL_NOTICE, "Full resync requested by slave %s",
				   replicationGetSlaveName( c ) );
		}
		goto need_full_resync;
	}

	/* 从参数对象中获取psync_offset */
	if ( getLongLongFromObjectOrReply( c, c->argv[2], &psync_offset, NULL ) !=
	     C_OK )
		goto need_full_resync;
	/*
	 * 如果psync_offset小于repl_backlog_off，说明backlog所备份的数据的已经太新了，有一些数据被覆盖，则需要进行全量复制
	 * 如果psync_offset大于(server.repl_backlog_off + server.repl_backlog_histlen)，表示当前backlog的数据不够全，则需要进行全量复制
	 */
	if ( !server.repl_backlog ||
	     psync_offset < server.repl_backlog_off ||
	     psync_offset > (server.repl_backlog_off + server.repl_backlog_histlen) )
	{
		serverLog( LL_NOTICE,
			   "Unable to partial resync with slave %s for lack of backlog (Slave request was: %lld).", replicationGetSlaveName( c ), psync_offset );
		if ( psync_offset > server.master_repl_offset )
		{
			serverLog( LL_WARNING,
				   "Warning: slave %s tried to PSYNC with an offset that is greater than the master replication offset.", replicationGetSlaveName( c ) );
		}
		goto need_full_resync;
	}

	/*
	 * 执行到这里，则可以进行部分重同步
	 * 1. 设置client状态为从节点
	 * 2. 向从节点发送 +CONTINUE 表示接受 partial resync 被接受
	 * 3. 发送backlog的数据给从节点
	 */

	/* 设置client状态为从节点 */
	c->flags |= CLIENT_SLAVE;
	/* 设置复制状态为在线，此时RDB文件传输完成，发送差异数据 */
	c->replstate = SLAVE_STATE_ONLINE;
	/* 设置从节点收到ack的时间 */
	c->repl_ack_time = server.unixtime;
	/* slave向master发送ack标志设置为0 */
	c->repl_put_online_on_ack = 0;
	/* 将当前client加入到从节点链表中 */
	listAddNodeTail( server.slaves, c );
	/* 向从节点发送 +CONTINUE */
	buflen = snprintf( buf, sizeof(buf), "+CONTINUE\r\n" );
	if ( write( c->fd, buf, buflen ) != buflen )
	{
		freeClientAsync( c );
		return(C_OK);
	}
	/* 将backlog的数据发送从节点 */
	psync_len = addReplyReplicationBacklog( c, psync_offset );
	serverLog( LL_NOTICE,
		   "Partial resynchronization request from %s accepted. Sending %lld bytes of backlog starting from offset %lld.", replicationGetSlaveName( c ), psync_len, psync_offset );
	/* 计算延迟值小于min-slaves-max-lag的从节点的个数 */
	refreshGoodSlavesCount();
	return(C_OK); /* The caller can return, no full resync needed. */

need_full_resync:
	return(C_ERR);
}
```
如果可以进行部分重同步，主节点则会发送`"+CONTINUE\r\n"`作为从节点发送`PSYNC`回复（看标题2.8）。然后调用`addReplyReplicationBacklog()`函数，将`backlog`中的数据发送给从节点。于是就完成了部分重同步。

`addReplyReplicationBacklog()`函数所做的就是将`backlog`写到从节点的client的输出缓冲区中。
