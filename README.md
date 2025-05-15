# BlockChain

## 区块和链（v0.0.1）

创建了一个完整的简化区块链系统。这个系统包含以下主要特性：

1. **区块结构（Block 类）**：
   - 包含索引、时间戳、数据、前一个区块的哈希值
   - 实现了工作量证明（Proof-of-Work）机制
   - 使用 OpenSSL 进行 SHA-256 哈希计算

2. **区块链结构（Blockchain 类）**：
   - 管理区块链
   - 提供添加新区块的功能
   - 验证整个链的有效性

3. **主要功能**：
   - 工作量证明（通过调整难度值）
   - 区块验证
   - 链验证
   - 区块创建

要编译和运行这个项目，您需要：

1. 安装 OpenSSL 开发库
2. 使用 CMake 构建项目：
```bash
mkdir build
cd build
cmake -G "Visual Studio 15 2017" -A x64 ..
cmake --build . --config Release
```

3. 运行程序：
```bash
./blockchain
```

这个实现使用了现代 C++ 特性：
- C++17 标准
- 智能指针（`std::shared_ptr`）
- 现代时间处理（`std::chrono`）
- 字符串流处理
- 内存安全的设计

系统的工作流程是：
1. 创建区块链实例（默认难度为4）
2. 自动创建创建区块
3. 添加新区块时会自动进行挖矿（工作量证明）
4. 每个区块都包含前一个区块的哈希值，确保链的完整性
5. 可以随时验证整个链的有效性

您可以通过调整 `difficulty` 参数来改变挖矿难度。难度值越高，挖矿所需的时间就越长。

需要注意的是，这是一个简化的实现，包括:

- 区块的创建和链接
- 工作量证明（挖矿）机制
- 区块链的验证
- 数据的不可篡改性（通过哈希链接）

实际的区块链系统还需要考虑：

- 网络通信
- 共识机制
- 交易验证
- 钱包管理
- 更复杂的数据结构



## 交易结构和Merkle树（v0.0.2）

### Transaction

交易列表，一般是一个Transaction列表。

交易结构 (Transaction)

- 包含发送者、接收者、金额、时间戳和交易ID

- 交易ID通过哈希交易数据生成

- 提供了完整的 getter 方法

### Merkle 树

Merkle 树 (MerkleTree)

- 实现了基本的 Merkle 树结构

- 支持交易验证

- 自动处理奇数个叶子节点的情况

- 提供了根哈希获取方法



笔者注：Merkle树的实现这里其实仅仅临时对Transaction列表做了聚合。在其内部完成：

- 将所有Transaction提取hash值组成列表
- 然后按照双数（奇数最后一个重复）每2个聚合哈希成一个
- 递归汇总成一个hash值后，使用该hash值生成根MerkleNode（所以leaves_永远为空）
- Block在初始化的时候读取这个根MerkleNode的hash值

### 区块更新 (Block)

- 将原来的简单数据字段改为交易列表

- 添加了 Merkle 根哈希

- 更新了哈希计算方法以包含 Merkle 根

笔者注：Block在初始化时，入参简单data更新为交易列表（Transaction），构造时将该列表生成一个MerkleTree，但其实该树仅仅用于提取merkleRoot_，即对所有交易的hash值汇总后再hash。

另外区块保存了交易列表

### 主程序更新 (main.cpp)

- 创建示例交易

- 展示交易和 Merkle 树的使用

- 显示更详细的区块信息



主要修改包括：

1. 更新了 Blockchain 类以支持交易列表
1. 更新了 Block 类以包含交易和 Merkle 树
1. 添加了所有必要的源文件到 CMakeLists.txt



这是第一个不完善的交易结构和Merkle树演示。需笔者手动修正。

## MerkleTree完善(v0.0.3)

主要改进包括：

1. 完整的树形结构：

- 添加了节点层级（level）属性

- 实现了完整的二叉树结构

- 添加了树的可视化打印功能

1. 交易验证机制：

- 实现了 verifyTransaction 方法

- 添加了验证路径（proof path）的构建和存储

- 支持通过路径验证交易的有效性

1. 数据结构优化：

- 使用 unordered_map 存储叶子节点和验证路径

- 添加了节点类型判断（叶子节点/内部节点）

1. 调试功能：

- 添加了 printTree 方法用于可视化树的结构

- 支持按层级打印节点信息

现在，Merkle 树的实现更加完整和健壮：

1. 可以验证交易是否存在于树中

