
# Redis源码剖析和注释（十五）---- 通知功能实现与实战 (notify)
# Redis 通知功能实现与实战

## 1. 通知功能介绍

客户端可以通过  **订阅与发布功能（pub/sub）**功能，来接收那些以某种方式改动了Redis数据集的事件。

目前Redis的订阅与发布功能采用的是**发送即忘（fire and forget）的策略**，当订阅事件的客户端断线时，它会丢失所有在断线期间分发给它的事件。

## 2. 通知的类型

通知功能的类型分别为：

-   键空间通知（key-space notification）
-   键事件通知（key-event notification）
```
/* notify.c */
#define NOTIFY_KEYSPACE (1 << 0) /* K */        /* 键空间通知 */
#define NOTIFY_KEYEVENT (1 << 1) /* E */        /* 键事件通知 */
```
这两种通知的格式如下：
```
__keyspace@<db>__ : < key > < event > notifications.    /* 键空间通知格式 */
__keyevente@<db>__ : < event > < key > notifications .  /* 键事件通知格式 */
```
构建这两种通知格式的源码如下：
```
/*
 * event 是一个字符串类型的事件名
 * key 是一个对象代表一个键名
 * dbid 是数据库id
 */
void notifyKeyspaceEvent( int type, char *event, robj *key, int dbid )
{
	sds	chan;
	robj	*chanobj, *eventobj;
	int	len = -1;
	char	buf[24];

	/* If notifications for this class of events are off, return ASAP. */
	/*
	 * 如果notify_keyspace_events中配置了不发送type类型的通知，则直接返回
	 * notify_keyspace_events值为 一个type的亦或值，type保存有不发送的通知
	 */
	if ( !(server.notify_keyspace_events & type) )
		return;

	/* 创建一个事件通知对象 */
	eventobj = createStringObject( event, strlen( event ) );

	/* __keyspace@<db>__:<key> <event> notifications. */
	/* 发送 键空间 通知 */
	if ( server.notify_keyspace_events & NOTIFY_KEYSPACE )
	{
		/* 构建一个频道对象，格式如上 */
		chan	= sdsnewlen( "__keyspace@", 11 );
		len	= ll2string( buf, sizeof(buf), dbid );
		chan	= sdscatlen( chan, buf, len );
		chan	= sdscatlen( chan, "__:", 3 );
		chan	= sdscatsds( chan, key->ptr );
		chanobj = createObject( OBJ_STRING, chan );
		/* 通过publish命令发送频道对象chanobj和事件对象eventobj通知 */
		pubsubPublishMessage( chanobj, eventobj );
		decrRefCount( chanobj ); /* 释放对象 */
	}

	/* __keyevente@<db>__:<event> <key> notifications. */
	/* 发送 键事件 通知 */
	if ( server.notify_keyspace_events & NOTIFY_KEYEVENT )
	{
		/* 构建一个频道对象，格式如上 */
		chan = sdsnewlen( "__keyevent@", 11 );
		if ( len == -1 )
			len = ll2string( buf, sizeof(buf), dbid );
		chan	= sdscatlen( chan, buf, len );
		chan	= sdscatlen( chan, "__:", 3 );
		chan	= sdscatsds( chan, eventobj->ptr );
		chanobj = createObject( OBJ_STRING, chan );
		/* 通过publish命令发送频道对象chanobj和键key通知 */
		pubsubPublishMessage( chanobj, key );
		decrRefCount( chanobj );        /* 释放对象 */
	}
	decrRefCount( eventobj );               /* 释放事件对象 */
}
```
## 3. 通知功能的配置

通知功能的配置默认是关闭状态，因为大多数使用者不需要这个功能，因为开启该功能会有一些额外的开销。开启通知功能的两种方式

-   修改配置文件redis.conf中的`notify-keyspace-events`
-   CONFIG SET notify-keyspace-events 字符

