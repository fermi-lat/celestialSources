add_library(celestialSources STATIC src/SpectrumFactoryLoader.cxx)
target_include_directories(
  celestialSources PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:>
)

target_link_libraries(celestialSources PUBLIC facilities flux CLHEP::CLHEP)

if(NOT FERMI_BUILD_ROOT)
  target_compile_definitions(celestialSources PUBLIC BUILD_WITHOUT_ROOT)
  add_subdirectory(eblAtten)
  add_subdirectory(genericSources)
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

# add_executable(test_celestialSources src/test/test.cxx)
# target_link_libraries(test_celestialSources PRIVATE celestialSources)

