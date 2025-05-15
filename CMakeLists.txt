cmake_minimum_required(VERSION 3.10)

# 设置 vcpkg 工具链文件（如果存在）
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

project(SimpleBlockchain VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 OpenSSL 路径
set(OPENSSL_ROOT_DIR "D:/OpenSSL-Win64")
set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")

# 添加可执行文件
add_executable(blockchain
    main.cpp
    block.cpp
    blockchain.cpp
)

# 直接指定 OpenSSL 库文件
target_include_directories(blockchain PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(blockchain PRIVATE
    "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD/libssl.lib"
    "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD/libcrypto.lib"
)