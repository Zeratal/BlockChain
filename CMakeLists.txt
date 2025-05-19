cmake_minimum_required(VERSION 3.10)

# Set vcpkg toolchain file (if exists)
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

project(SimpleBlockchain VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set OpenSSL paths
set(OPENSSL_ROOT_DIR "D:/OpenSSL-Win64")
set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
# set(OPENSSL_SSL_LIBRARY "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD/libssl.lib")
# set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD/libcrypto.lib")

# # Find OpenSSL package
# find_package(OpenSSL REQUIRED)

# Add executable
add_executable(blockchain
    main.cpp
    block.cpp
    blockchain.cpp
    transaction.cpp
    merkletree.cpp
    wallet.cpp
    utxo.cpp
    transactionpool.cpp
)

# Include directories
target_include_directories(blockchain PRIVATE 
    ${OPENSSL_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link OpenSSL libraries
# target_link_libraries(blockchain
#     PRIVATE
#     OpenSSL::SSL
#     OpenSSL::Crypto
# )

# 直接指定 OpenSSL 库文件
target_include_directories(blockchain PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(blockchain PRIVATE
    "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD/libssl.lib"
    "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD/libcrypto.lib"
)