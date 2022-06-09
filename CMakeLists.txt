add_library(celestialSources src/SpectrumFactoryLoader.cxx)
target_include_directories(
  celestialSources PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:>
)

if(NOT FERMI_BUILD_ROOT)
  target_compile_definitions(celestialSources PRIVATE BUILD_WITHOUT_ROOT)
else()
  target_sources(celestialSources src/TRandom4.cxx)
  add_subdirectory(EarthPhenom)
  # add_subdirectory(GRB)
  # add_subdirectory(GRBobs)
  # add_subdirectory(GRBtemplate)
  # add_subdirectory(microQuasar)
  # add_subdirectory(Pulsar)
  # add_subdirectory(pySources)
  # add_subdirectory(SpectObj)
endif()

add_subdirectory(eblAtten)
add_subdirectory(genericSources)
add_subdirectory(microQuasar)

target_link_libraries(celestialSources PRIVATE eblAtten genericSources)

#### Tests ####
add_executable(test_celestialSources src/test/test.cxx)
target_link_libraries(test_celestialSources PRIVATE celestialSources)


###############################################################
# Installation
###############################################################
install(DIRECTORY celestialSources DESTINATION ${FERMI_INSTALL_INCLUDEDIR})

install(
  TARGETS celestialSources test_celestialSources
  # EXPORT fermiTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
