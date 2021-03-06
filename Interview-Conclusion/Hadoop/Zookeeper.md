# ZooKeeper

## 1 CAP & BASE

### 1.1 CAP

CAP原则又称CAP定理，指的是在一个分布式系统中，

（1）Consistency（一致性）：在分布式系统中的所有数据备份，在同一时刻是否同样的值。（等同于所有节点访问同一份最新的数据副本）

（2）Availability（可用性）：在集群中一部分节点故障后，集群整体是否还能响应客户端的读写请求。（对数据更新具备高可用性）

（3）Partition tolerance（分区容错性）：以实际效果而言，分区相当于对通信的时限要求。系统如果不能在时限内达成数据一致性，就意味着发生了分区的情况，必须就当前操作在C和A之间做出选择。

### 1.2 BASE

ASE是Basically Available（基本可用）、Soft state（软状态）和Eventually consistent（最终一致性）三个短语的缩写。

1、基本可用:分布式系统在出现不可预知故障的时候，允许损失部分可用性

2、软状态:允许系统中的数据存在中间状态，并认为该中间状态的存在不会影响系统的整体可用性，即允许系统在不同节点的数据副本之间进行数据同步的过程存在延时

3、最终一致性:所有的数据副本，在经过一段时间的同步之后，最终都能够达到一个一致的状态。

## 2 一致性协议

### 2.1 2PC

1、阶段一：提交事务请求（投票阶段）

（1）事务询问:协调者向所有的参与者发送事务内容，询问是否可以执行事务提交操作，并开始等待各参与者的响应

（2）执行事务:各参与者节点执行事务操作，并将Undo和Redo信息计入事务日志中

（3）各参与者向协调者反馈事务询问的响应

如果参与者成功执行了事务操作，那么就反馈给协调者Yes响应，表示事务可以执行；如果参与者没有成功执行事务，那么就反馈给协调者No响应，表示事务不可以执行。

2、阶段二：执行事务提交（执行阶段）

（1）执行事务提交

如果所有参与者的反馈都是Yes响应，那么

A、发送提交请求

协调者向所有参与者节点发出Commit请求

B、事务提交

参与者接收到Commit请求后，会正式执行事务提交操作，并在完成提交之后释放在整个事务执行期间占用的事务资源

C、反馈事务提交结果

参与者在完成事务提交之后，向协调者发送ACK信息

D、完成事务

协调者接收到所有参与者反馈的ACK消息后，完成事务

（2）中断事务

任何一个参与者反馈了No响应，或者在等待超时之后，协调者尚无法接收到所有参与者的反馈响应，那么就会中断事务。

A、发送回滚请求

协调者向所有参与者节点发出Rollback请求

B、事务回滚

参与者接收到rollback请求后，会利用其在阶段一中记录的Undo信息来执行事务回滚操作，并在完成回滚之后释放整个事务执行期间占用的资源

C、反馈事务回滚结果

参与者在完成事务回滚之后，向协调者发送ACK信息

D、中断事务

协调者接收到所有参与者反馈的ACK信息后，完成事务中断

缺点

（1）同步阻塞

在二阶段提交的执行过程中，所有参与该事务操作的逻辑都处于阻塞状态，各个参与者在等待其他参与者响应的过程中，将无法进行其他任何操作。

（2）单点问题

一旦协调者出现问题，那么整个二阶段提交流程将无法运转，更为严重的是，如果是在阶段二中出现问题，那么其他参与者将会一直处于锁定事务资源的状态中，无法继续完成事务操作。

（3）数据不一致

在阶段二，当协调者向所有参与者发送commit请求之后，发生了局部网络异常或协调者在尚未发完commit请求之前自身发生了崩溃，导致最终只有部分参与者接收到了commit请求，于是这部分参与者执行事务提交，而没收到commit请求的参与者则无法进行事务提交，于是整个分布式系统出现了数据不一致性现象。

（4）太过保守

如果参与者在与协调者通信期间出现故障，协调者只能靠超时机制来判断是否需要中断事务，这个策略比较保守，需要更为完善的容错机制，任意一个节点的失败都会导致整个事务的失败。

