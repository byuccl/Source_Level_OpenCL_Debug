
cmake_minimum_required(VERSION 3.5)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 14)

# set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# include_directories(.)
# add_subdirectory(easygl)
# add_subdirectory(src)

file(GLOB asst_routing_srcs "*.cpp"
)

add_executable(htmlgen ${asst_routing_srcs})
# target_link_libraries(asst_routing easygl)
# target_include_directories(asst_routing PUBLIC ".")