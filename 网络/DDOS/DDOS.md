﻿# DDOS(Distributed Denial of Service)
DDoS可以简单分为三类：
(1)海量数据包从互联网的各个角落蜂拥而来，堵塞IDC入口，让各种强大的硬件防御系统、快速高效的应急流程而无用武之地，这种类型的攻击典型代表是ICMP Flood和UDP Flood，现在已不常见；
(2)每隔几分钟发一个包甚至只需要一个包，就可以让服务器不再响应。这类攻击主要是利用协议或者软件的漏洞发起，如Slowloris攻击，Hash冲突攻击等，需要特定环境机缘巧合下才能出现；
(3)上述两种的混合，轻灵浑厚兼而有之，既利用了协议、系统的缺陷，又具备了海量的流量，如SYN Flood攻击，DNS Query Flood攻击，是当前的主流攻击方式。
1.1 SYN Flood  
  
  
SYN Flood是互联网上最经典的DDoS攻击方式之一，最早出现于1999年左右，雅虎是当时最著名的受害者。SYN Flood攻击利用了TCP三次握手的缺陷，能够以较小代价使目标服务器无法响应，且难以追查。  
标准的TCP三次握手过程如下：

1.  客户端发送一个包含SYN标志的TCP报文，SYN即同步（Synchronize），同步报文会指明客户端使用的端口以及TCP连接的初始序号;
2.  服务器在收到客户端的SYN报文后，将返回一个SYN ACK（即确认Acknowledgement）的报文，表示客户端的请求被接受，同时TCP初始序号自动加一。
3.  客户端也返回一个确认报文ACK给服务器端，同样TCP序列号被加一。

  
经过这三步，TCP连接就建立完成。TCP协议为了实现可靠传输，在三次握手的过程中设置了一些异常处理机制。第三步中如果服务器没收到客户端的最终ACK确认报文，会一直处于SYN_RECV状态，将客户端IP加入等待列表，并重发第二步的SYN ACK报文。重发一般进行3-5次，大约间隔30秒左右轮询一次等待列表重试所有客户端。另一方面，服务器在自己发出了SYN ACK报文后，会预分配资源为即将建立的TCP连接储存信息做准备，这个资源在等待重试期间一直保留。更为重要的是，服务器资源有限，可以维护的SYN_RECV状态超过极限后就不再接受新的SYN报文，也就是拒绝新的TCP连接建立。  
SYN Flood正是利用了上文中TCP协议的设定，达到攻击的目的。攻击者伪装大量的IP地址给服务器发送SYN报文，由于伪造的IP地址几乎不可能存在，也就几乎没有设备会给服务器返回任何应答了。因此，服务器将会维持一个庞大的等待列表，不停的重试发送SYN ACK报文，同时占用着大量的资源无法释放。更关键的是，被攻击服务器的SYN_RECV队列被恶意的数据包占满，不再接受新的SYN请求，合法用户无法完成三次握手建立起TCP连接。也就是说，这个服务器被SYN Flood拒绝服务了。  
对SYN Flood有兴趣的可以看看[http://www.icylife.net/yunshu/show.php?id=367](http://www.icylife.net/yunshu/show.php?spm=0.0.0.0.WQH6cn&id=367)，这是笔者2006年写的代码，后来做过几次修改，修改bug，并降低了攻击性，纯做测试使用。  
  
1.2 DNS Query Flood  
  
  
作为互联网最基础最核心的服务，DNS自然也是DDoS攻击的重要目标之一。打垮DNS服务能够间接的打垮了一个公司的全部业务，或者打垮一个地区的网络服务。前些时候风头正盛的黑客组织anonymous也曾经宣布要攻击全球互联网的13台根DNS服务器，不过最终没有得手。  
UDP攻击是最容易发起海量流量的攻击手段，而且源IP随机伪造难以追查。但是过滤比较容易，因为大多数IP并不提供UDP服务，直接丢弃UDP流量即可。所以现在纯粹的UDP流量攻击比较少见了，取而代之的是UDP协议承载的DNS Query Flood攻击。简单地说，越上层协议上发动的DDoS攻击越难以防御，因为协议越上层，与业务关联越大，防御系统面临的情况越复杂。  
DNS Query Flood就是攻击者操纵大量傀儡机器，对目标发起海量的域名查询请求。为了防止基于ACL的过滤，必须提高数据包的随机性。常用的做法是UDP层随机伪造源IP地址，随机伪造源端口等参数。在DNS协议层，随机伪造查询ID以及待解析域名。随机伪造待解析域名除了防止过滤外，还可以降低命中DNS缓存的可能性，尽可能多的消耗DNS服务器的CPU资源。  
关于DNS Query  
Flood的代码，笔者2011年7月份为了测试服务器性能曾经写过一份代码，见[http://www.icylife.net/yunshu/show.php?id=832](http://www.icylife.net/yunshu/show.php?spm=0.0.0.0.WQH6cn&id=832)。同样的，这份代码人为降低了攻击性，只做测试用途。  
  
1.3 HTTP Flood  
  
  
上文描述的SYN  
Flood、DNS Query Flood在现阶段已经能做到有效防御了，真正另各大厂商以及互联网企业头疼的是HTTP Flood攻击。HTTP Flood是针对WEB服务在第七层协议发起的攻击，它巨大危害性主要表现在三个方面，发起方便；过滤困难；影响深远。  
SYN Flood和DNS Query Flood都需要攻击者以root权限控制大批量的傀儡机，收集大量root权限的傀儡机是很花费时间精力的一件事情，而且在攻击过程中傀儡机会由于流量异常被管理员发现，攻击者的资源快速损耗而补充缓慢，导致攻击强度明显降低而且不可长期持续。HTTP Flood攻击则不同，攻击者并不需要控制大批的傀儡机，取而代之的是通过端口扫描程序在互联网上寻找匿名的HTTP代理或者SOCKS代理，攻击者通过匿名代理对攻击目标发起HTTP请求。匿名代理是一种比较丰富的资源，花几天时间获取上午的代理并不是难事，因此攻击容易发起而且可以长期高强度的持续。  
另一方面，HTTP  
Flood攻击在HTTP层发起，极力模仿正常用户的网页请求行为，与网站业务紧密相关，安全厂商很难提供一套通用的且不影响用户体验的方案。在一个地方工作的很好的规则，换一个场景可能带来大量的误杀。  
最后，HTTP Flood攻击会引起严重的连锁反应，不仅仅是直接导致被攻击的WEB前端响应缓慢，还间接攻击到后端的JAVA等业务层逻辑以及更后端的数据库服务，增大他们的压力，甚至对日至存储服务器都带来影响。  
有意思的是，HTTP  
Flood还有个剖有历史渊源的昵称叫做CC攻击。CC是Challenge Collapsar的缩写，而Collapsar是国内一家著名安全公司的DDoS防御设备。从目前的情况来看，不仅仅是Collapsar，所有的硬件防御设备都还在被挑战着，风险并未解除。  
  
1.4 慢速连接攻击  
  
  
一提起攻击，第一反应就是海量的流量，海量的报文。但是有一种攻击却反其道而行之，以慢著称，以至于有些攻击目标被打死了都不知道是怎么死的，这就是慢速连接攻击，最具代表性的是rsnake发明的SlowLoris。  
HTTP协议规定，HTTP Request以\r\n\r\n结尾表示客户端发送结束，服务端开始处理。那么，如果永远不发送\r\n\r\n会如何？SlowLoris就是利用这一点来做DDoS攻击。攻击者在HTTP请求头中将Connection设置为Keep-Alive，要求Web Server保持TCP连接不要断开，随后缓慢的每隔几分钟发送一个key value格式的数据到服务端，如a:b\r\n，导致服务端认为HTTP头部没有接收完成而一直等待。如果攻击者使用多线程或者傀儡机来做同样的操作，服务器的WEB容器很快就被攻击者占满了TCP连接而不再接受新的请求。  
很快的，SlowLoris开始出现各种变种。比如POST方法向WEB Server提交数据，填充一大大Content-Length但是缓慢的一个字节一个字节的POST真正数据内容的等等。关于SlowLoris攻击，rsnake也给出了一个测试代码，[http://ha.ckers.org/slowloris/slowloris.pl](http://ha.ckers.org/slowloris/slowloris.pl?spm=0.0.0.0.WQH6cn&file=slowloris.pl)。