### 2.2 3PC（解决2PC的阻塞，但还是可能造成数据不一致）

为了避免在通知所有参与者提交事务时，其中一个参与者crash不一致时，就出现了三阶段提交的方式。三阶段提交在两阶段提交的基础上增加了一个preCommit的过程，当所有参与者收到preCommit后，并不执行动作，直到收到commit或超过一定时间后才完成操作。

1、阶段一CanCommit

（1）事务询问

协调者向各参与者发送CanCommit的请求，询问是否可以执行事务提交操作，并开始等待各参与者的响应

（2）参与者向协调者反馈询问的响应

参与者收到CanCommit请求后，正常情况下，如果自身认为可以顺利执行事务，那么会反馈Yes响应，并进入预备状态，否则反馈No。

2、阶段二PreCommit

（1）执行事务预提交

如果协调者接收到各参与者反馈都是Yes，那么执行事务预提交

A、发送预提交请求

协调者向各参与者发送preCommit请求，并进入prepared阶段

B、事务预提交

参与者接收到preCommit请求后，会执行事务操作，并将Undo和Redo信息记录到事务日记中

C、各参与者向协调者反馈事务执行的响应

如果各参与者都成功执行了事务操作，那么反馈给协调者Ack响应，同时等待最终指令，提交commit或者终止abort

（2）中断事务

如果任何一个参与者向协调者反馈了No响应，或者在等待超时后，协调者无法接收到所有参与者的反馈，那么就会中断事务。

A、发送中断请求

协调者向所有参与者发送abort请求

B、中断事务

无论是收到来自协调者的abort请求，还是等待超时，参与者都中断事务

3、阶段三doCommit

（1）执行提交

A、发送提交请求

假设协调者正常工作，接收到了所有参与者的ack响应，那么它将从预提交阶段进入提交状态，并向所有参与者发送doCommit请求

B、事务提交

参与者收到doCommit请求后，正式提交事务，并在完成事务提交后释放占用的资源

C、反馈事务提交结果

参与者完成事务提交后，向协调者发送ACK信息

D、完成事务

协调者接收到所有参与者ack信息，完成事务

（2）中断事务

假设协调者正常工作，并且有任一参与者反馈No，或者在等待超时后无法接收所有参与者的反馈，都会中断事务

A、发送中断请求

协调者向所有参与者节点发送abort请求

B、事务回滚

参与者接收到abort请求后，利用undo日志执行事务回滚，并在完成事务回滚后释放占用的资源

C、反馈事务回滚结果

参与者在完成事务回滚之后，向协调者发送ack信息

D、中断事务

协调者接收到所有参与者反馈的ack信息后，中断事务。

阶段三可能出现的问题：

协调者出现问题、协调者与参与者之间网络出现故障。不论出现哪种情况，最终都会导致参与者无法及时接收到来自协调者的doCommit或是abort请求，针对这种情况，参与者都会在等待超时后，继续进行事务提交（timeout后中断事务）。

### 2.3 Paxos（解决单点问题）

这个算法有两个阶段（假设这个有三个结点：A，B，C）：

第一阶段：Prepare阶段

A把申请修改的请求Prepare Request发给所有的结点A，B，C。注意，Paxos算法会有一个Sequence Number（你可以认为是一个提案号，这个数不断递增，而且是唯一的，也就是说A和B不可能有相同的提案号），这个提案号会和修改请求一同发出，任何结点在“Prepare阶段”时都会拒绝其值小于当前提案号的请求。所以，结点A在向所有结点申请修改请求的时候，需要带一个提案号，越新的提案，这个提案号就越是是最大的。

如果接收结点收到的提案号n大于其它结点发过来的提案号，这个结点会回应Yes（本结点上最新的被批准提案号），并保证不接收其它\<n的提案。这样一来，结点上在Prepare阶段里总是会对最新的提案做承诺。

优化：在上述 prepare 过程中，如果任何一个结点发现存在一个更高编号的提案，则需要通知 提案人，提醒其中断这次提案。

第二阶段：Accept阶段

