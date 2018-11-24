# SQL执行过程

1 SQL Interface: SQL接口。

接受用户的SQL命令，并且返回用户需要查询的结果。比如select from就是调用SQL Interface

2 Parser: 解析器。

SQL命令传递到解析器的时候会被解析器验证和解析。解析器是由Lex和YACC实现的，是一个很长的脚本。

在 MySQL中我们习惯将所有 Client 端发送给 Server 端的命令都称为 query ，在 MySQL Server 里面，连接线程接收到客户端的一个 Query 后，会直接将该 query 传递给专门负责将各种 Query 进行分类然后转发给各个对应的处理模块。
主要功能：
a . 将SQL语句进行语义和语法的分析，分解成数据结构，然后按照不同的操作类型进行分类，然后做出针对性的转发到后续步骤，以后SQL语句的传递和处理就是基于这个结构的。
b.  如果在分解构成中遇到错误，那么就说明这个sql语句是不合理的

3 Optimizer: 查询优化器。

SQL语句在查询之前会使用查询优化器对查询进行优化。就是优化客户端请求的 query（sql语句） ，根据客户端请求的 query 语句，和数据库中的一些统计信息，在一系列算法的基础上进行分析，得出一个最优的策略，告诉后面的程序如何取得这个 query 语句的结果

他使用的是“选取-投影-联接”策略进行查询。
用一个例子就可以理解： select uid,name from user where gender = 1;这个select 查询先根据where 语句进行选取，而不是先将表全部查询出来以后再进行gender过滤,这个select查询先根据uid和name进行属性投影，而不是将属性全部取出以后再进行过滤,将这两个查询条件联接起来生成最终查询结果

4 Cache和Buffer：查询缓存。

他的主要功能是将客户端提交 给MySQL 的 Select 类 query 请求的返回结果集 cache 到内存中，与该 query 的一个 hash 值做一个对应。该 Query 所取数据的基表发生任何数据的变化之后， MySQL 会自动使该 query 的Cache 失效。在读写比例非常高的应用系统中， Query Cache 对性能的提高是非常显著的。当然它对内存的消耗也是非常大的。如果查询缓存有命中的查询结果，查询语句就可以直接去查询缓存中取数据。这个缓存机制是由一系列小缓存组成的。比如表缓存，记录缓存，key缓存，权限缓存等

5 、存储引擎接口

存储引擎接口模块可以说是 MySQL 数据库中最有特色的一点了。目前各种数据库产品中，基本上只有 MySQL 可以实现其底层数据存储引擎的插件式管理。这个模块实际上只是 一个抽象类，但正是因为它成功地将各种数据处理高度抽象化，才成就了今天 MySQL 可插拔存储引擎的特色。从图2还可以看出，MySQL区别于其他数据库的最重要的特点就是其插件式的表存储引擎。MySQL插件式的存储引擎架构提供了一系列标准的管理和服务支持，这些标准与存储引擎本身无关，可能是每个数据库系统本身都必需的，如SQL分析器和优化器等，而存储引擎是底层物理结构的实现，每个存储引擎开发者都可以按照自己的意愿来进行开发。

注意：存储引擎是基于表的，而不是数据库。

SELECT `name`,COUNT(`name`) AS num FROM student WHERE grade < 60 GROUP BY `name` HAVING num >= 2 ORDER BY num DESC,`name` ASC LIMIT 0,2;

1，一条查询的sql语句先执行的是 FROM student 负责把数据库的表文件加载到内存中去

2，WHERE grade < 60，会把（图1.0）所示表中的数据进行过滤，取出符合条件的记录行，生成一张临时表

3，GROUP BY `name`会把图（1.3）的临时表切分成若干临时表

4，SELECT 的执行读取规则分为sql语句中有无GROUP BY两种情况。

（1）当没有GROUP BY时，SELECT 会根据后面的字段名称对内存中的一张临时表整列读取。

（2）当查询sql中有GROUP BY时，会对内存中的若干临时表分别执行SELECT，而且只取各临时表中的第一条记录，然后再形成新的临时表。这就决定了查询sql使用GROUP BY的场景下，SELECT后面跟的一般是参与分组的字段和聚合函数，否则查询出的数据要是情况而定。另外聚合函数中的字段可以是表中的任意字段，需要注意的是聚合函数会自动忽略空值。

5，HAVING num >= 2对上图所示临时表中的数据再次过滤，与WHERE语句不同的是HAVING 用在GROUP BY之后，WHERE是对FROM student从数据库表文件加载到内存中的原生数据过滤，而HAVING 是对SELECT 语句执行之后的临时表中的数据过滤，所以说column AS otherName ,otherName这样的字段在WHERE后不能使用，但在HAVING 后可以使用。但HAVING的后使用的字段只能是SELECT 后的字段，SELECT后没有的字段HAVING之后不能使用。HAVING num >= 2语句执行之后生成一张临时表

6，ORDER BY num DESC,`name` ASC对以上的临时表按照num，name进行排序。

7，LIMIT 0,2取排序后的前两个。
