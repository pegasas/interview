# HTTP重定向基本原理

在HTTP协议中，有三类重定向状态码：301 redirect、302 redirect与meta fresh。
301 redirect代表永久性转移（Permaneutly Moved），
302 redirect代表暂时性转移（Temporarily Moved），
meta fresh代表在特定时间后重定向到新的网页。

HTTP状态代码是在服务器返回数据的第一行实现的，比如你访问www.g.cn这个网址．
Google的服务器返回的数据第一行是：HTTP/1.1 301 Moved Permanently，
页面自动眺转到http://www.google.cn，
表示g.cn这个URL被永久重定向到http://www.google.cn。

三类重定向的作用都是将用户的资源请求转向到另外一个URL，
而这一节所说的HTTP重定向，是用于CDN均衡调度的，
显然我们应该选择302 redirect。
因为负载均衡系统需要实现的是将用户立刻且暂时性地重定向到另一台服务设备上去。

比如浏览器请求www.tanxingcai.com这个域名，服务器返回应答消息如下：
HTTP/1.1 302 Found
Date: Wed, 17 Mar 2010 08:11:11 GMT
Server: Apache/2.2.15 (Unix) mod_ssl/2.2.15 0penSSL/0.9.7a DAV/2
PHP/5.2.9
X-Powered-By, PHP/5.2.9
Location :   http://bj.tanxingcai.com
Content-Length: 0

浏览器得到这个应答后，会再次发起请求，
只不过请求的域名替换成http://bj.tanxingcai.com。
有时候，域名替代也直接使用IP地址。

利用HTTP重定向的GSLB工作过程大致是：当用户在浏览器地址栏里输入一个域名，
如HTTP://www.tanxingcai.com时，本地DNS服务器返回的解析结果是GSLB的IP地址。
用户端浏览器会向GSLB发起HTTP请求，GSLB向浏览器返回响应请求的HTTP数据包，
这个HTTP数据包的“头信息（Header）”中会包含一条“302 found”信息，
告诉用户向某个IP地址的服务器请求内容。
在这个过程中，GSLB还会做一个动作，就是修改原来的URL，把它改写成另一个URL'，
这个URL'中的域名就是用户要去进行内容请求的服务设备的主机名。
用户向这个服务器请求内容得到应答后，才是真正开始浏览网页内容的过程。

与智能DNS方式相似，GSLB在做HTTP重定向时，也就是在为用户选择最佳服务器时，
也会综合考虑很多条件，比如服务器的负载情况、用户与各个服务器之间的链路质量、
用户请求的内容所在位置等。区别在于，相比较于DNS方式，
基于HTTP重定向方式的GSLB系统能够看到用户的IP地址，以及用户请求的具体内容，
所以能够进行更精细的定位，这是HTTP重定向方式的最大优点。

当然HTTP重定向也有其不足。顾名思义，这种方法只适用于HTTP应用，
不适用于任何其他应用。比如微软的MMS协议、RTSP协议，
就不能使用这种方式进行重定向。其次，由于HTTP重定向过程需要额外解析域名URL'，
还需要与URL'建立TCP连接并且发送HTTP请求，使得响应时间加长。
第三，不同于DNS方式，没有任何用户请求能被外部系统终结，
所有请求都必须进入 GSLB系统，这将成为性能和可靠性的瓶颈。
