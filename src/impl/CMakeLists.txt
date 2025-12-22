# RSM (Reflective Shadow Maps) 目标
add_executable(rsm rsm_main.cpp obj_loader.hpp obj_loader.cpp)
target_link_libraries(rsm PRIVATE common)