`notify-keyspace-events`参数可以是一下字符的任意组合，它指定了服务器该发送哪种类型的通知：
|字符|发送的通知|
|--|--|
|K|键空间通知，所有通知以 `__keyspace@<db>__` 为前缀|
|--|--|
|E|键事件通知，所有通知以 `__keyevent@<db>__` 为前缀|
|--|--|
|g|`DEL` 、 `EXPIRE` 、 `RENAME` 等类型无关的通用命令的通知|
|--|--|
|$|字符串命令的通知|
|--|--|
|l|列表命令的通知|
|--|--|
|s|集合命令的通知|
|--|--|
|h|哈希命令的通知|
|--|--|
|z|有序集合命令的通知|
|--|--|
|x|过期事件：每当有过期键被删除时发送|
|--|--|
|e|驱逐(evict)事件：每当有键因为 `maxmemory` 政策而被删除时发送|
|--|--|
|A|参数 `g$lshzxe` 的别名，包含所有的字符|
输入的参数中至少要有一个  `K`  或者  `E`  ，否则的话，不管其余的参数是什么，都不会有任何通知被分发。例如，如果设置`KEA`那么表示发送所有类型的通知。

这些字符的源码定义：
```
/* 键空间通知的类型，每个类型都关联着一个有目的的字符 */
#define NOTIFY_KEYSPACE (1 << 0) /* K */                                                                                                                        /* 键空间 */
#define NOTIFY_KEYEVENT (1 << 1) /* E */                                                                                                                        /* 键事件 */
#define NOTIFY_GENERIC	(1 << 2) /* g */                                                                                                                        /* 通用无类型通知 */
#define NOTIFY_STRING	(1 << 3) /* $ */                                                                                                                        /* 字符串类型键通知 */
#define NOTIFY_LIST	(1 << 4) /* l */                                                                                                                        /* 列表键通知 */
#define NOTIFY_SET	(1 << 5) /* s */                                                                                                                        /* 集合键通知 */
#define NOTIFY_HASH	(1 << 6) /* h */                                                                                                                        /* 哈希键通知 */
#define NOTIFY_ZSET	(1 << 7) /* z */                                                                                                                        /*有序集合键通知 */
#define NOTIFY_EXPIRED	(1 << 8) /* x */                                                                                                                        /* 过期有关的键通知 */
#define NOTIFY_EVICTED	(1 << 9) /* e */                                                                                                                        /* 驱逐有关的键通知 */
#define NOTIFY_ALL	(NOTIFY_GENERIC | NOTIFY_STRING | NOTIFY_LIST | NOTIFY_SET | NOTIFY_HASH | NOTIFY_ZSET | NOTIFY_EXPIRED | NOTIFY_EVICTED) /* A */       /* 所有键通知 */
```
这些字符通常使用一个`int`类型的`flags`参数通过多个字符按位或运算保存起来。因此就涉及到`flags`和字符串的相互转换

