# Redis 事务实现和乐观锁

## 1. 事务的介绍

`Redis`事务（transaction）提供了以下五个命令，用于用户操作事务功能，其分别是：

MULTI标记一个事务块的开始

DISCARD放弃执行事务

EXEC执行事务中的所有命令

WATCH监视一个或多个key，如果至少有一个key在EXEC之前被修改，则放弃执行事务

UNWATCH取消WATCH命令对所有键的监视

事务提供了一种将多个命令请求打包，然后**一次性、按顺序**地执行多个命令的机制，并且在事务的执行期间，服务器不会打断事务而改去执行其他客户端的命令请求，它会将事务中的所有命令都执行完毕，然后才去执行其他客户端的命令请求。

-  `Redis`中的**事务（transaction）是一组命令的集合**。事务同命令一样都是`Redis`中的**最小执行单位**，一个事务中的命令要么都执行，要么都不执行。

## 2. 事务的实现

执行事务的过程分为以下的几个阶段：

-   开始事务
-   命令入队
-   执行事务

### 2.1 开始事务

在客户端执行一个`MULTI`命令，标记一个事务块的开始。该命令会被封装成`Redis`协议的格式发送给服务器，服务器接收到该命令会调用`multiCommand()`函数来执行。函数源码如下：

```
void multiCommand( client *c )
{
	/* 客户端已经处于事务状态，回复错误后返回 */
	if ( c->flags & CLIENT_MULTI )
	{
		addReplyError( c, "MULTI calls can not be nested" );
		return;
	}
	/* 打开客户的的事务状态标识 */
	c->flags |= CLIENT_MULTI;
	/* 回复OK */
	addReply( c, shared.ok );
}
```

该函数首先先会判断当前客户端是否处于事务状态（CLIENT_MULTI），如果没有处于事务状态，那么会打开客户端的事务状态标识，并且回复客户端一个`OK`。

执行完`MULTI`命令，表示着一个事务的开始。

### 2.2 命令入队

由于事务的命令是一次性、按顺序的执行，因此，需要将客户端中的所有事务命令事前保存起来。

在每个描述客户端状态的结构`struct client`中，保存着有关事务状态的成员变量，他的定义如下：

```
typedef struct client {
	/* 事物状态 */
	multiState mstate;
} client;
```

`multiState`是一个结构体，定义如下：

```
typedef struct multiState {
	/* 事务命令队列数组 */
	multiCmd *commands;             /* Array of MULTI commands */
	/* 事务命令的个数 */
	int count;                      /* Total number of MULTI commands */
	/* 同步复制的标识 */
	int minreplicas;                /* MINREPLICAS for synchronous replication */
	/* 同步复制的超时时间 */
	time_t minreplicas_timeout;     /* MINREPLICAS timeout as unixtime. */
} multiState;
```

重点关注前两个属性。

-   `commands`成员是一个指针，保存的是一个成员类型为`multiCmd`的数组首地址，每一个成员保存的都是事务命令。

-   `count`成员保存的是以`commands`为地址的数组的成员个数。

对于每一个事务命令，都使用`multiCmd`类型来描述，定义如下：

```
/* 事务命令状态 */
typedef struct multiCmd {
	/* 命令的参数列表 */
	robj **argv;
	/* 命令的参数个数 */
	int argc;
	/* 命令函数指针 */
	struct redisCommand *cmd;
} multiCmd;
```

> 在客户端连接到服务器的时候，服务器会为客户端创建一个`client`，当客户端发送命令给服务器，会触发客户端和服务器网络连接的套接字产生读事件，然后会调用在创建`client`时所设置的回调函数`readQueryFromClient()`，来执行处理客户端发来的请求。该函数首先要读取被包装为协议的命令，然后调用`processInputBuffer()`函数将协议格式命令转换为参数列表的形式，最后会调用`processCommand()`函数执行命令。

由于之前执行了`MULTI`命令，因此客户端打开了`CLIENT_MULTI`标识，表示当前处于事务状态。因此在`processCommand()`函数中，最后执行命令的代码如下：

```
if ( c->flags & CLIENT_MULTI &&
     c->cmd->proc != execCommand && c->cmd->proc != discardCommand &&
     c->cmd->proc != multiCommand && c->cmd->proc != watchCommand )
{
	/* 除了上述的四个命令，其他的命令添加到事务队列中 */
	queueMultiCommand( c );
	addReply( c, shared.queued );
	/* 执行普通的命令 */
} else {
	/* 调用 call() 函数执行命令.... */
}
```