如果提案者A收到了超过半数的结点返回的Yes，然后他就会向所有的结点发布Accept Request（同样，需要带上提案号n），如果没有超过半数的话，那就返回失败。

当结点们收到了Accept Request后，如果对于接收的结点来说，n是最大的了，那么，它就会通过request（修改这个值），如果发现自己有一个更大的提案号，那么，结点就会拒绝request（拒绝修改）。

### 2.4 Raft协议(解决paxos的实现难度)

三个角色

follower、candidate、leader。
最开始大家都是follower，当follower监听不到leader，就可以自己成为candidate，发起投票

leader选举：timeout限制

follower成为candidate的超时时间，每个follower都在150ms and 300ms之间随机，之后看谁先timeout，谁就先成为candidate，然后它会先投自己一票，再向其他节点发起投票邀请。
如果其他节点在这轮选举还没有投过票，那么就给candidate投票，然后重置自己的选举timeout。
如果得到大多数的投票就成为leader，之后定期开始向follower发送心跳。

如果两个follower同时成为candidate的话，如果最后得到的票数相同，则等待其他follower的选择timeout之后成为candidate，继续开始新一轮的选举。

log复制

leader把变动的log借助心跳同步给follower，过半回复之后才成功提交，之后再下一次心跳之后，follower也commit变动，在自己的node上生效。

分裂之后，另一个分区的follower接受不到leader的timeout，然后会有一个先timeout，成为candidate，最后成为leader。

于是两个分区就有了两个leader。

当客户端有变动时，其中的leader由于无法收到过半的提交，则保持未提交状态。有的leader的修改，可以得到过半的提交，则可以修改生效。

当分裂恢复之后，leader开始对比选举的term，发现有更高的term存在时，他们会撤销未提交的修改，然后以最新的为准。

### 2.5 ISR的机制(解决f容错的2f+1成本问题)

Kafka并没有使用Zab或Paxos协议的多数投票机制来保证主备数据的一致性，而是提出了ISR的机制（In-Sync Replicas）的机制来保证数据一致性。

ISR认为对于2f+1个副本来说，多数投票机制要求最多只能允许f个副本发生故障，如果要支持2个副本的容错，则需要至少维持5个副本，对于消息系统的场景来说，效率太低。

ISR的运行机制如下：将所有次级副本数据分到两个集合，其中一个被称为ISR集合，这个集合备份数据的特点是即时和主副本数据保持一致，而另外一个集合的备份数据允许其消息队列落后于主副本的数据。在做主备切换时，只允许从ISR集合中选择主副本，只有ISR集合内所有备份都写成功才能认为这次写入操作成功。在具体实现时，kafka利用zookeeper来保持每个ISR集合的信息，当ISR集合内成员变化时，相关构件也便于通知。通过这种方式，如果设定ISR集合大小为f+1，那么可以最多允许f个副本故障，而对于多数投票机制来说，则需要2f+1个副本才能达到相同的容错性。

### 2.6 ZAB协议

#### 2.6.1 消息广播模式

zookeeper采用ZAB协议的核心就是只要有一台服务器提交了proposal，就要确保所有的服务器最终都能正确提交proposal。这也是CAP/BASE最终实现一致性的一个体现。

(1)客户端发起一个写操作请求

(2)Leader服务器将客户端的request请求转化为事物proposal提案，同时为每个proposal分配一个全局唯一的ID，即ZXID。

(3)leader服务器与每个follower之间都有一个队列，leader将消息发送到该队列

(4)follower机器从队列中取出消息处理完(写入本地事物日志中)毕后，向leader服务器发送ACK确认。

(5)leader服务器收到半数以上的follower的ACK后，即认为可以发送commit

(6)leader向所有的follower服务器发送commit消息

#### 2.6.2 崩溃恢复

确保已经被leader提交的proposal必须最终被所有的follower服务器提交。
确保丢弃已经被leader出的但是没有被提交的proposal。

leader服务器发生崩溃时分为如下场景：

