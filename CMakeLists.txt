cmake_minimum_required(VERSION 3.10)

# Set vcpkg toolchain file (if exists)
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

project(BlockChain)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set OpenSSL paths
set(OPENSSL_ROOT_DIR "D:/OpenSSL-Win64")
set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
# set(OPENSSL_SSL_LIBRARY "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD/libssl.lib")
# set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD/libcrypto.lib")

# Find OpenSSL package
find_package(OpenSSL REQUIRED)

# Find Boost package
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.88.0 REQUIRED COMPONENTS system)

# Add nlohmann/json
if(DEFINED ENV{NLOHMANN_JSON_INCLUDE_DIR})
    include_directories($ENV{NLOHMANN_JSON_INCLUDE_DIR})
else()
    message(FATAL_ERROR "Please set NLOHMANN_JSON_INCLUDE_DIR environment variable")
endif()

# Add executable
add_executable(BlockChain
    main.cpp
    blockchain.cpp
    block.cpp
    transaction.cpp
    wallet.cpp
    utxo.cpp
    transactionpool.cpp
    merkletree.cpp
    p2p_node.cpp
)

# Include directories
target_include_directories(BlockChain PRIVATE
    ${OPENSSL_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
)

# Link OpenSSL libraries
target_link_libraries(BlockChain PRIVATE
    ${OPENSSL_LIBRARIES}
    Boost::system
)

# 直接指定 OpenSSL 库文件
# target_include_directories(BlockChain PRIVATE ${OPENSSL_INCLUDE_DIR})
# target_link_libraries(BlockChain PRIVATE
#     "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD/libssl.lib"
#     "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD/libcrypto.lib"
# )