project(pcap2packet)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

add_executable(pcap2packet pcap2packet.cpp)
target_link_libraries(pcap2packet pcap)
