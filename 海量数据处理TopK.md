# 海量数据处理TopK

①top K问题

在海量数据中找出TopK。
一般解法：
（1）分治：将数据集按照Hash方法分解成多个小数据集
（2）Trie树/hash：Trie树或Hash统计每个小数据集中的频率
（3）小顶堆：求出每个数据集中出现频率最高的前K个数
（4）把（1）种多个小数据集的结果进行两两归并

②重复问题：BitMap位图 或 Bloom Filter布隆过滤器 或 Hash_set集合。每个元素对应一个bit处理。

③排序问题：外部排序 或 BitMap位图。分割文件+文件内排序+文件之间归并。

1亿个浮点数找出最大的10000个？

将1亿个数据分成100份，每份100万个数据，找到每份数据中最大的10000个，最后在剩下的100 * 10000个数据里面找出最大的10000个。如果100万数据选择足够理想，那么可以过滤掉1亿数据里面99%的数据。100万个数据里面查找最大的10000个数据的方法如下：用快速排序的方法，将数据分为2堆，如果大的那堆个数N大于10000个，继续对大堆快速排序一次分成2堆，如果大的那堆个数N大于10000个，继续对大堆快速排序一次分成2堆，如果大堆个数N小于10000个，就在小的那堆里面快速排序一次，找第10000-n大的数字；递归以上过程，就可以找到第1w大的数。参考上面的找出第1w大数字，就可以类似的方法找到前10000大数字了。此种方法需要每次的内存空间为10^6*4=4MB，一共需要101次这样的比较。

Top K问题：

1. 有一个1G大小的一个文件，里面每一行是一个词，词的大小不超过16字节，内存限制大小是1M。返回频数最高的100个词。

①分治：顺序读文件，对每个词x取Hash(x)%2000，按照该值存到2000个小文件中。每个文件是500k左右。如果有文件超过了1M则继续分割。O(N)

②Trie树/Hash_map：字符串用Trie树最好。对每个小文件，统计其中出现的词频。O(N)*(平均字符长度),长度一般是常数，也就是O(N).

③小顶堆：用容量为100的小顶堆，以频率为value值插入，取每个文件现频率最大的100个词，把这100个词及相应的频率存入文件。最差O（N）*lg(100)，也就是O（N）.注：2,3步骤合起来需要一轮磁盘存取过程。存入文件的个数可以缩减一下，因为主要开销在磁盘读取上，减少文件读取次数，可以在每个文件存取最大容量的字符数量，比如这道题1*（M/16字节字符串长度+频率（int）8字节）的数存到一个文件中。比如20000个词存在一个文件中，可以缩减到10个文件。这样最后一步只需要读取10次就可以了。

④归并：将得到的10个文件里面的数进行归并，取前100个词。注：我觉得其实不需要多路归并，因为只需要找top100的数，归并排序首先是nlgn的复杂度，第二是频繁的磁盘存取，这里最好是还是在内存建立容量为100的小顶堆，依次读文件，遍历每个文件中的元素更新小顶堆，这样只需10次存取，并且时间复杂度是nlog100,也就是O（n）的。

注释：为什么说用Trie树好，我之前一直没想明白，因为网上说Trie树是空间换时间，而这道题是空间敏感呀的。总结了一下，其实是两点我没想明白：

1.字符串会通过一个hash算法（BKDRHash，APHash，DJBHash，JSHash，RSHash，SDBMHash，可以自己看一下，基本就是按位来进行hash的）映射为一个正整数然后对应到hash表中的一个位置，表中记录的value值是次数，这样统计次数只需要将字符串hash一下找到对应位置把次数+1就行了。如果这样的话hash中是不是不用存储字符串本身？如果不存储字符串本身，那应该是比较省空间的。而且效率的话因为Tire树找到一个字符串也是要按位置比较一遍，所以效率差不多呀。但是，其实字符串的hash是要存储字符串本身的，不管是开放地址法还是散列表法，都无法做到不冲突。除非桶个数是字符串的所有情况26^16，那是肯定空间不够的，因此hash表中必须存着字符串的值，也就是key值。字符串本身，那么hash在空间上肯定是定比不过Trie树的，因为Trie树对公共前缀只存储一次。

