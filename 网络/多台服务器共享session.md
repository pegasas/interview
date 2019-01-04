# 多台服务器共享session

1.通过数据库mysql共享session

    a.采用一台专门的mysql服务器来存储所有的session信息。

     用户访问随机的web服务器时，会去这个专门的数据库服务器check一下session的情况，以达到session同步的目的。

     缺点就是：依懒性太强，mysql服务器无法工作，影响整个系统；

   b.将存放session的数据表与业务的数据表放在同一个库。如果mysql做了主从，需要每一个库都需要存在这个表，并且需要数据实时同步。

   缺点：用数据库来同步session，会加大数据库的负担，数据库本来就是容易产生瓶颈的地方，如果把session还放到数据库里面，无疑是雪上加霜。上面的二种方法，第一点方法较好，把放session的表独立开来，减轻了真正数据库的负担 。但是session一般的查询频率较高，放在数据库中查询性能也不是很好，不推荐使用这种方式。

2.通过cookie共享session

  把用户访问页面产生的session放到cookie里面，就是以cookie为中转站。

  当访问服务器A时，登录成功之后将产生的session信息存放在cookie中；当访问请求分配到服务器B时，服务器B先判断服务器有没有这个session，如果没有，在去看看客户端的cookie里面有没有这个session，如果cookie里面有，就把cookie里面的sessoin同步到web服务器B，这样就可以实现session的同步了。

  缺点：cookie的安全性不高，容易伪造、客户端禁止使用cookie等都可能造成无法共享session。

3.通过服务器之间的数据同步session

　　使用一台作为用户的登录服务器，当用户登录成功之后，会将session写到当前服务器上，我们通过脚本或者守护进程将session同步到其他服务器上，这时当用户跳转到其他服务器，session一致，也就不用再次登录。

　　缺陷：速度慢，同步session有延迟性，可能导致跳转服务器之后，session未同步。而且单向同步时，登录服务器宕机，整个系统都不能正常运行。

4.通过NFS共享Session

　　选择一台公共的NFS服务器（Network File Server）做共享服务器，所有的Web服务器登陆的时候把session数据写到这台服务器上，那么所有的session数据其实都是保存在这台NFS服务器上的，不论用户访问那太Web服务器，都要来这台服务器获取session数据，那么就能够实现共享session数据了。

　　缺点：依赖性太强，如果NFS服务器down掉了，那么大家都无法工作了，当然，可以考虑多台NFS服务器同步的形式。

5.通过memcache同步session

　　memcache可以做分布式，如果没有这功能，他也不能用来做session同步。他可以把web服务器中的内存组合起来，成为一个"内存池"，不管是哪个服务器产生的sessoin都可以放到这个"内存池"中，其他的都可以使用。

　　优点：以这种方式来同步session，不会加大数据库的负担，并且安全性比用cookie大大的提高，把session放到内存里面，比从文件中读取要快很多。

　　缺点：memcache把内存分成很多种规格的存储块，有块就有大小，这种方式也就决定了，memcache不能完全利用内存，会产生内存碎片，如果存储块不足，还会产生内存溢出。

6.通过redis共享session

　　redis与memcache一样，都是将数据放在内存中。区别的是redis会周期性的把更新的数据写入磁盘或者把修改操作写入追加的记录文件，并且在此基础上实现了master-slave(主从)同步。

  根据实际开发应用，一般选择使用memcache或redis方式来共享session.
