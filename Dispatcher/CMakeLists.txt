cmake_minimum_required(VERSION 3.12)
project(Dispatcher)

set(CMAKE_CXX_STANDARD 17)

INCLUDE_DIRECTORIES(
        /Users/williamleonards/Documents/vcpkg/installed/x64-osx/include
        /usr/local/include
        /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1/include
)
LINK_DIRECTORIES(
        /Users/williamleonards/Documents/vcpkg/installed/x64-osx/lib
        /usr/local/lib
        /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1/lib
)

find_package(Poco REQUIRED Net)
find_package(Poco REQUIRED Util)
#find_package(cpprestsdk REQUIRED)

# <------------ add hiredis dependency --------------->
find_path(HIREDIS_HEADER hiredis)
include_directories(PUBLIC ${HIREDIS_HEADER})

find_library(HIREDIS_LIB hiredis)
link_libraries(${HIREDIS_LIB})

# <------------ add redis-plus-plus dependency -------------->
# NOTE: this should be *sw* NOT *redis++*
find_path(REDIS_PLUS_PLUS_HEADER sw)
include_directories(PUBLIC ${REDIS_PLUS_PLUS_HEADER})

find_library(REDIS_PLUS_PLUS_LIB redis++)
link_libraries(${REDIS_PLUS_PLUS_LIB})

link_libraries(
        /usr/local/lib/libamqpcpp.a
        Poco::Net
        Poco::Util
)

add_executable(Dispatcher main.cpp Synchronizer.cpp Sema.cpp SimplePocoHandler.cpp
        SessionManager.cpp)