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



## 余额管理(0.0.5)

余额管理涉及到如何把保存钱包的余额。

这里一波三折：

- 最初AI举例将余额存在钱包，然后试图在交易操作钱包，然而实施失败
- 后续AI将余额管理作为带三方管理，笔者认为失去意义
- 最后AI将每个钱包的余额记入Block中，笔者排除一堆幻觉后调试通过。但遗留几个问题。

==遗留问题：==

1. 钱包中保存余额的部分失效了，钱包自己是否应该知道余额
2. 每次统计钱包余额，要遍历chain上所有块。一个成熟的区块链会这么干么？

您提出了两个非常好的问题！

1. 关于钱包是否应该知道余额：
   - 在真实的区块链系统中，钱包确实应该知道自己的余额
   - 钱包可以维护一个本地缓存，通过监听区块链上的交易来更新余额
   - 这样可以快速查询余额，而不需要每次都遍历整个链

2. 关于余额计算方式：
   - 您说得对，遍历整个链来计算余额是非常低效的
   - 成熟的区块链系统通常使用 UTXO (Unspent Transaction Output) 模型或账户模型
   - 我们当前实现的是账户模型，但实现方式不够优化

让我们改进这个设计：

1. 首先修改 `Wallet` 类，添加余额管理：
```cpp
class Wallet {
public:
    // ... 其他代码 ...
    
    // 余额管理
    double getBalance() const { return balance_; }
    void updateBalance(double amount) { balance_ += amount; }
    
    // 监听交易更新余额
    void processTransaction(const Transaction& tx) {
        if (tx.getFrom() == getPublicKey()) {
            balance_ -= tx.getAmount();
        }
        if (tx.getTo() == getPublicKey()) {
            balance_ += tx.getAmount();
        }
    }

private:
    // ... 其他成员 ...
    double balance_ = 0.0;
};
```

2. 在 `Blockchain` 类中添加余额缓存：
```cpp
class Blockchain {
public:
    // ... 其他代码 ...
    
    // 获取地址余额（使用缓存）
    double getBalance(const std::string& address) const {
        auto it = balanceCache_.find(address);
        if (it != balanceCache_.end()) {
            return it->second;
        }
        return 0.0;
    }
    
    void addBlock(const std::vector<Transaction>& transactions) {
        // 验证所有交易
        for (const auto& tx : transactions) {
            if (!validateTransaction(tx)) {
                throw std::runtime_error("Invalid transaction: insufficient balance or invalid signature");
            }
        }
        
        // 创建新区块
        std::string previousHash = chain_.empty() ? "0" : chain_.back()->getHash();
        auto newBlock = std::make_shared<Block>(chain_.size(), transactions, previousHash);
        
        // 更新余额缓存
        for (const auto& tx : transactions) {
            balanceCache_[tx.getFrom()] -= tx.getAmount();
            balanceCache_[tx.getTo()] += tx.getAmount();
        }
        
        // 挖矿
        newBlock->mineBlock(difficulty_);
        
        // 添加区块
        chain_.push_back(newBlock);
        
        // 通知钱包更新余额
        for (const auto& tx : transactions) {
            auto fromWallet = getWalletByPublicKey(tx.getFrom());
            if (fromWallet) {
                fromWallet->processTransaction(tx);
            }
            auto toWallet = getWalletByPublicKey(tx.getTo());
            if (toWallet) {
                toWallet->processTransaction(tx);
            }
        }
    }

private:
    // ... 其他成员 ...
    std::map<std::string, double> balanceCache_;  // 余额缓存
};
```

3. 修改 `main.cpp` 中的代码：
```cpp
int main() {
    try {
        // ... 创建钱包和生成密钥对的代码 ...
        
        // 创建区块链
        Blockchain blockchain(4);
        
        // 设置初始余额
        blockchain.addBlock({
            Transaction("SYSTEM", alicePublicKey, 100.0),
            Transaction("SYSTEM", bobPublicKey, 50.0),
            Transaction("SYSTEM", charliePublicKey, 25.0)
        });
        
        // 打印初始余额
        std::cout << "\nInitial balances:" << std::endl;
        std::cout << "Alice: " << aliceWallet.getBalance() << std::endl;
        std::cout << "Bob: " << bobWallet.getBalance() << std::endl;
        std::cout << "Charlie: " << charlieWallet.getBalance() << std::endl;
        
        // ... 其他代码 ...
    }
    // ... 其他代码 ...
}
```

