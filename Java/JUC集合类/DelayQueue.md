# DelayQueue

内部结构

可重入锁
用于根据delay时间排序的优先级队列
用于优化阻塞通知的线程元素leader
用于实现阻塞和通知的Condition对象

delayed和PriorityQueue
delayed是一个具有过期时间的元素
PriorityQueue是一个根据队列里元素某些属性排列先后的顺序队列

delayQueue其实就是在每次往优先级队列中添加元素,然后以元素的delay/过期值作为排序的因素,以此来达到先过期的元素会拍在队首,每次从队列里取出来都是最先要过期的元素
