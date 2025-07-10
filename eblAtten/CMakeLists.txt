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

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_executable(test_eblAtten src/test/test.cxx)
target_link_libraries(test_eblAtten PRIVATE eblAtten)

###############################################################
# Installation
###############################################################
install(DIRECTORY eblAtten DESTINATION ${FERMI_INSTALL_INCLUDEDIR})
install(DIRECTORY data/ DESTINATION ${FERMI_INSTALL_REFDATADIR}/eblAtten)

install(
  TARGETS eblAtten test_eblAtten
  # EXPORT fermiTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