-   字符串 to flags
```
/* 对传入的字符串参数进行分析，返回一个flags，flags保存字符串每个字符所映射的键空间事件类型 */
int keyspaceEventsStringToFlags( char *classes )
{
	char	*p = classes;
	int	c, flags = 0;

	while ( (c = *p++) != '\0' )
	{
		switch ( c )
		{
		case 'A': flags |= NOTIFY_ALL; break;
		case 'g': flags |= NOTIFY_GENERIC; break;
		case '$': flags |= NOTIFY_STRING; break;
		case 'l': flags |= NOTIFY_LIST; break;
		case 's': flags |= NOTIFY_SET; break;
		case 'h': flags |= NOTIFY_HASH; break;
		case 'z': flags |= NOTIFY_ZSET; break;
		case 'x': flags |= NOTIFY_EXPIRED; break;
		case 'e': flags |= NOTIFY_EVICTED; break;
		case 'K': flags |= NOTIFY_KEYSPACE; break;
		case 'E': flags |= NOTIFY_KEYEVENT; break;
		default: return(-1);
		}
	}
	return(flags);
}
```
-   flags to 字符串
```
/* 根据flags返回一个字符串，字符串中的字符就是设置flags的字符 */
sds keyspaceEventsFlagsToString( int flags )
{
	sds res;

	res = sdsempty();
	if ( (flags & NOTIFY_ALL) == NOTIFY_ALL )
	{
		res = sdscatlen( res, "A", 1 );
	} else {
		if ( flags & NOTIFY_GENERIC )
			res = sdscatlen( res, "g", 1 );
		if ( flags & NOTIFY_STRING )
			res = sdscatlen( res, "$", 1 );
		if ( flags & NOTIFY_LIST )
			res = sdscatlen( res, "l", 1 );
		if ( flags & NOTIFY_SET )
			res = sdscatlen( res, "s", 1 );
		if ( flags & NOTIFY_HASH )
			res = sdscatlen( res, "h", 1 );
		if ( flags & NOTIFY_ZSET )
			res = sdscatlen( res, "z", 1 );
		if ( flags & NOTIFY_EXPIRED )
			res = sdscatlen( res, "x", 1 );
		if ( flags & NOTIFY_EVICTED )
			res = sdscatlen( res, "e", 1 );
	}
	if ( flags & NOTIFY_KEYSPACE )
		res = sdscatlen( res, "K", 1 );
	if ( flags & NOTIFY_KEYEVENT )
		res = sdscatlen( res, "E", 1 );
	return(res);
}
```
## 4. 通知功能实战

[发布订阅功能的命令键使用方法](http://blog.csdn.net/men_wen/article/details/62237970)

-   客户端1
```
127.0 .0 .1 : 6379 > PSUBSCRIBE __key *     /* 执行接收所有类型的通知 */
Reading messages ... (press Ctrl - C to quit)
1) "psubscribe"
2) "__key*"
3)( integer ) 1
/* 此时客户端阻塞等待消息的发布 */
```
-   客户端2
```
127.0 .0 .1 : 6379 > CONFIG SET notify - keyspace - events KEA /* 设置接受所有通知，会改变server.notify_keyspace_events值 */
OK
127.0 .0 .1 : 6379 > set number 888
OK
127.0 .0 .1 : 6379 > DEL number 888
(integer) 1
127.0 .0 .1 : 6379 > HSET hash field1 value1
	( integer ) 1
127.0 .0 .1 : 6379 > EXPIRE hash 5           /* 设置生存时间为5秒 */
	(integer) 1
```
-   客户端1
```
127.0 .0 .1 : 6379 > PSUBSCRIBE __key * /* 执行接收所有类型的通知 */
Reading messages ... (press Ctrl - C to quit)
1) "psubscribe"
2) "__key*"
3)( integer ) 1
/* 此时客户端阻塞等待消息的发布，当客户端2发布消息时，会打印一下信息 */
1) "pmessage"                           /* set number 888 命令所生成的通知 */
2) "__key*"
3) "__keyspace@0__:number"
4) "set"
1) "pmessage"
2) "__key*"
3) "__keyevent@0__:set"
4) "number"

1) "pmessage"                   /* DEL number 888 命令所生成的通知 */
2) "__key*"
3) "__keyspace@0__:number"
4) "del"
1) "pmessage"
2) "__key*"
3) "__keyevent@0__:del"
4) "number"

1) "pmessage"                   /* HSET hash field1 value1 命令所生成的通知 */
2) "__key*"
3) "__keyspace@0__:hash"
4) "hset"
1) "pmessage"
2) "__key*"
3) "__keyevent@0__:hset"
4) "hash"

1) "pmessage"                   /* EXPIRE hash 5 命令所生成的通知 */
2) "__key*"
3) "__keyspace@0__:hash"
4) "expire"
1) "pmessage"
2) "__key*"
3) "__keyevent@0__:expire"
4) "hash"

1) "pmessage"                   /* 当hash键的5秒的生存时间到时后，自动发送 expired 通知 */
2) "__key*"
3) "__keyspace@0__:hash"
4) "expired"
1) "pmessage"
2) "__key*"
3) "__keyevent@0__:expired"
4) "hash"
```

