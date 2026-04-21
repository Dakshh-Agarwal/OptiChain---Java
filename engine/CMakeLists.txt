cmake_minimum_required(VERSION 3.14)
project(InventoryEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(engine
    main.cpp
    graph.cpp
    fenwick.cpp
    allocator.cpp
    splitter.cpp
    demand.cpp
)

# Windows-specific: static linking so no DLL dependencies
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
    target_compile_options(engine PRIVATE /W3)
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
    target_compile_options(engine PRIVATE -Wall -Wextra)
endif()
