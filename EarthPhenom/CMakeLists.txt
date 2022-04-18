add_library(
  EarthPhenom STATIC
  src/EarthPhenomInner.cxx
  src/EarthPhenomLimb.cxx
)
target_include_directories(
  EarthPhenom PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:>
)
target_link_libraries(EarthPhenom PUBLIC astro facilities flux ROOT::Core)

add_executable(test_EarthPhenom src/test/test_main.cxx)
target_link_libraries(test_EarthPhenom EarthPhenom)
