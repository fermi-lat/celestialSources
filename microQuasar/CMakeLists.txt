add_library(microQuasar STATIC src/microQuasar.cxx)
target_include_directories(
  microQuasar
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:>
)
target_link_libraries(
  microQuasar
  PRIVATE genericSources flux astro facilities
)

add_executable(test_microQuasar src/test/test_main.cxx)
target_link_libraries(test_microQuasar PRIVATE microQuasar flux)

###############################################################
# Installation
###############################################################
install(DIRECTORY microQuasar DESTINATION ${FERMI_INSTALL_INCLUDEDIR})
install(DIRECTORY xml/ DESTINATION ${FERMI_INSTALL_XMLDIR}/microQuasar)

install(
  TARGETS microQuasar test_microQuasar
  # EXPORT fermiTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
