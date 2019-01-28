# SQL优化explain

执行计划包含的信息

+----+-------------+-------+-------+---------------+---------+---------+------+------+-------------+
| id | select_type | table | type  | possible_keys | key     | key_len | ref  | rows | Extra       |
+----+-------------+-------+-------+---------------+---------+---------+------+------+-------------+

1. id：

包含一组数字，表示查询中执行select子句或操作表的顺序

2. select_type

示查询中每个select子句的类型（简单OR复杂）

a. SIMPLE：查询中不包含子查询或者UNION
b. 查询中若包含任何复杂的子部分，最外层查询则被标记为：PRIMARY
c. 在SELECT或WHERE列表中包含了子查询，该子查询被标记为：SUBQUERY
d. 在FROM列表中包含的子查询被标记为：DERIVED（衍生）用来表示包含在from子句中的子查询的select，mysql会递归执行并将结果放到一个临时表中。服务器内部称为"派生表"，因为该临时表是从子查询中派生出来的
e. 若第二个SELECT出现在UNION之后，则被标记为UNION；若UNION包含在FROM子句的子查询中，外层SELECT将被标记为：DERIVED
f. 从UNION表获取结果的SELECT被标记为：UNION RESULT

SUBQUERY和UNION还可以被标记为DEPENDENT和UNCACHEABLE。
DEPENDENT意味着select依赖于外层查询中发现的数据。
UNCACHEABLE意味着select中的某些 特性阻止结果被缓存于一个item_cache中。

3. type

表示MySQL在表中找到所需行的方式，又称“访问类型”，常见类型如下:

ALL, index,  range, ref, eq_ref, const, system, NULL

从左到右，性能从最差到最好

ALL：Full Table Scan， MySQL将遍历全表以找到匹配的行

index：Full Index Scan，index与ALL区别为index类型只遍历索引树

range:索引范围扫描，对索引的扫描开始于某一点，返回匹配值域的行。显而易见的索引范围扫描是带有between或者where子句里带有<, >查询。当mysql使用索引去查找一系列值时，例如IN()和OR列表，也会显示range（范围扫描）,当然性能上面是有差异的。

ref：使用非唯一索引扫描或者唯一索引的前缀扫描，返回匹配某个单独值的记录行

eq_ref：类似ref，区别就在使用的索引是唯一索引，对于每个索引键值，表中只有一条记录匹配，简单来说，就是多表连接中使用primary key或者 unique key作为关联条件

const、system：当MySQL对查询某部分进行优化，并转换为一个常量时，使用这些类型访问。如将主键置于where列表中，MySQL就能将该查询转换为一个常量

注：system是const类型的特例，当查询的表只有一行的情况下，使用system

NULL：MySQL在优化过程中分解语句，执行时甚至不用访问表或索引，例如从一个索引列里选取最小值可以通过单独索引查找完成。

4. possible_keys
指出MySQL能使用哪个索引在表中找到记录，查询涉及到的字段上若存在索引，则该索引将被列出，但不一定被查询使用

5. key
显示MySQL在查询中实际使用的索引，若没有使用索引，显示为NULL

6. key_len
表示索引中使用的字节数，可通过该列计算查询中使用的索引的长度（key_len显示的值为索引字段的最大可能长度，并非实际使用长度，即key_len是根据表定义计算而得，不是通过表内检索出的）

7. ref
表示上述表的连接匹配条件，即哪些列或常量被用于查找索引列上的值

8. rows
表示MySQL根据表统计信息及索引选用情况，估算的找到所需的记录所需要读取的行数

9. Extra
包含不适合在其他列中显示但十分重要的额外信息
a. Using index
该值表示相应的select操作中使用了覆盖索引（Covering Index）
覆盖索引（Covering Index）
MySQL可以利用索引返回select列表中的字段，而不必根据索引再次读取数据文件
包含所有满足查询需要的数据的索引称为覆盖索引（Covering Index）
注意：如果要使用覆盖索引，一定要注意select列表中只取出需要的列，不可select * ，因为如果将所有字段一起做索引会导致索引文件过大，查询性能下降

b. Using where
表示mysql服务器将在存储引擎检索行后再进行过滤。许多where条件里涉及索引中的列，当（并且如果）它读取索引时，就能被存储引擎检验，因此不是所有带where字句的查询都会显示"Using where"。有时"Using where"的出现就是一个暗示：查询可受益与不同的索引。

c. Using temporary
表示MySQL需要使用临时表来存储结果集，常见于排序和分组查询

这个值表示使用了内部临时(基于内存的)表。一个查询可能用到多个临时表。有很多原因都会导致MySQL在执行查询期间创建临时表。两个常见的原因是在来自不同表的上使用了DISTINCT,或者使用了不同的ORDER BY和GROUP BY列。可以强制指定一个临时表使用基于磁盘的MyISAM存储引擎。这样做的原因主要有两个：
1)内部临时表占用的空间超过min(tmp_table_size，max_heap_table_size)系统变量的限制
2)使用了TEXT/BLOB 列

d. Using filesort
MySQL中无法利用索引完成的排序操作称为“文件排序”

e. Using join buffer
改值强调了在获取连接条件时没有使用索引，并且需要连接缓冲区来存储中间结果。如果出现了这个值，那应该注意，根据查询的具体情况可能需要添加索引来改进能。

f. Impossible where
这个值强调了where语句会导致没有符合条件的行。

h. Select tables optimized away
这个值意味着仅通过使用索引，优化器可能仅从聚合函数结果中返回一行.

I. Index merges
当MySQL 决定要在一个给定的表上使用超过一个索引的时候，就会出现以下格式中的一个，详细说明使用的索引以及合并的类型。
Using sort_union(...)
Using union(...)
Using intersect(...)

总结：
• EXPLAIN不会告诉你关于触发器、存储过程的信息或用户自定义函数对查询的影响情况
• EXPLAIN不考虑各种Cache
• EXPLAIN不能显示MySQL在执行查询时所作的优化工作
• 部分统计信息是估算的，并非精确值
• EXPALIN只能解释SELECT操作，其他操作要重写为SELECT后查看执行计划。