2.为什么说Trie树是空间换时间呢，我觉得网上这么说不甚合理，这句话其实是相对于二叉查找树来说的，之所以效率高，是因为二叉查找树每次查找都要比较大小，并且因为度为2，查找深度很大，比较次数也多，因此效率差。而Trie树是按位进行hash的，比如26个字母组成的字符串，每次找对应位的字符-‘a’就是位置了。而且度是26，查找深度就是字符串位数，查找起来效率自然就很快。但是为啥说是空间换时间，是因为字符串的Trie树若想存储所有的可能字符串，比如16位，一个点要对应下一位26种情况，也就是26个分支，也得26^16个位置，所以空间是很大的。但是Trie树的话可以采用依次插入的，不需要每个点记录26个点，而是只存在有值的分支，Trie树节点只要存频率次数，插入的流程就是挨个位子找分支，没有就新建，有就次数+1就行了。因此空间上很省，因为重复前缀就统计一次，而效率很高，O(length)。


2. 海量日志数据，提取出某日访问百度次数最多的那个IP。注：跟上一题一致，甚至更简单，不需要考虑trie树。

①分治：IP是32位，共有2^32个IP。访问该日的日志，将IP取出来，采用Hash，比如模1000，把所有IP存入1000个小文件。

②Hash_map：统计每个小文件中出现频率最大的IP，记录其频率。

③小顶堆：这里用一个变量即可。在这1000个小文件各自最大频率的IP中，然后直接找出频率最大的IP。

3. 海量数据分布在100台电脑中，想个办法高效统计出这批数据的TOP10。注：主要不同点在于分布式

分析：虽然数据已经是分布的，但是如果直接求各自的Top10然后合并的话，可能忽略一种情况，即有一个数据在每台机器的频率都是第11，但是总数可能属于Top10。所以应该先把100台机器中相同的数据整合到相同的机器，然后再求各自的Top10并合并。

①分治：顺序读每台机器上的数据，按照Hash(x)%100重新分布到100台机器内。接下来变成了单机的topk问题。单台机器内的文件如果太大，可以继续Hash分割成小文件。

②Hash_map：统计每台机器上数据的频率。

③小顶堆：采用容量为10的小顶堆，统计每台机器上的Top10。然后把这100台机器上的TOP10组合起来，共1000个数据，再用小顶堆求出TOP10。

4. 一个文本文件，大约有一万行，每行一个词，要求统计出其中最频繁出现的前10个词，请给出思想，给出时间复杂度分析。 注：文件大小不需要分割文件

①分治：一万行不算多，不用分割文件。

②Trie树：统计每个词出现的次数，时间复杂度是O(n*le)  (le表示单词的平准长度)。

③小顶堆：容量为10的小顶堆，找出词频最多的前10个词，时间复杂度是O(n*lg10)  (lg10表示堆的高度)。

总的时间复杂度是 O(n*le)与O(n*lg10)中较大的那一个。



5. 一个文本文件，找出前10个经常出现的词，但这次文件比较长，说是上亿行或十亿行，总之无法一次读入内存，问最优解。

比上一题多一次分割。分割成可以一次读入内存的大小。

①分治：顺序读文件，将文件Hash分割成小文件，求小文件里的词频。

②③同上。



6. 100w个数中找出最大的100个数。

方法1：用容量为100的小顶堆查找。复杂度为O(100w * lg100)。小根堆是最好的方法。

方法2：采用快速排序的思想，每次分割之后只考虑比标兵值大的那一部分，直到大的部分在比100多且不能分割的时候，采用传统排序算法排序，取前100个。复杂度为O(100w*100)。

方法3：局部淘汰法。取前100个元素并排序，然后依次扫描剩余的元素，插入到排好序的序列中，并淘汰最小值。复杂度为O(100w * lg100)  (lg100为二分查找的复杂度)。



重复问题：
1. 给定a、b两个文件，各存放50亿个url，每个url各占64字节，内存限制是4G，让你找出a、b文件共同的url？

