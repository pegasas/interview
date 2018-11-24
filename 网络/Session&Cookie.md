# Application,Session和Cookie的区别

方法|信息量大小|保存时间|应用范围|保存位置|
|--|--|--|--|--|
Application|任意大小|整个应用程序的生命期|所有用户|服务器端|
|--|--|--|--|--|
Session|小量,简单的数据|用户活动时间+一段延迟时间(一般为20分钟)|单个用户|服务器端|
|--|--|--|--|--|
Cookie|小量,简单的数据|可以根据需要设定|单个用户|客户端|

1.Application对象 
Application用于保存所有用户的公共的数据信息,如果使用Application对象,一个需要考虑的问题是任何写操作都要在Application_OnStart事件(global.asax)中完成.尽管使用Application.Lock和Applicaiton.Unlock方法来避免写操作的同步,但是它串行化了对Application对象的请求,当网站访问量大的时候会产生严重的性能瓶颈.因此最好不要用此对象保存大的数据集合

2.Session对象
Session用于保存每个用户的专用信息.她的生存期是用户持续请求时间再加上一段时间(一般是20分钟左右).Session中的信息保存在Web服务器内容中,保存的数据量可大可小.当Session超时或被关闭时将自动释放保存的数据信息.由于用户停止使用应用程序后它仍然在内存中保持一段时间,因此使用Session对象使保存用户数据的方法效率很低.对于小量的数据,使用Session对象保存还是一个不错的选择.使用Session对象保存信息的代码如下:
```
//存放信息
Session["username"]="zhouhuan";
//读取数据
string UserName=Session["username"].ToString();
```
 3.Cookie对象
Cookie用于保存客户浏览器请求服务器页面的请求信息,程序员也可以用它存放非敏感性的用户信息，信息保存的时间可以根据需要设置.如果没有设置Cookie失效日期,它们仅保存到关闭浏览器程序为止.如果将Cookie对象的Expires属性设置为Minvalue,则表示Cookie永远不会过期.Cookie存储的数据量很受限制,大多数浏览器支持最大容量为4096,因此不要用来保存数据集及其他大量数据.由于并非所有的浏览器都支持Cookie,并且数据信息是以明文文本的形式保存在客户端的计算机中,因此最好不要保存敏感的,未加密的数据,否则会影响网站的安全性.使用Cookie对象保存的代码如下:
```
//存放信息
Response.Cookies["UserID"].Value="0001";
//读取信息
string UserID=Response.Cookies["UserID"].Value;
```
