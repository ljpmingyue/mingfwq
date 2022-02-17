cmake_minimum_required(VERSION 3.0) #指定cmake最小版本
project(mingfwq)                    #设置项目名称

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -ggdb -fPIC -std=c++11 -fpermissive -Wall -Wno-deprecated -Werror -Wno-unused-function")

##set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

##将指定目录添加到编译器的头文件搜索路径之下
include_directories(.)
include_directories(..)
include_directories(/usr/local/include) #设置包含的目录
link_directories(/usr/local/lib)        #设置链接库搜索目录


set(LIB_SRC
    mingfwq/address.cpp
    mingfwq/bytearray.cpp
    mingfwq/config.cpp
    mingfwq/fd_manager.cpp
    mingfwq/fiber.cpp
    mingfwq/http/http.cpp
    mingfwq/http/http_connection.cpp
    mingfwq/http/http_parser.cpp
    mingfwq/http/http_session.cpp
    mingfwq/http/http_server.cpp
    mingfwq/http/servlet.cpp
    mingfwq/http/http11_parser.rl.cpp
    mingfwq/http/httpclient_parser.rl.cpp
    mingfwq/hook.cpp
    mingfwq/iomanager.cpp
    mingfwq/log.cpp 
    mingfwq/scheduler.cpp
    mingfwq/socket.cpp
    mingfwq/socket_stream.cpp
    mingfwq/stream.cpp
    mingfwq/tcp_server.cpp
    mingfwq/timer.cpp
    mingfwq/thread.cpp
    mingfwq/util.cpp  
    mingfwq/uri.rl.cpp
   )

##ragelmaker(mingfwq/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/mingfwq)

find_library(YAMLCPP yaml-cpp)          #查找指定的库文件
find_library(PTHREAD pthread)

#ragelmaker(mingfwq/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/mingfwq/http)

# SHARED动态库或共享库  STATIC静态库
add_library(mingfwq SHARED ${LIB_SRC}) #生成动态库或者共享库
#add_library(mingyuefwq_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (mingyuefwq_static PROPERTIES OUTPUT_NAME "mingyuefwq")

set(LIBS
    mingfwq
    dl
    pthread
    yaml-cpp  
    )

##mingfwq_add_executable(test "tests/test.cpp" mingfwq "${LIBS}")

add_executable(test tests/test.cpp)     #生成可执行文件
add_dependencies(test mingfwq)
target_link_libraries(test mingfwq ${LIBS})  #设置需要链接的库

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config mingfwq)
target_link_libraries(test_config mingfwq ${LIBS})

add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread mingfwq)
target_link_libraries(test_thread mingfwq ${LIBS})

add_executable(test_util tests/test_util.cpp)
add_dependencies(test_util mingfwq)
target_link_libraries(test_util mingfwq ${LIBS})

add_executable(test_fiber tests/test_fiber.cpp)
add_dependencies(test_fiber mingfwq)
target_link_libraries(test_fiber mingfwq ${LIBS})

add_executable(test_scheduler tests/test_scheduler.cpp)
add_dependencies(test_scheduler mingfwq)
target_link_libraries(test_scheduler mingfwq ${LIBS})

add_executable(test_iomanager tests/test_iomanager.cpp)
add_dependencies(test_iomanager mingfwq)
target_link_libraries(test_iomanager mingfwq ${LIBS})

add_executable(test_hook tests/test_hook.cpp)
add_dependencies(test_hook mingfwq)
target_link_libraries(test_hook mingfwq ${LIBS})

add_executable(test_address tests/test_address.cpp)
add_dependencies(test_address mingfwq)
target_link_libraries(test_address mingfwq ${LIBS})

add_executable(test_socket tests/test_socket.cpp)
add_dependencies(test_socket mingfwq)
target_link_libraries(test_socket mingfwq ${LIBS})

add_executable(test_bytearray tests/test_bytearray.cpp)
add_dependencies(test_bytearray mingfwq)
target_link_libraries(test_bytearray mingfwq ${LIBS})

add_executable(test_http tests/test_http.cpp)
add_dependencies(test_http mingfwq)
target_link_libraries(test_http mingfwq ${LIBS})

add_executable(test_http_parser tests/test_http_parser.cpp)
add_dependencies(test_http_parser mingfwq)
target_link_libraries(test_http_parser mingfwq ${LIBS})

add_executable(test_tcp_server tests/test_tcp_server.cpp)
add_dependencies(test_tcp_server mingfwq)
target_link_libraries(test_tcp_server mingfwq ${LIBS})

add_executable(echo_server examples/echo_server.cpp)
add_dependencies(echo_server mingfwq)
target_link_libraries(echo_server mingfwq ${LIBS})

add_executable(test_http_server tests/test_http_server.cpp)
add_dependencies(test_http_server mingfwq)
target_link_libraries(test_http_server mingfwq ${LIBS})

add_executable(test_http_connect tests/test_http_connect.cpp)
add_dependencies(test_http_connect mingfwq)
target_link_libraries(test_http_connect mingfwq ${LIBS})

add_executable(test_uri tests/test_uri.cpp)
add_dependencies(test_uri mingfwq)
target_link_libraries(test_uri mingfwq ${LIBS})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