分析：每个文件的大小约为5G×64=320G，远远大于内存大小。考虑采取分而治之的方法。

方法1：

①分治：遍历文件a，对每个url求Hash%1000，根据值将url分别存储到1000个小文件中，每个小文件约为300M。文件b采用同样hash策略分到1000个小文件中。上述两组小文件中，只有相同编号的小文件才可能有相同元素。

②Hash_set：读取a组中一个小文件的url存储到hash_set中，然后遍历b组中相同编号小文件的每个url，查看是否在刚才构建的hash_set中。如果存在，则存到输出文件里。

方法2：

如果允许有一定的错误率，可以使用Bloom filter，使用位数组，4G内存大概可以表示340亿bit。将其中一个文件中的url使用Bloom filter映射为这340亿bit，然后挨个读取另外一个文件的url，检查是否在Bloom filter中。如果是，那么该url应该是共同的url（注意会有一定的错误率）。

注： bloom filter被用来检测一个元素是不是集合中的一个成员。如果检测结果为是，该元素不一定在集合中；但如果检测结果为否，该元素一定不在集合中。主要思路是：将一个元素映射到一个 m 长度的阵列上，使用 k 个哈希 函数对应 k 个点，如果所有点都是 1 的话，那么元素在集合内，如果有 0 的话，元素则不在集合内。 错误率：如何根据输入元素个数n，确定位数组m的大小及hash函数个数k，k=(ln2)*(m/n)时错误率最小，为f = (1 – e-kn/m)k 。



2. 在2.5亿个整数中找出不重复的整数，内存不足以容纳这2.5亿个整数。

分析：2.5亿个整数大概是954MB，也不是很大。当然可以更节省内存。整数一共2^32个数.每个数用2bit的话，需要1GB。也就是

方法1：

采用2-Bitmap，每个数分配2bit，00表示不存在，01表示出现一次，10表示多次，11无意义。共需内存60MB左右。然后扫描这2.5亿个整数，查看Bitmap中相对应位，如果是00变01，01变10，10保持不变。所描完后，查看Bitmap，把对应位是01的整数输出。

注：感觉这个方法不对呀，bitmap要统计所有的整数值，2*3^32是需要1GB内存呀，不是60MB, 954MB都存不下怎么存1GB?? 得到结论，bitmap统计整数存在性起码得有1G的内存。也就是说少于268435456个数不如直接hash，消耗的内存反而更小！

方案2：

分治法，Hash分割成小文件处理。注意hash保证了每个文件中的元素一定不会在其他文件中存在。利用Hash_set，在小文件中找出不重复的整数，再进行归并。

方案3：

或者，我觉得可以将整个整数域划的bitmap根据内存大小分成可以几个文件，比如划分四个文件，这样的话0-1*2^30在一个范围，，……，3*2^30-4*2^30在一个文件中，内存只要保证250M大小即可。整数需要放在对应的bitmap里面的对应位置，这里位置使用的是相对偏移量（value-首元素大小）。跟方案2相比分割的 方法不一样，以及每个小文件可以使用bitmap方法，所以更快一些。只是不知道有没有这种分割。



3. 一个文件包含40亿个整数，找出不包含的一个整数。分别用1GB内存和10MB内存处理。

1GB内存：

①Bitmap：对于32位的整数，共有232个，每个数对应一个bit，共需0.5GB内存。遍历文件，将每个数对应的bit位置1。最后查找0bit位即可。

10MB内存： 10MB = 8 × 107bit

①分治：将所有整数分段，每1M个数对应一个小文件，共4000个小文件。注意计算机能表示的所有整数有4G个。

②Hash_set：对每个小文件，遍历并加入Hash_set，最后如果set的size小于1M，则有不存在的数。利用Bitmap查找该数。

注：计算机能表示的整数个数一共有4G个，整数域hash分割成10M一个文件，，一共分割成400个小文件，每个小文件判断不存在的数，再把这些数全都归并起来。磁盘IO次数越少越好！！所以不明白为啥1M对应一个小文件，而不取最大的10M。



