
# Redis源码剖析和注释（二十三）--- Redis Sentinel实现(哨兵的执行过程和执行的内容)
# Redis Sentinel实现（上）

## 1. Redis Sentinel 介绍和部署

请参考[Redis Sentinel 介绍与部署](http://blog.csdn.net/men_wen/article/details/72724406)

`sentinel.c`文件详细注释：[Redis Sentinel详细注释](https://github.com/menwengit/redis_source_annotation)

本文会分为两篇分别接受`Redis Sentinel`的实现，本篇主要将`Redis`哨兵的执行过程和执行的内容。

-   [Redis Sentinel实现上](https://blog.csdn.net/men_wen/article/details/72805850#redis-sentinel%E5%AE%9E%E7%8E%B0%E4%B8%8A)
    -   [Redis Sentinel 介绍和部署](https://blog.csdn.net/men_wen/article/details/72805850#1-redis-sentinel-%E4%BB%8B%E7%BB%8D%E5%92%8C%E9%83%A8%E7%BD%B2)
    -   [Redis Sentinel 的执行过程和初始化](https://blog.csdn.net/men_wen/article/details/72805850#2-redis-sentinel-%E7%9A%84%E6%89%A7%E8%A1%8C%E8%BF%87%E7%A8%8B%E5%92%8C%E5%88%9D%E5%A7%8B%E5%8C%96)
        -   [1 检查是否开启哨兵模式](https://blog.csdn.net/men_wen/article/details/72805850#21-%E6%A3%80%E6%9F%A5%E6%98%AF%E5%90%A6%E5%BC%80%E5%90%AF%E5%93%A8%E5%85%B5%E6%A8%A1%E5%BC%8F)
        -   [2 初始化哨兵的配置](https://blog.csdn.net/men_wen/article/details/72805850#22-%E5%88%9D%E5%A7%8B%E5%8C%96%E5%93%A8%E5%85%B5%E7%9A%84%E9%85%8D%E7%BD%AE)
        -   [3 载入配置文件](https://blog.csdn.net/men_wen/article/details/72805850#23-%E8%BD%BD%E5%85%A5%E9%85%8D%E7%BD%AE%E6%96%87%E4%BB%B6)
            -   [31 创建实例](https://blog.csdn.net/men_wen/article/details/72805850#231-%E5%88%9B%E5%BB%BA%E5%AE%9E%E4%BE%8B)
            -   [32 查找主节点](https://blog.csdn.net/men_wen/article/details/72805850#232-%E6%9F%A5%E6%89%BE%E4%B8%BB%E8%8A%82%E7%82%B9)
        -   [4 开启 Sentinel](https://blog.csdn.net/men_wen/article/details/72805850#24-%E5%BC%80%E5%90%AF-sentinel)
    -   [Redis Sentinel 的所有操作](https://blog.csdn.net/men_wen/article/details/72805850#3-redis-sentinel-%E7%9A%84%E6%89%80%E6%9C%89%E6%93%8D%E4%BD%9C)
        -   [1 TILT 模式判断](https://blog.csdn.net/men_wen/article/details/72805850#31-tilt-%E6%A8%A1%E5%BC%8F%E5%88%A4%E6%96%AD)
        -   [2 执行周期性任务](https://blog.csdn.net/men_wen/article/details/72805850#32-%E6%89%A7%E8%A1%8C%E5%91%A8%E6%9C%9F%E6%80%A7%E4%BB%BB%E5%8A%A1)
        -   [3 执行脚本任务](https://blog.csdn.net/men_wen/article/details/72805850#33-%E6%89%A7%E8%A1%8C%E8%84%9A%E6%9C%AC%E4%BB%BB%E5%8A%A1)
            -   [31 准备脚本](https://blog.csdn.net/men_wen/article/details/72805850#331-%E5%87%86%E5%A4%87%E8%84%9A%E6%9C%AC)
            -   [32 执行脚本](https://blog.csdn.net/men_wen/article/details/72805850#332-%E6%89%A7%E8%A1%8C%E8%84%9A%E6%9C%AC)
            -   [33 脚本清理工作](https://blog.csdn.net/men_wen/article/details/72805850#333-%E8%84%9A%E6%9C%AC%E6%B8%85%E7%90%86%E5%B7%A5%E4%BD%9C)
            -   [34 杀死超时脚本](https://blog.csdn.net/men_wen/article/details/72805850#334-%E6%9D%80%E6%AD%BB%E8%B6%85%E6%97%B6%E8%84%9A%E6%9C%AC)
        -   [4 脑裂](https://blog.csdn.net/men_wen/article/details/72805850#34-%E8%84%91%E8%A3%82)
    -   [哨兵的使命](https://blog.csdn.net/men_wen/article/details/72805850#4-%E5%93%A8%E5%85%B5%E7%9A%84%E4%BD%BF%E5%91%BD)

`标题4`将会在[Redis Sentinel实现（下）](http://blog.csdn.net/men_wen/article/details/72805897)中详细剖析。

## 2. Redis Sentinel 的执行过程和初始化

`Sentinel`本质上是一个运行在特殊模式下的`Redis`服务器，无论如何，都是执行服务器的`main`来启动。主函数中关于`Sentinel`启动的代码如下：
```
int main( int argc, char **argv )
{
	/* 1. 检查开启哨兵模式的两种方式 */
	server.sentinel_mode = checkForSentinelMode( argc, argv );
	/* 2. 如果已开启哨兵模式，初始化哨兵的配置 */
	if ( server.sentinel_mode )
	{
		initSentinelConfig();
		initSentinel();
	}
	/* 3. 载入配置文件 */
	loadServerConfig( configfile, options );
	/* 开启哨兵模式，哨兵模式和集群模式只能开启一种 */
	if ( !server.sentinel_mode )
	{
		/* 在不是哨兵模式下，会载入AOF文件和RDB文件，打印内存警告，集群模式载入数据等等操作。 */
	} else {
		sentinelIsRunning();
	}
}
```
以上过程可以分为四步：

-   检查是否开启哨兵模式
-   初始化哨兵的配置
-   载入配置文件
-   开启哨兵模式

### 2.1 检查是否开启哨兵模式

在[Redis Sentinel 介绍与部署](http://blog.csdn.net/men_wen/article/details/72724406)文章中，介绍了两种开启的方法：

-   `redis-sentinel sentinel.conf`
-   `redis-server sentinel.conf --sentinel`

主函数中调用了`checkForSentinelMode()`函数来判断是否开启哨兵模式。
```
int checkForSentinelMode( int argc, char **argv )
{
	int j;

	if ( strstr( argv[0], "redis-sentinel" ) != NULL )
		return(1);
	for ( j = 1; j < argc; j++ )
		if ( !strcmp( argv[j], "--sentinel" ) )
			return(1);
	return(0);
}
```
如果开启了哨兵模式，就会将`server.sentinel_mode`设置为`1`。

### 2.2 初始化哨兵的配置

在主函数中调用了两个函数`initSentinelConfig()`和`initSentinel()`，前者用来初始化`Sentinel`节点的默认配置，后者用来初始化`Sentinel`节点的状态。`sentinel.c`文件详细注释：[Redis Sentinel详细注释](https://github.com/menwengit/redis_source_annotation)

在`sentinel.c`文件中定义了一个全局变量`sentinel`，它是`struct sentinelState`类型的，用于保存当前`Sentinel`的状态。

-   `initSentinelConfig()`，初始化哨兵节点的默认端口为26379。
```
/* 设置Sentinel的默认端口，覆盖服务器的默认属性 */
void initSentinelConfig( void )
{
	server.port = REDIS_SENTINEL_PORT;
}
```
-   `initSentinel()`，初始化哨兵节点的状态
```
/* 执行Sentinel模式的初始化操作 */
void initSentinel( void )
{
	unsigned int j;


	/* Remove usual Redis commands from the command table, then just add
	 * the SENTINEL command. */
	/* 将服务器的命令表清空 */
	dictEmpty( server.commands, NULL );
	/* 只添加Sentinel模式的相关命令，Sentinel模式下一共11个命令 */
	for ( j = 0; j < sizeof(sentinelcmds) / sizeof(sentinelcmds[0]); j++ )
	{
		int			retval;
		struct redisCommand	*cmd = sentinelcmds + j;

		retval = dictAdd( server.commands, sdsnew( cmd->name ), cmd );
		serverAssert( retval == DICT_OK );
	}

	/* Initialize various data structures. */
	/* 初始化各种Sentinel状态的数据结构 */

	/* 当前纪元，用于实现故障转移操作 */
	sentinel.current_epoch = 0;
	/* 监控的主节点信息的字典 */
	sentinel.masters = dictCreate( &instancesDictType, NULL );
	/* TILT模式 */
	sentinel.tilt			= 0;
	sentinel.tilt_start_time	= 0;
	/* 最后执行时间处理程序的时间 */
	sentinel.previous_time = mstime();
	/* 正在执行的脚本数量 */
	sentinel.running_scripts = 0;
	/* 用户脚本的队列 */
	sentinel.scripts_queue = listCreate();
	/* Sentinel通过流言协议接收关于主服务器的ip和port */
	sentinel.announce_ip	= NULL;
	sentinel.announce_port	= 0;
	/* 故障模拟 */
	sentinel.simfailure_flags = SENTINEL_SIMFAILURE_NONE;
	/* Sentinel的ID置为0 */
	memset( sentinel.myid, 0, sizeof(sentinel.myid) );
}
```
在哨兵模式下，只有11条命令可以使用，因此要用哨兵模式的命令表来代替Redis原来的命令表。

之后就是初始化`sentinel`的成员变量。我们重点关注这几个成员：

-   **dict *masters**  ：当前哨兵节点监控的主节点字典。字典的键是主节点实例的名字，字典的值是一个指针，指向一个`sentinelRedisInstance`类型的结构。
-   **int running_scripts**： 当前正在执行的脚本的数量。
-   **list *scripts_queue**：保存要执行用户脚本的队列。

### 2.3 载入配置文件

在启动哨兵节点时，要指定一个`.conf`配置文件，配置文件可以将配置项分为两类。

> [Sentinel配置说明](http://blog.csdn.net/men_wen/article/details/72724406)

-   sentinel monitor \ \ \ \  
    -   例如：`sentinel monitor mymaster 127.0.0.1 6379 2`
    -   当前Sentinel节点监控 127.0.0.1:6379 这个主节点
    -   2 代表判断主节点失败至少需要2个Sentinel节点节点同意
    -   mymaster 是主节点的别名
-   sentinel xxxxxx \ xxxxxx  
    -   例如：`sentinel down-after-milliseconds mymaster 30000`
    -   每个Sentinel节点都要定期PING命令来判断Redis数据节点和其余Sentinel节点是否可达，如果超过30000毫秒且没有回复，则判定不可达。
    -   例如：`sentinel parallel-syncs mymaster 1`
    -   当Sentinel节点集合对主节点故障判定达成一致时，Sentinel领导者节点会做故障转移操作，选出新的主节点，原来的从节点会向新的主节点发起复制操作，限制每次向新的主节点发起复制操作的从节点个数为1。

配置文件以这样的格式告诉哨兵节点，监控的主节点是谁，有什么样的限制条件。如果想要监控多个主节点，只需按照此格式在配置文件中多写几份。

既然配置文件都是如此，那么处理的函数也是如此处理，由于配置项很多，但是大体相似，所以我们列举处理示例的代码块：
```
sentinelRedisInstance *ri;

/* SENTINEL monitor选项 */
if ( !strcasecmp( argv[0], "monitor" ) && argc == 5 )
{
	/* monitor <name> <host> <port> <quorum> */
	int quorum = atoi( argv[4] ); /* 获取投票数 */
	/* 投票数必须大于等于1 */
	if ( quorum <= 0 )
		return("Quorum must be 1 or greater.");
	/* 创建一个主节点实例，并加入到Sentinel所监控的master字典中 */
	if ( createSentinelRedisInstance( argv[1], SRI_MASTER, argv[2],
					  atoi( argv[3] ), quorum, NULL ) == NULL )
	{
		switch ( errno )
		{
		case EBUSY: return("Duplicated master name.");
		case ENOENT: return("Can't resolve master instance hostname.");
		case EINVAL: return("Invalid port number");
		}
	}

	/* sentinel down-after-milliseconds选项 */
} else if ( !strcasecmp( argv[0], "down-after-milliseconds" ) && argc == 3 )
{
	/* down-after-milliseconds <name> <milliseconds> */
	/* 获取根据name查找主节点实例 */
	ri = sentinelGetMasterByName( argv[1] );
	if ( !ri )
		return("No such master with specified name.");
	/* 设置主节点实例的主观下线的判断时间 */
	ri->down_after_period = atoi( argv[2] );
	if ( ri->down_after_period <= 0 )
		return("negative or zero time parameter.");
	/* 根据ri主节点的down_after_period字段的值设置所有连接该主节点的从节点和Sentinel实例的主观下线的判断时间 */
	sentinelPropagateDownAfterPeriod( ri );
```
载入配置文件主要使用了两个函数`createSentinelRedisInstance()`和`sentinelGetMasterByName()`。前者用来根据指定监控的主节点来创建实例，而后者则要根据名字找到对应的主节点实例来设置配置的参数。

#### 2.3.1 创建实例

调用`createSentinelRedisInstance()`函数创建被该哨兵节点所监控的主节点实例，然后将新创建的主节点实例保存到`sentinel.masters`字典中，也就是初始化时创建的字典。该函数是一个通用的函数，根据参数`flags`不同创建不同类型的实例，并且将实例保存到不同的字典中：

-   SRI_MASTER：创建一个主节点实例，保存到当前哨兵节点监控的主节点字典中。
-   SRI_SLAVE：创建一个从节点实例，保存到主节点实例的从节点字典中。
-   SRI_SENTINE：创建一个哨兵节点实例，保存到其他监控该主节点实例的哨兵节点的字典中。

我们先列出函数的原型：
```
sentinelRedisInstance *createSentinelRedisInstance(char *name, int flags, char *hostname, int port, int quorum, sentinelRedisInstance *master)
```
-   如果flags设置了`SRI_MASTER`，该实例被添加进`sentinel.masters`表中
-   如果flags设置了`SRI_SLAVE`  或者  `SRI_SENTINEL`，`master`**一定不为空**并且该实例被添加到`master->slaves`或`master->sentinels`中
-   如果该实例是从节点或者是哨兵节点，`name`参数被忽略，并且被自动设置为`hostname:port`

当根据`flags`能够获取实例的类型后，就会初始化一个`sentinelRedisInstance`类型的实例，添加到对应的字典中。
```
typedef struct sentinelRedisInstance {
	/* 标识值，记录了当前Redis实例的类型和状态 */
	int flags;                              /* See SRI_... defines */
	/*
	 * 实例的名字
	 * 主节点的名字由用户在配置文件中设置
	 * 从节点以及Sentinel节点的名字由Sentinel自动设置，格式为：ip:port
	 */
	char *name;                             /* Master name from the point of view of this sentinel. */
	/* 实例运行的独一无二ID */
	char *runid;                            /* Run ID of this instance, or unique ID if is a Sentinel.*/
	/* 配置纪元，用于实现故障转移 */
	uint64_t config_epoch;                  /* Configuration epoch. */
	/* 实例地址：ip和port */
	sentinelAddr *addr;                     /* Master host. */
	/* 实例的连接，有可能是被Sentinel共享的 */
	instanceLink *link;                     /* Link to the instance, may be shared for Sentinels. */
	/* 最近一次通过 Pub/Sub 发送信息的时间 */
	mstime_t last_pub_time;                 /* Last time we sent hello via Pub/Sub. */
	/*
	 * 只有被Sentinel实例使用
	 * 最近一次接收到从Sentinel发送来hello的时间
	 */
	mstime_t last_hello_time;
	/* 最近一次回复SENTINEL is-master-down的时间 */
	mstime_t last_master_down_reply_time;   /* Time of last reply to
	                                         * SENTINEL is-master-down command. */
	/* 实例被判断为主观下线的时间 */
	mstime_t s_down_since_time;             /* Subjectively down since time. */
	/* 实例被判断为客观下线的时间 */
	mstime_t o_down_since_time;             /* Objectively down since time. */
	/*
	 * 实例无响应多少毫秒之后被判断为主观下线
	 * 由SENTINEL down-after-millisenconds配置设定
	 */
	mstime_t down_after_period;             /* Consider it down after that period. */
	/* 从实例获取INFO命令回复的时间 */
	mstime_t info_refresh;                  /* Time at which we received INFO output from it. */

	/* 实例的角色 */
	int role_reported;
	/* 角色更新的时间 */
	mstime_t role_reported_time;
	/* 最近一次从节点的主节点地址变更的时间 */
	mstime_t slave_conf_change_time;        /* Last time slave master addr changed. */

	/* Master specific. */
	/*----------------------------------主节点特有的属性----------------------------------*/
	/* 其他监控相同主节点的Sentinel */
	dict *sentinels;                        /* Other sentinels monitoring the same master. */
	/*
	 * 如果当前实例是主节点，那么slaves保存着该主节点的所有从节点实例
	 * 键是从节点命令，值是从节点服务器对应的sentinelRedisInstance
	 */
	dict *slaves;                           /* Slaves for this master instance. */
	/*
	 * 判定该主节点客观下线的投票数
	 * 由SENTINEL monitor <master-name> <ip> <port> <quorum>配置
	 */
	unsigned int quorum;                    /* Number of sentinels that need to agree on failure. */
	/*
	 * 在故障转移时，可以同时对新的主节点进行同步的从节点数量
	 * 由sentinel parallel-syncs <master-name> <number>配置
	 */
	int parallel_syncs;                     /* How many slaves to reconfigure at same time. */
	/* 连接主节点和从节点的认证密码 */
	char *auth_pass;                        /* Password to use for AUTH against master & slaves. */

	/*----------------------------------从节点特有的属性----------------------------------*/
	/* 从节点复制操作断开时间 */
	mstime_t master_link_down_time;         /* Slave replication link down time. */
	/* 按照INFO命令输出的从节点优先级 */
	int slave_priority;                     /* Slave priority according to its INFO output. */
	/* 故障转移时，从节点发送SLAVEOF <new>命令的时间 */
	mstime_t slave_reconf_sent_time;        /* Time at which we sent SLAVE OF <new> */
	/* 如果当前实例是从节点，那么保存该从节点连接的主节点实例 */
	struct sentinelRedisInstance *master;   /* Master instance if it's slave. */
	/* INFO命令的回复中记录的主节点的IP */
	char *slave_master_host;                /* Master host as reported by INFO */
	/* INFO命令的回复中记录的主节点的port */
	int slave_master_port;                  /* Master port as reported by INFO */
	/* INFO命令的回复中记录的主从服务器连接的状态 */
	int slave_master_link_status;           /* Master link status as reported by INFO */
	/* 从节点复制偏移量 */
	unsigned long long slave_repl_offset;   /* Slave replication offset. */

	/*----------------------------------故障转移的属性----------------------------------*/
	/*
	 * 如果这是一个主节点实例，那么leader保存的是执行故障转移的Sentinel的runid
	 * 如果这是一个Sentinel实例，那么leader保存的是当前这个Sentinel实例选举出来的领头的runid
	 */
	char *leader;
	/* leader字段的纪元 */
	uint64_t leader_epoch;                  /* Epoch of the 'leader' field. */
	/* 当前执行故障转移的纪元 */
	uint64_t failover_epoch;                /* Epoch of the currently started failover. */
	/* 故障转移操作的状态 */
	int failover_state;                     /* See SENTINEL_FAILOVER_STATE_* defines. */
	/* 故障转移操作状态改变的时间 */
	mstime_t failover_state_change_time;
	/* 最近一次故障转移尝试开始的时间 */
	mstime_t failover_start_time;           /* Last failover attempt start time. */
	/* 更新故障转移状态的最大超时时间 */
	mstime_t failover_timeout;              /* Max time to refresh failover state. */
	/* 记录故障转移延迟的时间 */
	mstime_t failover_delay_logged;
	/* 晋升为新主节点的从节点实例 */
	struct sentinelRedisInstance *promoted_slave;
	/* 通知admin的可执行脚本的地址，如果设置为空，则没有执行的脚本 */
	char *notification_script;
	/* 通知配置的client的可执行脚本的地址，如果设置为空，则没有执行的脚本 */
	char *client_reconfig_script;
	/* 缓存INFO命令的输出 */
	sds info; /* cached INFO output */
} sentinelRedisInstance;
```
该实例用来抽象描述一个节点，可以是主节点、从节点或者是哨兵节点。

#### 2.3.2 查找主节点

在配置文件中分的那两个部分，第一部分是创建上面给出的结构实例，另一部分则是配置其中的一部分成员。因此，第一步要根据名字在**哨兵节点的主节点字典**中找到主节点实例。
```
sentinelRedisInstance *sentinelGetMasterByName( char *name )
{
	sentinelRedisInstance	*ri;
	sds			sdsname = sdsnew( name );
	/* 从Sentinel所监视的所有主节点中寻找名字为name的主节点，找到返回 */
	ri = dictFetchValue( sentinel.masters, sdsname );
	sdsfree( sdsname );
	return(ri);
}
```
当找到并返回主节点实例后，就可以配置其变量了。例如：`ri->down_after_period = atoi(argv[2])`

### 2.4 开启 Sentinel

载入完配置文件，就会调用`sentinelIsRunning()`函数开启`Sentinel`。该函数主要干了这几个事：

-   检查配置文件是否可写，因为要重写配置文件。
-   为没有`runid`的哨兵节点分配 ID，并重写到配置文件中，并且打印到日志中。
-   生成一个`+monitor`事件通知。

所以在启动一个哨兵节点时，查看日志会发现：
```
12775 : X 28 May 15 : 14 : 34.953 # Sentinel ID is a4dce0267abdb89f7422c9a42960e6cb6e4
d565a
12775 : X 28 May 15 : 14 : 34.953 # + monitor master mymaster 127.0 .0 .1 6379 quorum 2
```
至此，就正式启动了哨兵节点。我们用图片的方式来描述一下一个哨兵节点监控两个主节点的情况：
![avatar](data:image/jpeg;base64,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)
## 3. Redis Sentinel 的所有操作

`Redis`哨兵的操作，都是放在时间处理器中执行。服务器在初始化时会创建时间事件，并安装执行时间事件的处理函数`serverCron()`，在该函数调用`sentinelTimer()`函数（如下代码所示）来每`100ms`执行一次哨兵的定时中断，或者叫执行哨兵的任务。`sentinel.c`文件详细注释：[Redis Sentinel详细注释](https://github.com/menwengit/redis_source_annotation)
```
run_with_period( 100 )
{
	if ( server.sentinel_mode )
		sentinelTimer();
}
```
`sentinelTimer()`函数就是`Sentinel`的主函数，他的执行过程非常清晰，我们直接给出代码：
```
void sentinelTimer( void )
{
	/* 先检查Sentinel是否需要进入TITL模式，更新最近一次执行Sentinel模式的周期函数的时间 */
	sentinelCheckTiltCondition();
	/* 对Sentinel监控的所有主节点进行递归式的执行周期性操作 */
	sentinelHandleDictOfRedisInstances( sentinel.masters );
	/* 运行在队列中等待的脚本 */
	sentinelRunPendingScripts();
	/* 清理已成功执行的脚本，重试执行错误的脚本 */
	sentinelCollectTerminatedScripts();
	/* 杀死执行超时的脚本，等到下个周期在sentinelCollectTerminatedScripts()函数中重试执行 */
	sentinelKillTimedoutScripts();


	/* We continuously change the frequency of the Redis "timer interrupt"
	 * in order to desynchronize every Sentinel from every other.
	 * This non-determinism avoids that Sentinels started at the same time
	 * exactly continue to stay synchronized asking to be voted at the
	 * same time again and again (resulting in nobody likely winning the
	 * election because of split brain voting). */
	/* 我们不断改变Redis定期任务的执行频率，以便使每个Sentinel节点都不同步，这种不确定性可以避免Sentinel在同一时间开始完全继续保持同步，当被要求进行投票时，一次又一次在同一时间进行投票，因为脑裂导致有可能没有胜选者 */
	server.hz = CONFIG_DEFAULT_HZ + rand() % CONFIG_DEFAULT_HZ;
}
```
我们可以将哨兵的任务按顺序分为四部分：

-   TILT 模式判断
-   执行周期性任务。例如：定期发送PING、hello信息等等。
-   执行脚本任务
-   脑裂

接下来，依次分析

### 3.1 TILT 模式判断

TILT 模式是一种特殊的保护模式：当 Sentinel 发现系统有些不对劲时，Sentinel 就会进入 TILT 模式。

因为 Sentinel 的时间中断器默认每秒执行 10 次，所以我们预期时间中断器的两次执行之间的间隔为 100 毫秒左右。但是出现以下情况会出现异常：

-   Sentinel进程在某时被阻塞，有很多种原因，负载过大，IO任务密集，进程被信号停止等等。
-   系统时钟发送明显变化

Sentinel 的做法是（如下`sentinelCheckTiltCondition()`函数所示），记录上一次时间中断器执行时的时间，并将它和这一次时间中断器执行的时间进行对比：

-   如果两次调用时间之间的差距为负值，或者非常大（超过 2 秒钟），那么 Sentinel 进入 TILT 模式。
-   如果 Sentinel 已经进入 TILT 模式，那么 Sentinel 延迟退出 TILT 模式的时间。
```
void sentinelCheckTiltCondition( void )
{
	mstime_t now = mstime();
	/* 最后一次执行Sentinel时间处理程序的时间过去了过久 */
	mstime_t delta = now - sentinel.previous_time;
	/* 差为负数，或者大于2秒 */
	if ( delta < 0 || delta > SENTINEL_TILT_TRIGGER )
	{
		/* 设置Sentinel进入TILT状态 */
		sentinel.tilt = 1;
		/* 设置进入TILT状态的开始时间 */
		sentinel.tilt_start_time = mstime();
		sentinelEvent( LL_WARNING, "+tilt", NULL, "#tilt mode entered" );
	}
	/* 设置最近一次执行Sentinel时间处理程序的时间 */
	sentinel.previous_time = mstime();
}
```
当 Sentinel 进入 TILT 模式时，它仍然会继续监视所有目标，但是：

-   它不再执行任何操作，比如故障转移。
-   当有实例向这个 Sentinel 发送  `SENTINEL is-master-down-by-addr`  命令时，Sentinel 返回负值：因为这个 Sentinel 所进行的下线判断已经不再准确。

**如果 TILT 可以正常维持 30 秒钟，那么 Sentinel 退出 TILT 模式。**

### 3.2 执行周期性任务

我们先来看看在执行周期性任务的函数`sentinelHandleDictOfRedisInstances()`
```
void sentinelHandleDictOfRedisInstances( dict *instances )
{
	dictIterator		*di;
	dictEntry		*de;
	sentinelRedisInstance	*switch_to_promoted = NULL;

	/* There are a number of things we need to perform against every master. */
	di = dictGetIterator( instances );
	/* 遍历字典中所有的实例 */
	while ( (de = dictNext( di ) ) != NULL )
	{
		sentinelRedisInstance *ri = dictGetVal( de );
		/* 对指定的ri实例执行周期性操作 */
		sentinelHandleRedisInstance( ri );
		/* 如果ri实例是主节点 */
		if ( ri->flags & SRI_MASTER )
		{
			/* 递归的对主节点从属的从节点执行周期性操作 */
			sentinelHandleDictOfRedisInstances( ri->slaves );
			/* 递归的对监控主节点的Sentinel节点执行周期性操作 */
			sentinelHandleDictOfRedisInstances( ri->sentinels );
			/* 如果ri实例处于完成故障转移操作的状态，所有从节点已经完成对新主节点的同步 */
			if ( ri->failover_state == SENTINEL_FAILOVER_STATE_UPDATE_CONFIG )
			{
				/* 设置主从转换的标识 */
				switch_to_promoted = ri;
			}
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
}
```
该函数可以分为两部分：

-   递归的对当前哨兵所监控的所有主节点`sentinel.masters`，和所有主节点的所有从节点`ri->slaves`，和所有监控该主节点的其他所有哨兵节点`ri->sentinels`执行周期性操作。也就是`sentinelHandleRedisInstance()`函数。
-   在执行操作的过程中，可能发生主从切换的情况，因此要给所有原来主节点的从节点（除了被选为当做晋升的从节点）发送`slaveof`命令去复制新的主节点（晋升为主节点的从节点）。对应`sentinelFailoverSwitchToPromotedSlave()`函数。

由于这里的操作过多，因此先跳过，单独在`标题4`进行剖析。

### 3.3 执行脚本任务

在`Sentinel`的定时任务分为三步，也就是`sentinelTimer()`哨兵模式主函数中的三个函数：

-   `sentinelRunPendingScripts()`：运行在队列中等待的脚本。
-   `sentinelCollectTerminatedScripts()`：清理已成功执行的脚本，重试执行错误的脚本。
-   `sentinelKillTimedoutScripts()`：杀死执行超时的脚本，等到下个周期在sentinelCollectTerminatedScripts()函数中重试执行。

#### 3.3.1 准备脚本

我们先来说明脚本任务是如何加入到`sentinel.scripts_queue`中的。

首先在`Sentinel`中有两种脚本，分别是，都定义在`sentinelRedisInstance结构中`

-   通知admin的脚本。`char *notification_script`
-   重配置client的脚本。`char *client_reconfig_script`

在发生主从切换后，会调用`sentinelCallClientReconfScript()`函数，将**重配置client的脚本**放入脚本队列中。

在发生`LL_WARNING`级别的事件通知时，会调用`sentinelEvent()`函数，将**通知admin的脚本**放入脚本队列中。

然而这两个函数，都会调用最底层的`sentinelScheduleScriptExecution()`函数将脚本添加到脚本链表队列中。该函数源码如下：
```
#define SENTINEL_SCRIPT_MAX_ARGS 16
/* 将给定参数和脚本放入用户脚本队列中 */
void sentinelScheduleScriptExecution( char *path, ... )
{
	va_list			ap;
	char			*argv[SENTINEL_SCRIPT_MAX_ARGS + 1];
	int			argc = 1;
	sentinelScriptJob	*sj;

	va_start( ap, path );
	/* 将参数保存到argv中 */
	while ( argc < SENTINEL_SCRIPT_MAX_ARGS )
	{
		argv[argc] = va_arg( ap, char* );
		if ( !argv[argc] )
			break;
		argv[argc] = sdsnew( argv[argc] ); /* Copy the string. */
		argc++;
	}
	va_end( ap );
	/* 第一个参数是脚本的路径 */
	argv[0] = sdsnew( path );
	/* 分配脚本任务结构的空间 */
	sj		= zmalloc( sizeof(*sj) );
	sj->flags	= SENTINEL_SCRIPT_NONE;                         /* 脚本限制 */
	sj->retry_num	= 0;                                            /* 执行次数 */
	sj->argv	= zmalloc( sizeof(char*) * (argc + 1) );        /* 参数列表 */
	sj->start_time	= 0;                                            /* 开始时间 */
	sj->pid		= 0;                                            /* 执行脚本子进程的pid */
	/* 设置脚本的参数列表 */
	memcpy( sj->argv, argv, sizeof(char*) * (argc + 1) );
	/* 添加到脚本队列中 */
	listAddNodeTail( sentinel.scripts_queue, sj );

	/* Remove the oldest non running script if we already hit the limit. */
	/* 如果队列长度大于256个，那么删除最旧的脚本，只保留255个 */
	if ( listLength( sentinel.scripts_queue ) > SENTINEL_SCRIPT_MAX_QUEUE )
	{
		listNode	*ln;
		listIter	li;

		listRewind( sentinel.scripts_queue, &li );
		/* 遍历脚本链表队列 */
		while ( (ln = listNext( &li ) ) != NULL )
		{
			sj = ln->value;
			/* 跳过正在执行的脚本 */
			if ( sj->flags & SENTINEL_SCRIPT_RUNNING )
				continue;
			/* The first node is the oldest as we add on tail. */
			/* 删除最旧的脚本 */
			listDelNode( sentinel.scripts_queue, ln );
			/* 释放一个脚本任务结构和所有关联的数据 */
			sentinelReleaseScriptJob( sj );
			break;
		}
		serverAssert( listLength( sentinel.scripts_queue ) <=
			      SENTINEL_SCRIPT_MAX_QUEUE );
	}
}
```
`Redis`使用了`sentinelScriptJob`结构来管理脚本的一些信息，正如上述代码初始化那一部分。

而且当前哨兵维护的哨兵队列最多只能保留最新的255个脚本，如果脚本过多就会从队列中删除对旧的脚本。

#### 3.3.2 执行脚本

当要执行脚本放入了队列中，等到周期性函数`sentinelTimer()`时，就会执行。我们来执行脚本的函数`sentinelRunPendingScripts()`代码：
```
void sentinelRunPendingScripts( void )
{
	listNode	*ln;
	listIter	li;
	mstime_t	now = mstime();


	/* Find jobs that are not running and run them, from the top to the
	 * tail of the queue, so we run older jobs first. */
	listRewind( sentinel.scripts_queue, &li );
	/* 遍历脚本链表队列，如果没有超过同一时刻最多运行脚本的数量，找到没有正在运行的脚本 */
	while ( sentinel.running_scripts < SENTINEL_SCRIPT_MAX_RUNNING &&
		(ln = listNext( &li ) ) != NULL )
	{
		sentinelScriptJob	*sj = ln->value;
		pid_t			pid;

		/* Skip if already running. */
		/* 跳过正在运行的脚本 */
		if ( sj->flags & SENTINEL_SCRIPT_RUNNING )
			continue;

		/* Skip if it's a retry, but not enough time has elapsed. */
		/* 该脚本没有到达重新执行的时间，跳过 */
		if ( sj->start_time && sj->start_time > now )
			continue;

		/* 设置正在执行标志 */
		sj->flags |= SENTINEL_SCRIPT_RUNNING;
		/* 开始执行时间 */
		sj->start_time = mstime();
		/* 执行次数加1 */
		sj->retry_num++;
		/* 创建子进程执行 */
		pid = fork();

		/* fork()失败，报告错误 */
		if ( pid == -1 )
		{
			sentinelEvent( LL_WARNING, "-script-error", NULL,
				       "%s %d %d", sj->argv[0], 99, 0 );
			sj->flags	&= ~SENTINEL_SCRIPT_RUNNING;
			sj->pid		= 0;
			/* 子进程执行的代码 */
		} else if ( pid == 0 )
		{
			/* Child */
			/* 执行该脚本 */
			execve( sj->argv[0], sj->argv, environ );
			/* If we are here an error occurred. */
			/* 如果执行_exit(2)，表示发生了错误，不能重新执行 */
			_exit( 2 ); /* Don't retry execution. */
			/* 父进程，更新脚本的pid，和同时执行脚本的个数 */
		} else {
			sentinel.running_scripts++;
			sj->pid = pid;
			/* 并且通知事件 */
			sentinelEvent( LL_DEBUG, "+script-child", NULL, "%ld", (long) pid );
		}
	}
}
```
因为`Redis`是单线程架构的，所以和持久化一样，执行脚本需要创建一个子进程。

-   子进程：执行没有正在执行和已经到了执行时间的脚本任务。
-   父进程：更新脚本的信息。例如：正在执行的个数和执行脚本的子进程的`pid`等等。

父进程更新完脚本的信息后就会继续执行下一个`sentinelCollectTerminatedScripts()`函数

#### 3.3.3 脚本清理工作

-   如果在子进程执行的脚本已经执行完成，则可以从脚本队列中将其删除。
-   如果在子进程执行的脚本执行出错，但是可以在规定时间后重新执行，那么设置其执行的时间，下个周期重新执行。
-   如果在子进程执行的脚本执行出错，但是无法在执行，那么也会脚本队里中将其删除。

函数`sentinelCollectTerminatedScripts()`源码如下：
```
void sentinelCollectTerminatedScripts( void )
{
	int	statloc;
	pid_t	pid;

	/*
	 * 接受子进程退出码
	 * WNOHANG：如果没有子进程退出，则立刻返回
	 */
	while ( (pid = wait3( &statloc, WNOHANG, NULL ) ) > 0 )
	{
		int			exitcode	= WEXITSTATUS( statloc );
		int			bysignal	= 0;
		listNode		*ln;
		sentinelScriptJob	*sj;
		/* 获取造成脚本终止的信号 */
		if ( WIFSIGNALED( statloc ) )
			bysignal = WTERMSIG( statloc );
		sentinelEvent( LL_DEBUG, "-script-child", NULL, "%ld %d %d",
			       (long) pid, exitcode, bysignal );
		/* 根据pid查找并返回正在运行的脚本节点 */
		ln = sentinelGetScriptListNodeByPid( pid );
		if ( ln == NULL )
		{
			serverLog( LL_WARNING, "wait3() returned a pid (%ld) we can't find in our scripts execution queue!", (long) pid );
			continue;
		}
		sj = ln->value;

		/* 如果退出码是1并且没到脚本最大的重试数量 */
		if ( (bysignal || exitcode == 1) &&
		     sj->retry_num != SENTINEL_SCRIPT_MAX_RETRY )
		{       /* 取消正在执行的标志 */
			sj->flags	&= ~SENTINEL_SCRIPT_RUNNING;
			sj->pid		= 0;
			/* 设置下次执行脚本的时间 */
			sj->start_time = mstime() +
					 sentinelScriptRetryDelay( sj->retry_num );
			/* 脚本不能重新执行 */
		} else {
			/* 发送脚本错误的事件通知 */
			if ( bysignal || exitcode != 0 )
			{
				sentinelEvent( LL_WARNING, "-script-error", NULL,
					       "%s %d %d", sj->argv[0], bysignal, exitcode );
			}
			/* 从脚本队列中删除脚本 */
			listDelNode( sentinel.scripts_queue, ln );
			/* 释放一个脚本任务结构和所有关联的数据 */
			sentinelReleaseScriptJob( sj );
			/* 目前正在执行脚本的数量减1 */
			sentinel.running_scripts--;
		}
	}
}
```
#### 3.3.4 杀死超时脚本

`Sentinel`规定一个脚本最多执行`60s`，如果执行超时，则会杀死正在执行的脚本。
```
void sentinelKillTimedoutScripts( void )
{
	listNode	*ln;
	listIter	li;
	mstime_t	now = mstime();

	listRewind( sentinel.scripts_queue, &li );
	/* 遍历脚本队列 */
	while ( (ln = listNext( &li ) ) != NULL )
	{
		sentinelScriptJob *sj = ln->value;
		/* 如果当前脚本正在执行且执行，且脚本执行的时间超过60s */
		if ( sj->flags & SENTINEL_SCRIPT_RUNNING &&
		     (now - sj->start_time) > SENTINEL_SCRIPT_MAX_RUNTIME )
		{       /* 发送脚本超时的事件 */
			sentinelEvent( LL_WARNING, "-script-timeout", NULL, "%s %ld",
				       sj->argv[0], (long) sj->pid );
			/* 杀死执行脚本的子进程 */
			kill( sj->pid, SIGKILL );
		}
	}
}
```
### 3.4 脑裂

在`Redis`的[官方Sentinel文档](https://redis.io/topics/sentinel)中给出了一种关于脑裂的场景。
```
+---- +         +---- +
| M1 | ---------| R1 |
| S1 |         	| S2 |
+---- +         +---- +

Configuration: quorum = 1
/*
 * M1是主节点
 * R1是从节点
 * S1、S2是哨兵节点
 */
```
在此种情况中，如果主节点M1出现故障，那么R1将被晋升为主节点，因为两个`Sentinel`节点可以就配置的`quorum = 1`达成一致，并且会执行故障转移操作。如下图所示：
```
+----+           +------+
| M1 |----//-----| [M1] |
| S1 |           | S2   |
+----+           +------+
```
如果执行了故障转移之后，就会完全以对称的方式创建了两个主节点。客户端可能会不明确的写入数据到两个主节点，这就可能造成很多严重的后果，例如：争抢服务器的资源，争抢应用服务，数据损坏等等。

因此，最好不要进行这样的部署。

在哨兵模式的主函数`sentinelTimer()`，为了防止这样的部署造成的一些后果，所以每次执行后都会更改服务器的周期任务执行频率，如下所述：
```
server.hz = CONFIG_DEFAULT_HZ + rand() % CONFIG_DEFAULT_HZ;
```
不断改变`Redis`定期任务的执行频率，以便使每个Sentinel节点都不同步，这种不确定性可以避免Sentinel在同一时间开始完全继续保持同步，当被要求进行投票时，一次又一次在同一时间进行投票，因为脑裂导致有可能没有胜选者。

## 4. 哨兵的使命

`sentinel.c`文件详细注释：[Redis Sentinel详细注释](https://github.com/menwengit/redis_source_annotation)

该部分在[Redis Sentinel实现（下）](http://blog.csdn.net/men_wen/article/details/72805897)中单独剖析。