1. 可以追踪交易到根节点的路径

1. 支持树的层级管理

1. 提供了调试和可视化功能

### Block

Block中增加了merkleRoot_字段，其通过初始化时构造MerkleTree然后提取根节点的HASH值获得。

merkleRoot_可用于在后续验证交易是否被篡改。

### MerkleTree

#### MerkleNode

假设我们有4个交易（A、B、C、D），构建的默克尔树如下：

```
        Root
       /    \
      /      \
    AB        CD
   /  \      /  \
  A    B    C    D
```

相邻节点一起构造了一个父节点，其Hash值为2个子节点拼接后再次hash出的值。如果叶节点为奇数，则最后一个叶节点重复一次。

```
MerkleTree::printTree
  ABCC  Level 2: 779cc102d995f0cb01c420eddc2d9a2154a992cc4d93a113e18ab34e001850a2
    AB  Level 1: e5d5262bd00f3ab8dc254c5ed2d4a8b9a3793d3fce38e8e8a693ad451218033f
      A  Level 0: 43caa04a00f424b1911f5fff8ee452c32e2bc945ae8a88a05183021d27a69446
      B  Level 0: 7cb17ad411d33feb075223d142e3cb631afd52370cfd1961c34dc4d300978a3a
    CC  Level 1: 37f004022cc1769ebc8214ed4a5e17b387935f308bb6b81451b2fe0f0b6193d1
      C  Level 0: 97fd3316a49190b85a5b9305b28854d30ac937e551f759d45b56319d82b777e3
      C  Level 0: 97fd3316a49190b85a5b9305b28854d30ac937e551f759d45b56319d82b777e3
```

### 

#### ProofPaths

proofPaths 表为每个叶节点构造一个最快构建出根节点的相邻节点hash列表。即顺着列表逐个拼接HASH值并hash出父节点HASH值。

```
MerkleTree::MerkleTree
  MerkleNode::MerkleNode A 
  MerkleNode::MerkleNode B 
  MerkleNode::MerkleNode C 
  MerkleTree::buildTree
  MerkleNode::MerkleNode AB 
  MerkleNode::MerkleNode CC 
  MerkleTree::buildTree
  MerkleNode::MerkleNode ABCC 
  
  buildProofPaths leftPath CC true
    buildProofPaths leftPath B true
      buildProofPaths leaf A 
    buildProofPaths rightPath A false
      buildProofPaths leaf B 
  buildProofPaths rightPath AB false
    buildProofPaths leftPath C true
      buildProofPaths leaf C 
    buildProofPaths rightPath C false
      buildProofPaths leaf C 
```



##### `buildProofPaths` 的逻辑。

默克尔树的证明路径（Merkle Proof）的目的是：证明某个交易确实存在于区块链中。为了做到这一点，我们需要提供从该交易到根节点的路径上所有兄弟节点的哈希值。

让我们通过一个具体的例子来说明：

假设我们有4个交易（A、B、C、D），构建的默克尔树如下：

```
        Root
       /    \
      /      \
    AB        CD
   /  \      /  \
  A    B    C    D
```

现在，如果我们想证明交易 A 确实存在于树中，我们需要提供：

1. B 的哈希值（A 的兄弟节点）
2. CD 的哈希值（AB 的兄弟节点）

这样，验证者可以：

1. 用 A 的哈希值和 B 的哈希值计算出 AB
2. 用 AB 的哈希值和 CD 的哈希值计算出 Root
3. 将计算出的 Root 哈希值与实际的 Root 哈希值比较

所以，在 `buildProofPaths` 中：

```cpp
// 为左子节点构建路径
if (node->getRight()) {
    // 如果当前节点是左子节点，需要记录右兄弟节点的哈希值
    leftPath.push_back({node->getRight()->getHash(), true});
}
buildProofPaths(node->getLeft(), leftPath);
```

这段代码的意思是：

- 当我们在左子节点时，我们需要记录右兄弟节点的哈希值，因为验证时需要用它来计算父节点的哈希值
- `true` 表示这个哈希值应该放在左边（在验证时，我们需要用 `siblingHash + currentHash` 的方式组合）

同理：

```cpp
// 为右子节点构建路径
if (node->getLeft()) {
    // 如果当前节点是右子节点，需要记录左兄弟节点的哈希值
    rightPath.push_back({node->getLeft()->getHash(), false});
}
buildProofPaths(node->getRight(), rightPath);
```