(1)leader在提出proposal时未提交之前崩溃，则经过崩溃恢复之后，新选举的leader一定不能是刚才的leader。因为这个leader存在未提交的proposal。新选举出来的leader不能包含未提交的proposal，即新选举的leader必须都是已经提交了的proposal的follower服务器节点。同时，新选举的leader节点中含有最高的ZXID。这样做的好处就是可以避免了leader服务器检查proposal的提交和丢弃工作。

(2)leader在发送commit消息之后，崩溃。即消息已经发送到队列中。经过崩溃恢复之后，参与选举的follower服务器(刚才崩溃的leader有可能已经恢复运行，也属于follower节点范畴)中有的节点已经是消费了队列中所有的commit消息。即该follower节点将会被选举为最新的leader。剩下动作就是数据同步过程。

#### 2.6.3 数据同步

在zookeeper集群中新的leader选举成功之后，leader会将自身的提交的最大proposal的事物ZXID发送给其他的follower节点。follower节点会根据leader的消息进行回退或者是数据同步操作。最终目的要保证集群中所有节点的数据副本保持一致。

数据同步完之后，zookeeper集群如何保证新选举的leader分配的ZXID是全局唯一呢？这个就要从ZXID的设计谈起。

ZXID是一个长度64位的数字，其中低32位是按照数字递增，即每次客户端发起一个proposal,低32位的数字简单加1。高32位是leader周期的epoch编号，，每当选举出一个新的leader时，新的leader就从本地事物日志中取出ZXID,然后解析出高32位的epoch编号，进行加1，再将低32位的全部设置为0。这样就保证了每次新选举的leader后，保证了ZXID的唯一性而且是保证递增的。

## 3 ZooKeeper应用场景

### 3.1 发布/订阅

数据发布/订阅系统，即配置中心。需要发布者将数据发布到Zookeeper的节点上，供订阅者进行数据订阅，进而达到动态获取数据的目的，实现配置信息的集中式管理和数据的动态更新。

发布/订阅一般有两种设计模式：推模式和拉模式

（1）推模式：服务端主动将数据更新发送给所有订阅的客户端
（2）拉模式：客户端主动请求获取最新数据

Zookeeper采用了推拉相结合的模式，客户端向服务端注册自己需要关注的节点，一旦该节点数据发生变更，那么服务端就会向相应的客户端推送Watcher事件通知，客户端接收到此通知后，主动到服务端获取最新的数据。

若将配置信息存放到Zookeeper上进行集中管理，在通常情况下，应用在启动时会主动到Zookeeper服务端上进行一次配置信息的获取，同时，在指定节点上注册一个Watcher监听，这样在配置信息发生变更，服务端都会实时通知所有订阅的客户端，从而达到实时获取最新配置的目的。

### 3.2 负载均衡

· 域名配置，首先在Zookeeper上创建一个节点来进行域名配置，如DDNS/app1/server.app1.company1.com。

· 域名解析，应用首先从域名节点中获取IP地址和端口的配置，进行自行解析。同时，应用程序还会在域名节点上注册一个数据变更Watcher监听，以便及时收到域名变更的通知。

· 域名变更，若发生IP或端口号变更，此时需要进行域名变更操作，此时，只需要对指定的域名节点进行更新操作，Zookeeper就会向订阅的客户端发送这个事件通知，客户端之后就再次进行域名配置的获取。

### 3.3 命名服务

通过调用Zookeeper节点创建的API接口就可以创建一个顺序节点，并且在API返回值中会返回这个节点的完整名字，利用此特性，可以生成全局ID，其步骤如下

1. 客户端根据任务类型，在指定类型的任务下通过调用接口创建一个顺序节点，如"job-"。

2. 创建完成后，会返回一个完整的节点名，如"job-00000001"。

3. 客户端拼接type类型和返回值后，就可以作为全局唯一ID了，如"type2-job-00000001"。

### 3.4 分布式协调/通知

Zookeeper中特有的Watcher注册于异步通知机制，能够很好地实现分布式环境下不同机器，甚至不同系统之间的协调与通知，从而实现对数据变更的实时处理。通常的做法是不同的客户端都对Zookeeper上的同一个数据节点进行Watcher注册，监听数据节点的变化（包括节点本身和子节点），若数据节点发生变化，那么所有订阅的客户端都能够接收到相应的Watcher通知，并作出相应处理。