如果当前客户端处于事务状态，并且当前执行的命令不是`EXEC`、`DISCARD`、`MULTI`和`WATCH`命令，那么调用`queueMultiCommand()`函数将当前命令入队。该函数源码如下：

```
void queueMultiCommand( client *c )
{
	multiCmd	*mc;
	int		j;
	/* 增加队列的空间 */
	c->mstate.commands = zrealloc( c->mstate.commands,
				       sizeof(multiCmd) * (c->mstate.count + 1) );
	/* 获取新命令的存放地址 */
	mc = c->mstate.commands + c->mstate.count;
	/* 设置新命令的参数列表、参数数量和命令函数 */
	mc->cmd		= c->cmd;
	mc->argc	= c->argc;
	mc->argv	= zmalloc( sizeof(robj*) * c->argc );
	memcpy( mc->argv, c->argv, sizeof(robj*) * c->argc );
	/* 增加引用计数 */
	for ( j = 0; j < c->argc; j++ )
		incrRefCount( mc->argv[j] );
	/* 事务命令个数加1 */
	c->mstate.count++;
}
```

### 2.3 执行事务

当事务命令全部入队后，在客户端执行`EXEC`命令，就可以执行事务。服务器会调用`execCommand()`函数来执行`EXEC`命令。

```
void execCommand( client *c )
{
	int			j;
	robj			**orig_argv;
	int			orig_argc;
	struct redisCommand	*orig_cmd;
	/* 是否传播的标识 */
	int must_propagate = 0; /* Need to propagate MULTI/EXEC to AOF / slaves? */
	/* 如果客户端当前不处于事务状态，回复错误后返回 */
	if ( !(c->flags & CLIENT_MULTI) )
	{
		addReplyError( c, "EXEC without MULTI" );
		return;
	}
	/* 检查是否需要中断EXEC的执行因为： */


	/*
	 *  1. 被监控的key被修改
	 *  2. 入队命令时发生了错误
	 */
	/*
	 * 第一种情况返回空回复对象，第二种情况返回一个EXECABORT错误
	 * 如果客户的处于 1.命令入队时错误或者2.被监控的key被修改
	 */
	if ( c->flags & (CLIENT_DIRTY_CAS | CLIENT_DIRTY_EXEC) )
	{
		/* 回复错误信息 */
		addReply( c, c->flags & CLIENT_DIRTY_EXEC ? shared.execaborterr :
			  shared.nullmultibulk );
		/* 取消事务 */
		discardTransaction( c );
		/* 跳转到处理监控器代码 */
		goto handle_monitor;
	}

	/*
	 * 执行队列数组中的命令
	 * 因为所有的命令都是安全的，因此取消对客户端的所有的键的监视
	 */
	unwatchAllKeys( c ); /* Unwatch ASAP otherwise we'll waste CPU cycles */
	/* 备份EXEC命令 */
	orig_argv	= c->argv;
	orig_argc	= c->argc;
	orig_cmd	= c->cmd;
	/* 回复一个事务命令的个数 */
	addReplyMultiBulkLen( c, c->mstate.count );
	/* 遍历执行所有事务命令 */
	for ( j = 0; j < c->mstate.count; j++ )
	{
		/* 设置一个当前事务命令给客户端 */
		c->argc = c->mstate.commands[j].argc;
		c->argv = c->mstate.commands[j].argv;
		c->cmd	= c->mstate.commands[j].cmd;

		/* 当执行到第一个写命令时，传播事务状态 */
		if ( !must_propagate && !(c->cmd->flags & CMD_READONLY) )
		{
			/* 发送一个MULTI命令给所有的从节点和AOF文件 */
			execCommandPropagateMulti( c );
			/* 设置已经传播过的标识 */
			must_propagate = 1;
		}
		/* 执行该命令 */
		call( c, CMD_CALL_FULL );

		/* 命令可能会被修改，重新存储在事务命令队列中 */
		c->mstate.commands[j].argc	= c->argc;
		c->mstate.commands[j].argv	= c->argv;
		c->mstate.commands[j].cmd	= c->cmd;
	}
	/* 还原命令和参数 */
	c->argv = orig_argv;
	c->argc = orig_argc;
	c->cmd	= orig_cmd;
	/* 取消事务状态 */
	discardTransaction( c );
	/* 如果传播了EXEC命令，表示执行了写命令，更新数据库脏键数 */
	if ( must_propagate )
		server.dirty++;

handle_monitor:
	/* 如果服务器设置了监控器，并且服务器不处于载入文件的状态 */
	if ( listLength( server.monitors ) && !server.loading )
		/* 将参数列表中的参数发送给监控器 */
		replicationFeedMonitors( c, server.monitors, c->db->id, c->argv, c->argc );
}
```