4. 有10亿个URL，每个URL对应一个非常大的网页，怎样检测重复的网页？

分析：不同的URL可能对应相同的网页，所以要对网页求Hash。1G个URL+哈希值，总量为几十G，单机内存无法处理。

①分治：根据Hash%1000，将URL和网页的哈希值分割到1000个小文件中，注意：重复的网页必定在同一个小文件中。

②Hash_set：顺序读取每个文件，将Hash值加入集合，如果已存在则为重复网页。



排序问题：
1. 有10个文件，每个文件1G，每个文件的每一行存放的都是用户的query，每个文件的query都可能重复。要求按照query的频度排序。

方法1：

①分治：顺序读10个文件，按照Hash(query)%10的结果将query写入到另外10个文件。新生成的每个文件大小为1G左右(假设hash函数是随机的)。

②Hash_map：找一台内存为2G左右的机器，用Hash_map(query, query_count)来统计次数。

③内排序：利用快速/堆/归并排序，按照次数进行排序。将排序好的query和对应的query_count输出到文件中，得到10个排好序的文件。

④多路归并：将这10个文件进行归并排序。

方案2：

一般query的总量是有限的，只是重复的次数比较多。对于所有的query，一次性就可能加入到内存。这样就可以采用Trie树/Hash_map等直接统计每个query出现的次数，然后按次数做快速/堆/归并排序就可以了

方案3：

与方案1类似，在做完Hash分割后，将多个文件采用分布式的架构来处理（比如MapReduce），最后再进行合并。



2. 一共有N个机器，每个机器上有N个数。每个机器最多存O(N)个数并对它们操作。如何找到这N^2个数的中位数？

方法1： 32位的整数一共有232个

①分治：把0到232-1的整数划分成N段，每段包含232/N个整数。扫描每个机器上的N个数，把属于第一段的数放到第一个机器上，属于第二段的数放到第二个机器上，依此类推。 (如果有数据扎堆的现象，导致数据规模并未缩小，则继续分割)

②找中位数的机器：依次统计每个机器上数的个数并累加，直到找到第k个机器，加上其次数则累加值大于或等于N2/2，不加则累加值小于N2/2。

③找中位数：设累加值为x，那么中位数排在第k台机器所有数中第N2/2-x位。对这台机器的数排序，并找出第N2/2-x个数，即为所求的中位数。

复杂度是O(N2)。

方法2：

①内排序：先对每台机器上的数进行排序。

②多路归并：将这N台机器上的数归并起来得到最终的排序。找到第N2/2个数即是中位数。

复杂度是O(N2*lgN)。

1. 给定a、b两个文件，各存放50亿个url，每个url各占64字节，内存限制是4G，让你找出a、b文件共同的url？

方案1：可以估计每个文件安的大小为50G×64=320G，远远大于内存限制的4G。所以不可能将其完全加载到内存中处理。考虑采取分而治之的方法。

s 遍历文件a，对每个url求取clip_image002，然后根据所取得的值将url分别存储到1000个小文件（记为clip_image004）中。这样每个小文件的大约为300M。

s 遍历文件b，采取和a相同的方式将url分别存储到1000各小文件（记为clip_image006）。这样处理后，所有可能相同的url都在对应的小文件（clip_image008）中，不对应的小文件不可能有相同的url。然后我们只要求出1000对小文件中相同的url即可。

s 求每对小文件中相同的url时，可以把其中一个小文件的url存储到hash_set中。然后遍历另一个小文件的每个url，看其是否在刚才构建的hash_set中，如果是，那么就是共同的url，存到文件里面就可以了。

方案2：如果允许有一定的错误率，可以使用Bloom filter，4G内存大概可以表示340亿bit。将其中一个文件中的url使用Bloom filter映射为这340亿bit，然后挨个读取另外一个文件的url，检查是否与Bloom filter，如果是，那么该url应该是共同的url（注意会有一定的错误率）。

2. 有10个文件，每个文件1G，每个文件的每一行存放的都是用户的query，每个文件的query都可能重复。要求你按照query的频度排序。

方案1：