这段代码的意思是：

- 当我们在右子节点时，我们需要记录左兄弟节点的哈希值
- `false` 表示这个哈希值应该放在右边（在验证时，我们需要用 `currentHash + siblingHash` 的方式组合）

在验证时（`verifyPath` 函数）：

```cpp
for (const auto& [siblingHash, isLeft] : path) {
    if (isLeft) {
        // 兄弟节点在左边，当前哈希在右边
        currentHash = calculateHash(siblingHash + currentHash);
    } else {
        // 兄弟节点在右边，当前哈希在左边
        currentHash = calculateHash(currentHash + siblingHash);
    }
}
```

这样，通过记录兄弟节点的哈希值和位置信息，我们可以：

1. 正确重建从交易到根节点的路径
2. 验证交易是否真的存在于区块链中
3. 保证数据的完整性和不可篡改性

这就是为什么在构建路径时，我们要记录兄弟节点的哈希值而不是当前节点的哈希值。因为验证时需要的是兄弟节点的哈希值来重建父节点的哈希值。

##### verifyPath逻辑

在 `verifyPath` 函数中，我们应该倒序访问路径，因为我们需要从叶子节点开始，一步步向上构建到根节点。让我修改这部分代码

1. 原来的代码（正序访问）：

```cpp
for (const auto& [siblingHash, isLeft] : path) {
    // 从路径的第一个元素开始访问
    // 这样会导致哈希值的组合顺序错误
}
```

2. 修改后的代码（倒序访问）：

```cpp
for (auto it = path.rbegin(); it != path.rend(); ++it) {
    // 从路径的最后一个元素开始访问
    // 这样可以从叶子节点开始，一步步向上构建到根节点
}
```

为什么需要倒序访问？

1. 路径的构建顺序：
   - 在 `buildProofPaths` 中，我们是从根节点开始，向下构建路径
   - 路径中的元素顺序是：从根节点到叶子节点的顺序

2. 验证时的需求：
   - 我们需要从叶子节点开始，一步步向上构建到根节点
   - 每一步都需要用当前哈希值和兄弟节点的哈希值计算父节点的哈希值

3. 举个例子：

```
        Root
       /    \
      /      \
    AB        CD
   /  \      /  \
  A    B    C    D
```

如果要验证交易 A：

- 路径中存储的顺序是：[B的哈希值, CD的哈希值]
- 验证时需要：
  1. 先用 A 和 B 的哈希值计算 AB
  2. 再用 AB 和 CD 的哈希值计算 Root
- 所以需要倒序访问路径，先处理 CD 的哈希值，再处理 B 的哈希值

##



## 钱包系统 / 数字签名(v0.0.4)

添加钱包系统，包括密钥生成、签名和验证功能。使用 OpenSSL 的椭圆曲线加密（ECDSA）来实现。

这个实现添加了以下功能：

1. **钱包系统**：
   - 使用 OpenSSL 的椭圆曲线加密（ECDSA）
   - 支持密钥对生成
   - 支持交易签名和验证
   - 使用 secp256k1 曲线（与比特币相同）

2. **交易签名**：
   - 每个交易都可以被签名
   - 签名使用发送方的私钥
   - 任何人都可以使用发送方的公钥验证签名

3. **安全性**：
   - 私钥永远不会离开钱包
   - 所有敏感操作都有适当的错误处理
   - 使用 OpenSSL 的安全随机数生成

4. **主程序：**
   1. 创建三个钱包（Alice、Bob 和 Charlie）
   2. 为每个钱包生成密钥对
   3. 创建并签名交易
   4. 验证交易签名
   5. 创建区块链并添加区块
   6. 验证区块链的有效性
   7. 打印详细的区块链信息，包括：
      - 区块基本信息（索引、时间戳、哈希等）
      - 交易详情
      - Merkle 树结构
      - 交易验证结果

### 钱包

- 钱包其实是相对于用户，而每笔交易对应着2个钱包之间。
- 钱包生成时核心数拥有自己的私钥和公钥，私钥永不对外暴露，而公钥则暴露给交易
- 钱包可对自己作为发起方的交易签名，签名时使用私钥加密

### 交易

- 交易保存交易双方的公钥（替代交易双方的HASH值）
- 交易由发起方的钱包对交易的HASH值签名，并将签名保存在交易内
- 交易可通过发起方的公钥对签名解码，解码后是否和交易HASH值一致判断交易是否签名合法



