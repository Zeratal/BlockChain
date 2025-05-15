# BlockChain

## 区块和链

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
2. 自动创建创世区块
3. 添加新区块时会自动进行挖矿（工作量证明）
4. 每个区块都包含前一个区块的哈希值，确保链的完整性
5. 可以随时验证整个链的有效性

您可以通过调整 `difficulty` 参数来改变挖矿难度。难度值越高，挖矿所需的时间就越长。

需要注意的是，这是一个简化的实现，实际的区块链系统还需要考虑：
- 网络通信
- 共识机制
- 交易验证
- 钱包管理
- 更复杂的数据结构

您想要我详细解释代码的某个特定部分，或者需要添加其他功能吗？



## 交易结构和Merkle树

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
