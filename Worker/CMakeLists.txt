cmake_minimum_required(VERSION 3.12)
project(Worker)

set(CMAKE_CXX_STANDARD 14)

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

link_libraries(
        /usr/local/lib/libamqpcpp.a
        Poco::Net
)

add_executable(Worker main.cpp SimplePocoHandler.cpp)