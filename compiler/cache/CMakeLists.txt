add_library(li_check_cache STATIC check_cache.cpp)
target_include_directories(li_check_cache PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(li_check_cache PUBLIC li_config li_diagnostics)
target_compile_definitions(li_check_cache PRIVATE LI_VERSION="${LI_VERSION_STRING}")