如果当前客户端不处于事务状态会直接返回。

如果在按顺序执行事务命令的过程中，被`WATCH`命令监视的键发生修改，会被设置`CLIENT_DIRTY_CAS`状态；或者在执行`processCommand()`命令，该函数会在执行命令时会事先判断命令的合法性，如果不合法，会调用`flagTransaction()`函数来设置客户端`CLIENT_DIRTY_EXEC`状态。如果执行事务命令时，客户端处于以上两种状态，那么会调用`discardTransaction()`函数取消执行事务。

如果不处于以上两种状态，那么表示可以执行事务命令，但是先调用`unwatchAllKeys()`函数，解除当前客户端所监视的所有命令。

然后遍历事务队列中的所有命令，调用`call()`函数执行命令。如果事务状态中有写命令被执行，那么要将`MULTI`命令传播给从节点和`AOF`文件。并且会在最后更新数据库的脏键值。

执行完毕会调用`discardTransaction()`函数取消当前客户端的事务状态。

取消客户端的事务状态，即使释放客户端当前事务状态所有占用的资源，并且取消`CLIENT_MULTI`、  `CLIENT_DIRTY_CAS`、  `CLIENT_DIRTY_EXEC`三种状态。

## 3. WATCH命令的实现

`WATCH`命令可以监视一个(或多个) key ，如果在事务执行之前这个(或这些) key 被其他命令所改动，那么事务将被打断。

服务器调用`watchCommand()`函数执行`WATCH`命令。代码如下：

```
void watchCommand( client *c )
{
	int j;
	/* 如果已经处于事务状态，则回复错误后返回，必须在执行MULTI命令执行前执行WATCH */
	if ( c->flags & CLIENT_MULTI )
	{
		addReplyError( c, "WATCH inside MULTI is not allowed" );
		return;
	}
	/* 遍历所有的参数 */
	for ( j = 1; j < c->argc; j++ )
		/* 监控当前key */
		watchForKey( c, c->argv[j] );
	/* 回复OK */
	addReply( c, shared.ok );
}
```

如果执行`WATCH`命令时，函数处于事务状态，则直接返回。必须在执行`MULTI`命令执行前执行`WATCH`。

该函数会调用`watchForKey()`函数来监控所有指定的键。该函数代码如下：

```
void watchForKey( client *c, robj *key )
{
	list		*clients = NULL;
	listIter	li;
	listNode	*ln;
	watchedKey	*wk;

	listRewind( c->watched_keys, &li );
	/* 遍历客户端监视的键的链表，检查是否已经监视了指定的键 */
	while ( (ln = listNext( &li ) ) )
	{
		wk = listNodeValue( ln );
		/* 如果键已经被监视，则直接返回 */
		if ( wk->db == c->db && equalStringObjects( key, wk->key ) )
			return;  /* Key already watched */
	}
	/* 如果数据库中该键没有被client监视则添加它 */
	clients = dictFetchValue( c->db->watched_keys, key );
	/* 没有被client监视 */
	if ( !clients )
	{
		/* 创建一个空链表 */
		clients = listCreate();
		/* 值是被client监控的key，键是client，添加到数据库的watched_keys字典中 */
		dictAdd( c->db->watched_keys, key, clients );
		incrRefCount( key );
	}
	/* 将当前client添加到监视该key的client链表的尾部 */
	listAddNodeTail( clients, c );
	/* 将新的被监视的key和与该key关联的数据库加入到客户端的watched_keys中 */
	wk	= zmalloc( sizeof(*wk) );
	wk->key = key;
	wk->db	= c->db;
	incrRefCount( key );
	listAddNodeTail( c->watched_keys, wk );
}
```

首先，客户端会保存一个监视链表，定义为`list *watched_keys`。这个链表每一个节点保存一个`watchedKey`类型的指针，该结构代码如下：

```
typedef struct watchedKey {
	/* 被监视的key */
	robj *key;
	/* 被监视的key所在的数据库 */
	redisDb *db;
} watchedKey;
```

这个结构只要用来保存当前客户端监视的键和该键所在的数据库的映射关系。

