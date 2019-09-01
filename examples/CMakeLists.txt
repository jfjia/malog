set(LIBRARIES malog ${DEPENDENT_LIBRARIES})
add_executable(bench bench.cpp)
target_link_libraries(bench ${LIBRARIES})
