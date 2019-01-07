﻿# MYISAM、innodb索引

InnoDB索引和MyISAM索引的区别：

一是主索引的区别，InnoDB的数据文件本身就是索引文件。而MyISAM的索引和数据是分开的。

二是辅助索引的区别：InnoDB的辅助索引data域存储相应记录主键的值而不是地址。而MyISAM的辅助索引和主索引没有多大区别。

三是聚集非聚集索引的区别：innodb引擎的数据在物理磁盘上是按照主键顺序存储的，而myisam引擎的表数据是随机分布的；