Zookeeper主要负责进行分布式协调工作，在具体的实现上，根据功能将数据复制组件划分为三个模块：Core（实现数据复制核心逻辑，将数据复制封装成管道，并抽象出生产者和消费者概念）、Server（启动和停止复制任务）、Monitor（监控任务的运行状态，若数据复制期间发生异常或出现故障则进行告警）

### 3.5 集群管理

ZooKeeper2大特性：

（1）客户端如果对Zookeeper的数据节点注册Watcher监听，那么当该数据及诶单内容或是其子节点列表发生变更时，Zookeeper服务器就会向订阅的客户端发送变更通知。

（2）对在Zookeeper上创建的临时节点，一旦客户端与服务器之间的会话失效，那么临时节点也会被自动删除。

利用其两大特性，可以实现集群机器存活监控系统，若监控系统在节点上注册一个Watcher监听，那么但凡进行动态添加机器的操作，就会在节点下创建一个临时节点，这样，监控系统就能够实时监测机器的变动情况。

比如：分布式日志收集系统

### 3.6 Master选举

在分布式系统中，Master往往用来协调集群中其他系统单元，具有对分布式系统状态变更的决定权，如在读写分离的应用场景中，客户端的写请求往往是由Master来处理，或者其常常处理一些复杂的逻辑并将处理结果同步给其他系统单元。利用Zookeeper的强一致性，能够很好地保证在分布式高并发情况下节点的创建一定能够保证全局唯一性，即Zookeeper将会保证客户端无法重复创建一个已经存在的数据节点。

### 3.7 分布式锁

分布式锁用于控制分布式系统之间同步访问共享资源的一种方式，可以保证不同系统访问一个或一组资源时的一致性，主要分为排它锁和共享锁。

排它锁又称为写锁或独占锁，若事务T1对数据对象O1加上了排它锁，那么在整个加锁期间，只允许事务T1对O1进行读取和更新操作，其他任何事务都不能再对这个数据对象进行任何类型的操作，直到T1释放了排它锁。

共享锁又称为读锁，若事务T1对数据对象O1加上共享锁，那么当前事务只能对O1进行读取操作，其他事务也只能对这个数据对象加共享锁，直到该数据对象上的所有共享锁都被释放。

① 获取锁，在需要获取共享锁时，所有客户端都会到/shared_lock下面创建一个临时顺序节点，如果是读请求，那么就创建例如/shared_lock/host1-R-00000001的节点，如果是写请求，那么就创建例如/shared_lock/host2-W-00000002的节点。

② 判断读写顺序，不同事务可以同时对一个数据对象进行读写操作，而更新操作必须在当前没有任何事务进行读写情况下进行，通过Zookeeper来确定分布式读写顺序，大致分为四步。

1. 创建完节点后，获取/shared_lock节点下所有子节点，并对该节点变更注册监听。

2. 确定自己的节点序号在所有子节点中的顺序。

3. 对于读请求：若没有比自己序号小的子节点或所有比自己序号小的子节点都是读请求，那么表明自己已经成功获取到共享锁，同时开始执行读取逻辑，若有写请求，则需要等待。对于写请求：若自己不是序号最小的子节点，那么需要等待。

4. 接收到Watcher通知后，重复步骤1。

③ 释放锁，其释放锁的流程与独占锁一致。

#### 3.7.1 惊群效应

惊群效应：一个锁释放,所有等待的线程都被唤醒

每个锁竞争者，只需要关注/shared_lock节点下序号比自己小的那个节点是否存在即可。

### 3.8 分布式队列

分布式队列可以简单分为先入先出队列模型和等待队列元素聚集后统一安排处理执行的Barrier模型。

① FIFO先入先出，先进入队列的请求操作先完成后，才会开始处理后面的请求

② Barrier分布式屏障，最终的合并计算需要基于很多并行计算的子结果来进行