这样改进后：
1. 钱包维护自己的余额，可以快速查询
2. 区块链使用余额缓存，避免遍历整个链
3. 当新区块被添加时，钱包会收到通知并更新余额
4. 余额的验证仍然由区块链负责，保证安全性
5. 钱包的余额作为本地缓存，可以快速响应查询

这种设计更接近真实的区块链系统：
- 钱包负责管理自己的余额
- 区块链负责验证和记录交易
- 使用缓存提高性能
- 保持数据一致性

您觉得这个改进方案如何？



==笔者：需要补充钱包注册==

我已经完成了以下修改：

1. 在 `Blockchain` 类中添加了钱包管理功能：
   - 添加了 `wallets_` 映射来存储钱包
   - 添加了 `registerWallet` 和 `getWalletByPublicKey` 函数

2. 在 `blockchain.cpp` 中实现了这些新函数：
   - `registerWallet` 用于注册钱包到区块链
   - `getWalletByPublicKey` 用于通过公钥查找钱包

3. 修改了 `main.cpp`：
   - 将钱包对象改为使用智能指针
   - 添加了初始余额映射
   - 注册钱包到区块链
   - 更新了所有钱包相关的调用

这些修改应该解决了 `getWalletByPublicKey` 未定义的问题。现在区块链系统可以正确地管理钱包和余额了。

您还需要我帮您做其他修改吗？

==为什么创建区块链时添加了初始余额映射，注册钱包后，又设置初始余额==

您说得对，这里确实存在重复设置初始余额的问题。让我解释一下当前的问题并提供修复方案：

当前代码中存在两个地方设置初始余额：

1. 在创建区块链时通过 `initialBalances` 参数设置：
```cpp
std::map<std::string, double> initialBalances = {
    {alicePublicKey, 100.0},
    {bobPublicKey, 50.0},
    {charliePublicKey, 25.0}
};
Blockchain blockchain(4, initialBalances);
```

2. 在创建创世区块时又通过交易设置：
```cpp
blockchain.addBlock({
    Transaction("SYSTEM", alicePublicKey, 100.0),
    Transaction("SYSTEM", bobPublicKey, 50.0),
    Transaction("SYSTEM", charliePublicKey, 25.0)
});
```

这确实是不必要的重复。我们应该只保留一种方式来设置初始余额。

现在我们还需要修改 `Blockchain` 类的构造函数，移除 `initialBalances` 参数，

最后，我们需要修改 `blockchain.cpp` 中的构造函数实现：

我已经完成了以下修改：

1. 移除了 `Blockchain` 构造函数中的 `initialBalances` 参数
2. 移除了在创建区块链时通过映射设置初始余额的方式
3. 保留了通过创世区块交易设置初始余额的方式

这样修改的原因是：
1. 通过交易设置初始余额更符合区块链的设计理念，因为所有状态变化都应该通过交易来记录
2. 避免了重复设置余额的问题
3. 保持了代码的一致性和清晰性

现在初始余额只通过创世区块的交易来设置，这样更合理且避免了重复。您觉得这样的修改合适吗？

==设置初始余额的时候没有对交易签名==

您说得对！这是一个很好的发现。在设置初始余额时，我们使用了 `"SYSTEM"` 作为发送方，但没有对交易进行签名。这违反了区块链的基本规则，即所有交易都需要有效的签名。

让我们修改代码，为系统交易添加一个特殊的签名机制：

我已经完成了以下修改：

1. 在 `Transaction` 类中添加了 `createSystemTransaction` 静态方法，用于创建系统交易
2. 修改了 `verifySignature` 方法，使其能够验证系统交易的特殊签名
3. 更新了 `main.cpp` 中的初始余额设置，使用 `createSystemTransaction` 方法

这些修改的好处是：
1. 系统交易有了明确的标识和验证机制
2. 保持了交易验证的一致性
3. 使代码更加清晰和可维护

系统交易使用特殊的签名 `"SYSTEM_SIGNATURE"`，这是一个简化的实现。在实际的区块链系统中，系统交易通常会有更复杂的验证机制，比如：
1. 使用特殊的系统密钥对
2. 使用多重签名
3. 使用共识机制验证

