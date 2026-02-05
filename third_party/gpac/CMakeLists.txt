add_library(gpac INTERFACE)
target_include_directories(gpac INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include/")
target_link_libraries(gpac INTERFACE gpac_impl)