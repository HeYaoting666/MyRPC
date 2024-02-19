cmake_minimum_required(VERSION 3.16)

# 工程名
project(MyRPC)

set(CMAKE_CXX_STANDARD 11)

# 设置库目录
link_directories(/usr/lib)

# 查找指定目录下的所有源文件
aux_source_directory(./rocket/common COMMON_DIR_SRCS)
aux_source_directory(./rocket/net NET_DIR_SRCS)
aux_source_directory(./rocket/tcp TCP_DIR_SRCS)
aux_source_directory(./rocket/coder CODER_DIR_SRCS)
aux_source_directory(./rocket/rpc RPC_DIR_SRCS)


add_executable(TEST_RPC_SERVER test_cases/test_rpc_server.cpp
        ${COMMON_DIR_SRCS}
        ${NET_DIR_SRCS}
        ${TCP_DIR_SRCS}
        ${CODER_DIR_SRCS}
        ${RPC_DIR_SRCS}
        test_cases/order.pb.cc)

add_executable(TEST_RPC_CLIENT test_cases/test_rpc_client.cpp
        ${COMMON_DIR_SRCS}
        ${NET_DIR_SRCS}
        ${TCP_DIR_SRCS}
        ${CODER_DIR_SRCS}
        ${RPC_DIR_SRCS}
        test_cases/order.pb.cc)

find_package(Threads)

target_link_libraries(TEST_RPC_SERVER PRIVATE
        ${CMAKE_THREAD_LIBS_INIT}
        tinyxml protobuf)

target_link_libraries(TEST_RPC_CLIENT PRIVATE
        ${CMAKE_THREAD_LIBS_INIT}
        tinyxml protobuf)