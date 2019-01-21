# redis的过期策略和内存淘汰机制

## 名词解释

过期策略：即redis针对过期的key使用的清除策略，策略为，定期删除+惰性删除
内存淘汰机制：即内存占用达到内存限制设定值时触发的redis的淘汰策略来删除键

## 过期策略

定期删除，redis默认每隔100ms检查，是否有过期的key,有过期key则删除。需要说明的是，redis不是每隔100ms将所有的key检查一次，而是随机抽取进行检查(如果每隔100ms,全部key进行检查，redis岂不是卡死)。因此，如果只采用定期删除策略，会导致很多key到时间没有删除。

惰性删除，也就是说在你获取某个key的时候，redis会检查一下，这个key如果设置了过期时间那么是否过期了？如果过期了此时就会删除。

过期策略存在的问题，由于redis定期删除是随机抽取检查，不可能扫描清除掉所有过期的key并删除，然后一些key由于未被请求，惰性删除也未触发。这样redis的内存占用会越来越高。此时就需要内存淘汰机制

## 内存淘汰机制

redis配置文件中可以使用maxmemory <bytes>将内存使用限制设置为指定的字节数。当达到内存限制时，Redis会根据选择的淘汰策略来删除键。（ps：没搞明白为什么不是百分比）

策略有如下几种：（LRU的意思是：Least Recently Used最近最少使用的，LFU的意思是：Least Frequently Used最不常用的）

volatile-lru -> Evict using approximated LRU among the keys with an expire set.
                    在带有过期时间的键中选择最近最少使用的。（推荐）
allkeys-lru -> Evict any key using approximated LRU.
                    在所有的键中选择最近最少使用的。（不区分是否携带过期时间）（一般推荐）
volatile-lfu -> Evict using approximated LFU among the keys with an expire set.
                    在带有过期时间的键中选择最不常用的。
allkeys-lfu -> Evict any key using approximated LFU.
                    在所有的键中选择最不常用的。（不区分是否携带过期时间）
volatile-random -> Remove a random key among the ones with an expire set.
                    在带有过期时间的键中随机选择。
allkeys-random -> Remove a random key, any key.
                    在所有的键中随机选择。
volatile-ttl -> Remove the key with the nearest expire time (minor TTL)
                    在带有过期时间的键中选择过期时间最小的。
noeviction -> Don't evict anything, just return an error on write operations.
                    不要删除任何东西，只是在写操作上返回一个错误。默认。
