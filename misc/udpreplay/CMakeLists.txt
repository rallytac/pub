project(udpreplay)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

add_executable(udpreplay udpreplay.cpp)
target_link_libraries(udpreplay pcap)
