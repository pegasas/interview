# HTTP Header

1、通用头 General

Request URL ：请求的url
Request Method ： 请求的方法，可以是GET、POST
Status Code：HTTP 状态码，表示请求成功
Remote Address：远程IP地址
Referrer Policy: Referrer，用以指定该请求是从哪个页面跳转页来的，常被用于分析用户来源等信息。但是也有成为用户的一个不安全因素，比如有些网站直接将 sessionid 或是 token 放在地址栏里传递的，会原样不动地当作 Referrer 报头的内容传递给第三方网站。

2、响应头 Response Headers

Cache-Control：
指定请求和响应遵循的缓存机制，
Public 指示响应可被任何缓存区缓存。
Private 指示对于单个用户的整个或部分响应消息，不能被共享缓存处理。这允许服务器仅仅描述当用户的部
分响应消息，此响应消息对于其他用户的请求无效。
Content-Encoding：内容的压缩类型，此处是gzip
Content-Length：返回的内容的长度
Content-type：返回的内容类型，此处是html
Connection：
例如：Connection: keep-alive 当一个网页打开完成后，客户端和服务器之间用于传输HTTP数据的TCP连接不会关闭，如果客户端再次访问这个服务器上的网页，会继续使用这一条已经建立的连接
例如：Connection: close 代表一个Request完成后，客户端和服务器之间用于传输HTTP数据的TCP连接会关闭， 当客户端再次发送Request，需要重新建立TCP连接。
Date：请求的日期
Expires： 响应过期的日期和时间
Server：服务器

3、请求头 Request Headers

Accept：浏览器能够接收的内容类型，如text/javascript
Accept-Encoding：浏览器支持的压缩编码类型。
Accept-language：浏览器支持的语言
Cookie：HTTP请求发送时，会把保存在该请求域名下的所有cookie值一起发送给web服务器。
Host：指定请求的服务器的域名和端口号
Referer： 先前网页的地址，当前请求网页紧随其后,即来路
User-Agent：包含发出请求的用户信息
