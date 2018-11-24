# join原理

Simple Nested-Loop Join：效率最低，按照join的次序，在join的属性上一个个扫描，并合并结果。

Index Nested-Loop Join：效率最高，join的属性上面有索引，根据索引来匹配。

Block Nested-Loop Join：用于没有索引的列。它会采用join buffer，将外表的值缓存到join buffer中，然后与内表进行批量比较，这样可以降低对外表的访问频率