因此，首先遍历客户端的`watched_keys`链表，如果当前键已经被监视，则直接返回。

其次，客户端当前操作的数据库中保存有一个监视字典，定义为`dict *watched_keys`，该字典的键是数据库中被监视的键，字典的值是所有监视该键的客户端链表。

因此，如果当前键没有被监视的话，就将当前客户端添加到该监视该键的客户端链表的尾部。

然后新建一个`watchedKey`结构，将当前被监视的键和该键所在的数据库关联起来，保存到客户端的`watched_keys`链表中。

这样就将一个键监视起来，当数据库中的键被修改，例如执行`DEL`、`RENAME`、等等命令时，会调用`signalModifiedKey()`函数来处理数据库中的键被修改的情况，该函数直接调用`touchWatchedKey()`函数，来处理是否该键处于被监视的状态。

```
void touchWatchedKey( redisDb *db, robj *key )
{
	list		*clients;
	listIter	li;
	listNode	*ln;
	/* 如果数据库中没有被监视的key，直接返回 */
	if ( dictSize( db->watched_keys ) == 0 )
		return;
	/* 找出监视该key的client链表 */
	clients = dictFetchValue( db->watched_keys, key );
	/* 没找到返回 */
	if ( !clients )
		return;

	listRewind( clients, &li );
	/* 遍历所有监视该key的client */
	while ( (ln = listNext( &li ) ) )
	{
		client *c = listNodeValue( ln );
		/* 设置CLIENT_DIRTY_CAS标识 */
		c->flags |= CLIENT_DIRTY_CAS;
	}
}
```

该函数首先判断数据库中的`watched_keys`字典是否为空，如果为空，表示该键没有被客户端监视，那么直接返回。

否则会从数据库中的`watched_keys`字典中找到监视该键的客户端链表，遍历所有的客户端，将所有的客户端设置为`CLIENT_DIRTY_CAS`状态标识。

该标识在`标题2.3`中，执行`EXEC`命令时，如果客户端设置了`CLIENT_DIRTY_CAS`标识，那么会取消事务状态，执行事务失败。

## 4. 乐观锁

乐观锁（又名**乐观并发控制**，Optimistic Concurrency Control，缩写“OCC”），是一种并发控制的方法。它假设多用户并发的事务在处理时不会彼此互相影响，各事务能够在不产生锁的情况下处理各自影响的那部分数据。在提交数据更新之前，每个事务会先检查在该事务读取数据后，有没有其他事务又修改了该数据。

与乐观所相对的，就是悲观锁（又名**悲观并发控制**，Pessimistic Concurrency Control，缩写“PCC”），它可以阻止一个事务以影响其他用户的方式来修改数据。如果一个事务执行的操作都某行数据应用了锁，那只有当这个事务把锁释放，其他事务才能够执行与该锁冲突的操作。

通俗的说，就是悲观锁就是“先取锁在访问”，因为悲观锁会“悲观”地认为访问会产生冲突，因此这种保守的策略虽然在数据处理的安全行上提供了保障，但是在效率方面会让数据库产生极大的开销，而且还有可能出现死锁的情况。

在`Redis`中`WATCH`命令的实现是基于乐观锁，即，假设访问不会产生冲突，但是在提交数据之前会先检查该事务该事物读取数据后，其他事务是否修改数据，如果其他事务修改了数据，像`MySQL`提供了回滚操作，而`Redis`不支持回滚，因为`antirez`认为这与`Redis`简单高效的设计主旨不相符，并且`Redis`事务执行时错误在开发环境时是可以避免的。

乐观锁控制的事务一般包括三个阶段：

-   读取：当执行完`MULTI`命令后，客户端进入事务模式，客户端接下来输入的命令会读入到事务队列中，入队过程中出错会设置`CLIENT_DIRTY_EXEC`标识。

-   校验：如果数据库有键被修改，那么会检测被修改的键是否是被`WATCH`命令监视的命令，如果是则会设置对应的标识（CLIENT_DIRTY_CAS），并且在命令执行前会检测这两个标识，如果检测到该标识，则会取消事务的执行。

-   写入：如果没有设置以上两种标识，那么会执行事务的命令，而`Redis`是单进程模型，因此可以避免执行事务命令时其他请求可能修改数据库键的可能。

`Redis`的乐观锁不是通常实现乐观锁的一般方法：检测版本号，而是在执行完一个写命令后，会进行检查，检查是否是被`WATCH`监视的键。