您觉得这样的修改合适吗？

我已经修复了循环依赖的问题：

1. 在 wallet.h 中：

- 移除了 #include "transaction.h"

- 添加了 Transaction 类的前向声明

- 将 processTransaction 方法的实现移到 cpp 文件中

1. 在 wallet.cpp 中：

- 添加了 #include "transaction.h"

- 实现了 processTransaction 方法

这样修改的好处是：

1. 解决了循环依赖问题

1. 遵循了 C++ 的最佳实践，将实现细节放在 cpp 文件中

1. 保持了代码的功能不变

现在代码应该可以正常编译了。您还需要我做其他修改吗？



==validateTransaction校验余额时，系统钱包不应该校验==

您说得对！系统交易（`"SYSTEM"` 作为发送方）不应该进行余额校验，因为系统交易是用来创建初始余额的。让我修改 `Blockchain` 类中的 `validateTransaction` 方法：



我已经修改了 `validateTransaction` 方法，添加了对系统交易的特殊处理：

1. 首先验证交易签名
2. 如果是系统交易（`from_ == "SYSTEM"`），直接返回 `true`，跳过余额检查
3. 对于普通交易，继续执行余额检查

这样修改的好处是：
1. 系统交易可以不受余额限制，用于创建初始余额
2. 普通交易仍然需要验证余额，确保交易有效
3. 保持了代码的清晰性和可维护性

这个修改更符合区块链系统的设计，因为：
1. 系统交易是特殊的，用于初始化系统状态
2. 普通交易必须遵守余额规则
3. 系统交易仍然需要验证签名，确保其合法性

## 交易池和UTXO

我来帮您实现交易池(Transaction Pool)和UTXO(Unspent Transaction Output)模型。这是区块链系统中非常重要的两个组件。

主要的变更：

1. UTXO模型：
- 创建了`UTXO`类来表示未花费的交易输出
- 实现了`UTXOPool`类来管理所有UTXO
- 提供了查询余额、选择UTXO等功能

2. 交易池：
- 创建了`TransactionPool`类来管理待处理的交易
- 实现了交易的验证、添加、移除等功能
- 使用互斥锁保证线程安全

3. 交易结构更新：
- 添加了`TransactionInput`和`TransactionOutput`类
- 更新了`Transaction`类以支持UTXO模型
- 实现了更安全的交易验证机制

4. 区块链更新：
- 集成了UTXO池和交易池
- 更新了区块添加逻辑，支持UTXO模型
- 实现了余额查询的新方法

主要改进：
1. 更好的安全性：UTXO模型可以防止双重支付
2. 更高的效率：交易池可以缓存待处理交易
3. 更准确的余额计算：基于UTXO而不是简单的加减
4. 更好的并发支持：使用互斥锁保护共享资源

使用示例：
```cpp
// 创建交易
Transaction tx(fromAddress, toAddress, amount);

// 选择UTXO
auto utxos = utxoPool.selectUTXOs(fromAddress, amount);

// 添加输入
for (const auto& utxo : utxos) {
    tx.addInput(TransactionInput(utxo.getTxId(), utxo.getOutputIndex(), signature));
}

// 添加输出
tx.addOutput(TransactionOutput(amount, toAddress));
if (totalInput > amount) {
    tx.addOutput(TransactionOutput(totalInput - amount, fromAddress)); // 找零
}

// 添加到交易池
blockchain.addTransactionToPool(tx);
```



UTXO和交易池的概念和实现。

### UTXO (Unspent Transaction Output) 模型

UTXO是比特币等区块链系统中使用的一种交易模型。它的核心思想是：

1. **基本概念**：
- 每个交易输出(UTXO)代表一定数量的加密货币
- 每个UTXO只能被使用一次
- 交易必须使用已有的UTXO作为输入，并创建新的UTXO作为输出

2. **UTXO类的实现**：
```cpp
class UTXO {
    std::string txId_;        // 交易ID
    int outputIndex_;         // 输出索引
    double amount_;           // 金额
    std::string owner_;       // 所有者地址
    bool spent_;             // 是否已花费
};
```

