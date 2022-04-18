add_library(
  genericSources STATIC
  src/FileSpectrum.cxx
  src/FileSpectrumMap.cxx
  src/FitsImage.cxx
  src/FitsTransient.cxx
  src/GaussianQuadrature.h
  src/GaussianSource.cxx
  src/GaussianSpectrum.cxx
  src/Isotropic.cxx
  src/IsotropicFileSpectrum.cxx
  src/MapCube.cxx
  src/MapSource.cxx
  src/PeriodicSource.cxx
  src/Pulsar.cxx
  src/RadialSource.cxx
  src/SimpleTransient.cxx
  src/SourcePopulation.cxx
  src/SpectralTransient.cxx
  src/Util.cxx
)
target_include_directories(
  genericSources PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:>
)

target_link_libraries(
  genericSources PUBLIC
  facilities
  celestialSources
  eblAtten
  flux
  astro
  PkgConfig::cfitsio
  CLHEP::CLHEP
)

# add_executable(test_genericSources src/test/test.cxx)
# target_link_libraries(test_genericSources PRIVATE genericSources)
# target_include_directories(
#   test_genericSources PUBLIC
#   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
#   $<INSTALL_INTERFACE:>
# )