s 顺序读取10个文件，按照hash(query)%10的结果将query写入到另外10个文件（记为clip_image010）中。这样新生成的文件每个的大小大约也1G（假设hash函数是随机的）。

s 找一台内存在2G左右的机器，依次对clip_image010[1]用hash_map(query, query_count)来统计每个query出现的次数。利用快速/堆/归并排序按照出现次数进行排序。将排序好的query和对应的query_cout输出到文件中。这样得到了10个排好序的文件（记为clip_image012）。

s 对clip_image012[1]这10个文件进行归并排序（内排序与外排序相结合）。

方案2：

一般query的总量是有限的，只是重复的次数比较多而已，可能对于所有的query，一次性就可以加入到内存了。这样，我们就可以采用trie树/hash_map等直接来统计每个query出现的次数，然后按出现次数做快速/堆/归并排序就可以了。

方案3：

与方案1类似，但在做完hash，分成多个文件后，可以交给多个文件来处理，采用分布式的架构来处理（比如MapReduce），最后再进行合并。

3. 有一个1G大小的一个文件，里面每一行是一个词，词的大小不超过16字节，内存限制大小是1M。返回频数最高的100个词。

方案1：顺序读文件中，对于每个词x，取clip_image014，然后按照该值存到5000个小文件（记为clip_image016）中。这样每个文件大概是200k左右。如果其中的有的文件超过了1M大小，还可以按照类似的方法继续往下分，知道分解得到的小文件的大小都不超过1M。对每个小文件，统计每个文件中出现的词以及相应的频率（可以采用trie树/hash_map等），并取出出现频率最大的100个词（可以用含100个结点的最小堆），并把100词及相应的频率存入文件，这样又得到了5000个文件。下一步就是把这5000个文件进行归并（类似与归并排序）的过程了。

4. 海量日志数据，提取出某日访问百度次数最多的那个IP。

方案1：首先是这一天，并且是访问百度的日志中的IP取出来，逐个写入到一个大文件中。注意到IP是32位的，最多有clip_image018个IP。同样可以采用映射的方法，比如模1000，把整个大文件映射为1000个小文件，再找出每个小文中出现频率最大的IP（可以采用hash_map进行频率统计，然后再找出频率最大的几个）及相应的频率。然后再在这1000个最大的IP中，找出那个频率最大的IP，即为所求。

5. 在2.5亿个整数中找出不重复的整数，内存不足以容纳这2.5亿个整数。

方案1：采用2-Bitmap（每个数分配2bit，00表示不存在，01表示出现一次，10表示多次，11无意义）进行，共需内存clip_image020内存，还可以接受。然后扫描这2.5亿个整数，查看Bitmap中相对应位，如果是00变01，01变10，10保持不变。所描完事后，查看bitmap，把对应位是01的整数输出即可。

方案2：也可采用上题类似的方法，进行划分小文件的方法。然后在小文件中找出不重复的整数，并排序。然后再进行归并，注意去除重复的元素。

6. 海量数据分布在100台电脑中，想个办法高校统计出这批数据的TOP10。

方案1：

s 在每台电脑上求出TOP10，可以采用包含10个元素的堆完成（TOP10小，用最大堆，TOP10大，用最小堆）。比如求TOP10大，我们首先取前10个元素调整成最小堆，如果发现，然后扫描后面的数据，并与堆顶元素比较，如果比堆顶元素大，那么用该元素替换堆顶，然后再调整为最小堆。最后堆中的元素就是TOP10大。

s 求出每台电脑上的TOP10后，然后把这100台电脑上的TOP10组合起来，共1000个数据，再利用上面类似的方法求出TOP10就可以了。

7. 怎么在海量数据中找出重复次数最多的一个？

方案1：先做hash，然后求模映射为小文件，求出每个小文件中重复次数最多的一个，并记录重复次数。然后找出上一步求出的数据中重复次数最多的一个就是所求（具体参考前面的题）。

8. 上千万或上亿数据（有重复），统计其中出现次数最多的钱N个数据。