3. **UTXOPool的功能**：
- 管理所有未花费的交易输出
- 提供余额查询
- 选择UTXO进行交易
```cpp
class UTXOPool {
    // 存储结构：交易ID -> (输出索引 -> UTXO)
    std::map<std::string, std::map<int, UTXO>> utxos_;
    
    // 主要方法
    void addUTXO(const UTXO& utxo);                    // 添加UTXO
    void removeUTXO(const std::string& txId, int outputIndex);  // 移除UTXO
    double getBalance(const std::string& address);      // 查询余额
    std::vector<UTXO> selectUTXOs(const std::string& address, double amount);  // 选择UTXO
};
```

### 交易池 (Transaction Pool)

交易池是一个临时存储待处理交易的容器，主要功能包括：

1. **基本概念**：
- 存储等待被打包进区块的交易
- 提供交易验证功能
- 管理交易的添加和移除

2. **TransactionPool的实现**：
```cpp
class TransactionPool {
    std::map<std::string, Transaction> transactions_;  // 存储交易
    mutable std::mutex mutex_;                         // 线程安全锁
    
    // 主要方法
    bool addTransaction(const Transaction& transaction, const UTXOPool& utxoPool);
    void removeTransaction(const std::string& txId);
    std::vector<Transaction> getTransactions() const;
};
```

3. **交易验证流程**：
```cpp
bool TransactionPool::isValidTransaction(const Transaction& transaction, const UTXOPool& utxoPool) const {
    // 验证签名
    if (!transaction.verifySignature()) return false;
    
    // 验证余额
    if (!utxoPool.hasEnoughFunds(transaction.getFrom(), transaction.getAmount())) return false;
    
    // 验证金额
    if (transaction.getAmount() <= 0) return false;
    
    // 验证发送者和接收者
    if (transaction.getFrom() == transaction.getTo()) return false;
    
    return true;
}
```

### 工作流程

1. **创建交易**：
```cpp
// 1. 选择UTXO
auto utxos = utxoPool.selectUTXOs(fromAddress, amount);

// 2. 创建交易输入
for (const auto& utxo : utxos) {
    tx.addInput(TransactionInput(utxo.getTxId(), utxo.getOutputIndex(), signature));
}

// 3. 创建交易输出
tx.addOutput(TransactionOutput(amount, toAddress));
// 如果有找零
if (totalInput > amount) {
    tx.addOutput(TransactionOutput(totalInput - amount, fromAddress));
}
```

2. **交易处理**：
```cpp
// 1. 添加到交易池
blockchain.addTransactionToPool(tx);

// 2. 挖矿时从交易池获取交易
auto pendingTransactions = transactionPool.getTransactions();

// 3. 交易确认后更新UTXO池
void Blockchain::updateUTXOPool(const Block& block) {
    for (const auto& tx : block.getTransactions()) {
        // 移除已使用的UTXO
        for (const auto& input : tx.getInputs()) {
            utxoPool_.removeUTXO(input.getTxId(), input.getOutputIndex());
        }
        
        // 添加新的UTXO
        for (size_t i = 0; i < tx.getOutputs().size(); ++i) {
            const auto& output = tx.getOutputs()[i];
            UTXO utxo(tx.getTransactionId(), i, output.getAmount(), output.getOwner());
            utxoPool_.addUTXO(utxo);
        }
    }
}
```

这种设计的优点：
1. 防止双重支付
2. 提高交易验证效率
3. 支持并行处理
4. 更好的隐私保护
5. 更准确的余额计算

主程序更新：

1. 交易创建流程：

- 现在每个交易都需要选择UTXO作为输入

- 添加了交易输入和输出的处理

- 实现了找零机制

1. 交易池集成：

- 交易先添加到交易池

- 挖矿时从交易池获取待处理交易

- 交易确认后从交易池中移除

1. 余额查询：

- 使用UTXO池来查询余额

- 不再使用钱包的余额缓存

1. 输出信息：

- 添加了交易输入和输出的详细信息

- 显示UTXO的使用情况

- 显示找零信息

1. 本地化：

- 将所有输出信息改为中文

- 优化了输出格式

主要改进：

1. 更安全的交易机制

1. 更准确的余额计算

1. 更好的交易追踪

1. 更清晰的输出信息



# 遗留

用 Solidity 快速部署一个代币（ERC20 或 ERC721），或者给你一个最简智能合约“发币模板”。
