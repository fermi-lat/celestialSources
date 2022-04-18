add_library(
  eblAtten STATIC
  src/AsciiTableModel.cxx
  src/EblAtten.cxx
  src/IRB_routines.cxx
  src/Primack05.cxx
)
target_include_directories(
  eblAtten PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:>
)
target_link_libraries(
  eblAtten PUBLIC
  facilities
  st_facilities
)
# target_compile_definitions(eblAtten PUBLIC TRAP_FPE)

add_executable(test_eblAtten src/test/test.cxx)
target_link_libraries(test_eblAtten PRIVATE eblAtten)