方案1：上千万或上亿的数据，现在的机器的内存应该能存下。所以考虑采用hash_map/搜索二叉树/红黑树等来进行统计次数。然后就是取出前N个出现次数最多的数据了，可以用第6题提到的堆机制完成。

9. 1000万字符串，其中有些是重复的，需要把重复的全部去掉，保留没有重复的字符串。请怎么设计和实现？

方案1：这题用trie树比较合适，hash_map也应该能行。

10. 一个文本文件，大约有一万行，每行一个词，要求统计出其中最频繁出现的前10个词，请给出思想，给出时间复杂度分析。

方案1：这题是考虑时间效率。用trie树统计每个词出现的次数，时间复杂度是O(n*le)（le表示单词的平准长度）。然后是找出出现最频繁的前10个词，可以用堆来实现，前面的题中已经讲到了，时间复杂度是O(n*lg10)。所以总的时间复杂度，是O(n*le)与O(n*lg10)中较大的哪一个。

11. 一个文本文件，找出前10个经常出现的词，但这次文件比较长，说是上亿行或十亿行，总之无法一次读入内存，问最优解。

方案1：首先根据用hash并求模，将文件分解为多个小文件，对于单个文件利用上题的方法求出每个文件件中10个最常出现的词。然后再进行归并处理，找出最终的10个最常出现的词。

12. 100w个数中找出最大的100个数。

方案1：在前面的题中，我们已经提到了，用一个含100个元素的最小堆完成。复杂度为O(100w*lg100)。

方案2：采用快速排序的思想，每次分割之后只考虑比轴大的一部分，知道比轴大的一部分在比100多的时候，采用传统排序算法排序，取前100个。复杂度为O(100w*100)。

方案3：采用局部淘汰法。选取前100个元素，并排序，记为序列L。然后一次扫描剩余的元素x，与排好序的100个元素中最小的元素比，如果比这个最小的要大，那么把这个最小的元素删除，并把x利用插入排序的思想，插入到序列L中。依次循环，知道扫描了所有的元素。复杂度为O(100w*100)。

13. 寻找热门查询：

搜索引擎会通过日志文件把用户每次检索使用的所有检索串都记录下来，每个查询串的长度为1-255字节。假设目前有一千万个记录，这些查询串的重复读比较高，虽然总数是1千万，但是如果去除重复和，不超过3百万个。一个查询串的重复度越高，说明查询它的用户越多，也就越热门。请你统计最热门的10个查询串，要求使用的内存不能超过1G。

(1) 请描述你解决这个问题的思路；

(2) 请给出主要的处理流程，算法，以及算法的复杂度。

方案1：采用trie树，关键字域存该查询串出现的次数，没有出现为0。最后用10个元素的最小推来对出现频率进行排序。

14. 一共有N个机器，每个机器上有N个数。每个机器最多存O(N)个数并对它们操作。如何找到clip_image022个数中的中数？

方案1：先大体估计一下这些数的范围，比如这里假设这些数都是32位无符号整数（共有clip_image018[1]个）。我们把0到clip_image024的整数划分为N个范围段，每个段包含clip_image026个整数。比如，第一个段位0到clip_image028，第二段为clip_image026[1]到clip_image030，…，第N个段为clip_image032到clip_image024[1]。然后，扫描每个机器上的N个数，把属于第一个区段的数放到第一个机器上，属于第二个区段的数放到第二个机器上，…，属于第N个区段的数放到第N个机器上。注意这个过程每个机器上存储的数应该是O(N)的。下面我们依次统计每个机器上数的个数，一次累加，直到找到第k个机器，在该机器上累加的数大于或等于clip_image034，而在第k-1个机器上的累加数小于clip_image034[1]，并把这个数记为x。那么我们要找的中位数在第k个机器中，排在第clip_image036位。然后我们对第k个机器的数排序，并找出第clip_image036[1]个数，即为所求的中位数。复杂度是clip_image038的。

方案2：先对每台机器上的数进行排序。排好序后，我们采用归并排序的思想，将这N个机器上的数归并起来得到最终的排序。找到第clip_image034[2]个便是所求。复杂度是clip_image040的。
