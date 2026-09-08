add_executable(lic main.cpp check_cmd.cpp frontend.cpp workspace_check.cpp)
target_compile_definitions(lic PRIVATE LI_VERSION="${LI_VERSION_STRING}")
target_link_libraries(lic PRIVATE li_ast li_analyze li_check_cache li_codegen li_common li_config li_diagnostics li_lexer li_parser li_types li_mir li_verify)

# vc_emit.hpp (AutoVC Lean emission) lives in the verify library's include dir;
# lic's own headers (li/check_cmd.hpp, li/frontend.hpp, li/workspace_check.hpp)
# live in include/.
target_include_directories(lic PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../verify/include)