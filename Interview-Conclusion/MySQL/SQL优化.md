# SQL优化

1 不使用子查询

例：SELECT * FROM t1 WHERE id (SELECT id FROM t2 WHERE name=’hechunyang’);

对于mysql，不推荐使用子查询和join是因为本身join的效率就是硬伤，一旦数据量很大效率就很难保证，强烈推荐分别根据索引单表取数据，然后在程序里面做join，merge数据。

子查询效率太差，执行子查询时，MYSQL需要创建临时表，查询完毕后再删除这些临时表，所以，子查询的速度会受到一定的影响，这里多了一个创建和销毁临时表的过程。

2 避免函数索引，mysql函数索引导致全表扫描

例：SELECT * FROM t WHERE YEAR(d) >= 2016;
由于MySQL不像Oracle那样支持函数索引，即使d字段有索引，也会直接全表扫描。
应改为—–>
SELECT * FROM t WHERE d >= ‘2016-01-01’;

3 用IN或UNION来替换OR(必须是统一索引或者分别加索引)

低效查询
SELECT * FROM t WHERE LOC_ID = 10 OR LOC_ID = 20 OR LOC_ID = 30;
—–>
高效查询
SELECT * FROM t WHERE LOC_IN IN (10,20,30);

这里的type项是index_merge。搜索后发现是MySQL5.0后的新技术，索引合并。index merge 技术简单说就是在用OR，AND连接的多个查询条件时，可以分别使用前后查询中的索引，然后将它们各自的结果合并交集或并集。当然具体是否使用index merge，优化器会自己选择，比如and连接时有联合索引，或干脆全表查询就很快，就没必要使用它了。


4 LIKE双百分号无法使用到索引

SELECT * FROM t WHERE name LIKE ‘%de%’;
—–>
SELECT * FROM t WHERE name LIKE ‘de%’;

like %keyword    索引失效，使用全表扫描。但可以通过翻转函数+like前模糊查询+建立翻转函数索引=走翻转函数索引，不走全表扫描。

like keyword%    索引有效。

目前只有MySQL5.7支持全文索引（支持中文）

5 读取适当的记录LIMIT M,N

虽然只需要返回rows行记录，但却必须先访问offset行不会用到的记录。对一张数据量很大的表进行查询时，offset值可能非常大，此时limit语句的效率就非常低了。

SELECT message.* FROM message LIMIT 0,1000
SELECT message.* FROM message LIMIT 1000,1000
SELECT message.* FROM message LIMIT 2000,1000
—–>
SELECT message.* FROM message WHERE uid>0 LIMIT 1000
SELECT message.* FROM message WHERE uid>1000 LIMIT 1000
SELECT message.* FROM message WHERE uid>2000 LIMIT 1000

6 避免数据类型不一致

SELECT * FROM t WHERE id = ’19’;
—–>
SELECT * FROM t WHERE id = 19;

mysql在比较值的时候，如果两边值的类型不匹配，那么就会进行隐式类型转换（转化成浮点类型）；

总结如下：

当等式两边类型不一致的时候，都会被转换为浮点数再进行比较。

当等式左边varchar类型转化成浮点类型的时候，不会命中索引；

当等式左边是浮点类型的时候，右边类型转化不转化都能够命中索引；

7 分组统计可以禁止排序

默认情况下，MySQL对所有GROUP BY col1，col2…的字段进行排序。如果查询包括GROUP BY，想要避免排序结果的消耗，则可以指定ORDER BY NULL禁止排序。

SELECT goods_id FROM t GROUP BY goods_id;
—–>

SELECT goods_id FROM t GROUP BY goods_id ORDER BY NULL;

8 避免随机取记录

改成其大于随机的某个值

SELECT * FROM t1 WHERE 1=1 ORDER BY RAND() LIMIT 4;
MySQL不支持函数索引，会导致全表扫描

SELECT * FROM t1 WHERE id >= CEIL(RAND() * 1000) LIMIT 4;

9 禁止不必要的ORDER BY排序，可以取出来在程序排序

SELECT count(1) FROM user u LEFT JOIN user_info i ON u.id = i.user_id WHERE 1 = 1 ORDER BY u.create_time DESC;
—–>
SELECT count(1) FROM user u LEFT JOIN user_info i ON u.id = i.user_id;

10 批量INSERT插入

INSERT INTO t (id, name) VALUES(1,’Bea’);
INSERT INTO t (id, name) VALUES(2,’Belle’);
INSERT INTO t (id, name) VALUES(3,’Bernice’);
—–>
INSERT INTO t (id, name) VALUES(1,’Bea’), (2,’Belle’),(3,’Bernice’);
