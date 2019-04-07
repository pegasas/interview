# Ethernum

## 1 Ethernum P2P

以太坊底层分布式网络即P2P网络，使用了经典的Kademlia网络，简称kad。

Kademlia是一种分布式散列表(DHT)技术，以异或运算为距离度量基础

Node ID 在 P2P 网络中， 节点是通过唯一 ID 来进行标识的，在原始的 Kad 算法中，使用 160-bit 哈希空间来作为 Node ID。

Node Distance 每个节点保存着自己附近（nearest）节点的信息，但是在 Kademlia 中，这个距离不是指物理距离，而是指一种逻辑距离，通过运算得知。

XOR 异或运算 XOR 是一种位运算，用于计算两个节点之间距离的远近。把两个节点的 Node ID 进行 XOR 运算，XOR 的结果越小，表示距离越近。

K-Bucket 用一个 Bucket 来保存与当前节点距离在某个范围内的所有节点列表，比如 bucket0, bucket1, bucket2 ... bucketN 分别记录[1, 2), [2, 4), [4, 8), ... [2^i, 2^(i+1)) 范围内的节点列表
Bucket 分裂 如果初始 bucket 数量不够，则需要分裂（具体跟实现有关）

Update 在节点 bootstrap 过程中，需要把连接上的 peer 节点更新到自己的 Routing table 中对应的 bucket 中

LookUp 查找目标节点，找到与目标节点最近（nearest/closest）的 bucket，如果已在 bucket 中，则直接返回，否则向该 bucket 中的节点发送查询请求，这些节点继续迭代查找

收敛 & 时间复杂度 查找会最终收敛至目标节点，整个查询过程复杂度是 Log N

1、Node ID

Ethereum 使用 sha3，也是 256 位哈希空间， 32 字节。

2、Node Distance 与 XOR

直接对两个 Node ID 进行 XOR 运算，就可以得出他们之间的距离。

Kademlia 中，根据当前节点的 Node ID 与它保存的其他 peer 节点 Node ID 的匹配的最多的前缀 bit 个数来构建一颗二叉树（Binary Tree），

这里前缀匹配的 bit 数也叫 LCP，Longest Common Prefix，Kademlia 中根据 LCP 来划分子树。当前节点的每个 LCP 都是一个子树。

所以，对于一个 160 bit 空间的 Node ID 来说，一共会有 160 个子树，也就是 160 个 buckets。每个 bucket 可以通过 XOR 的结果来索引。

Kad 协议要求每个节点知道其各子树的至少一个节点，只要这些子树非空。在这个前提下，每个节点都可以通过 ID 值来找到任何一个节点。

3、K-Bucket & Routing Table

每个子树就是一个 K-Bucket，而且，子树中也包含许多 leaf 节点，每个 leaf 节点都表示一个 Peer。

Kademlia 中把子树中包含的 leaf 节点数量被设置为最多 k 个。这样可以有效控制整棵树的膨胀。

routing table 使用 K-Bucket list 来保存上述信息。

4、K-Bucket 更新

由于在真实的分布式网络中，由于网络的波动等因素，节点可能是频繁加入和退出网络的，而 kBucket 中保存的是相对静态的信息，因此需要随着一些条件的变化来进行相应的更新，最典型的需要更新 kbucket 的场景就是，当连上一个新的节点，或者有查询原本不在 kbucket 中的节点时。

kademlia 通过记录 kbucket 中每个节点的最近访问时间戳来判断节点的活跃度。

当需要更新一个 kbucket，时 取决于两个因素：

kbucket 未满，则直接添加
kbucket 已满，则判断是否存在剔除失效节点，存在，则用新节点替换，不存在，则抛弃新节点。

## 2 状态树、交易树、收据树

以太坊区块链中每个区块头都包含指向三个树的指针：状态树、交易树、收据树。

状态树代表访问区块后的整个状态；

交易树代表区块中所有交易，这些交易由index索引作为key；（例如，k0:第一个执行的交易，k1：第二个执行的交易）

收据树代表每笔交易相应的收据。

### 2.1 Patricia树

Patricia树，或称Patricia trie，或crit bit tree，压缩前缀树，是一种更节省空间的Trie。对于基数树的每个节点，如果该节点是唯一的儿子的话，就和父节点合并。

### 2.2 Merkle Tree

Merkle Tree，通常也被称作Hash Tree，顾名思义，就是存储hash值的一棵树。Merkle树的叶子是数据块(例如，文件或者文件的集合)的hash值。非叶节点是其对应子节点串联字符串的hash。

### 2.3 MPT（Merkle Patricia Tree）

知道了Merkle Tree，知道了Patricia Tree，顾名思义，MPT（Merkle Patricia Tree）就是这两者混合后的产物。

 在以太坊（ethereum）中，使用了一种特殊的十六进制前缀(hex-prefix, HP)编码，所以在字母表中就有16个字符。这其中的一个字符为一个nibble。

 

MPT树中的节点包括空节点、叶子节点、扩展节点和分支节点:

空节点，简单的表示空，在代码中是一个空串。

叶子节点（leaf），表示为[key,value]的一个键值对，其中key是key的一种特殊十六进制编码，value是value的RLP编码。

扩展节点（extension），也是[key，value]的一个键值对，但是这里的value是其他节点的hash值，这个hash可以被用来查询数据库中的节点。也就是说通过hash链接到其他节点。

分支节点（branch），因为MPT树中的key被编码成一种特殊的16进制的表示，再加上最后的value，所以分支节点是一个长度为17的list，前16个元素对应着key中的16个可能的十六进制字符，如果有一个[key,value]对在这个分支节点终止，最后一个元素代表一个值，即分支节点既可以搜索路径的终止也可以是路径的中间节点。
