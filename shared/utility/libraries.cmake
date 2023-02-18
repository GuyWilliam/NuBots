find_package(Eigen3 REQUIRED)
target_link_libraries(nuclear_utility PUBLIC Eigen3::Eigen)

find_package(YAML-CPP REQUIRED)
target_link_libraries(nuclear_utility PUBLIC YAML-CPP::YAML-CPP)

find_package(ExprTk REQUIRED)
target_link_libraries(nuclear_utility PRIVATE ExprTk::ExprTk)

find_package(fmt REQUIRED)
target_link_libraries(nuclear_utility PUBLIC fmt::fmt)

find_package(zstr REQUIRED)
target_link_libraries(nuclear_utility PUBLIC zstr::zstr)

find_package(mio REQUIRED)
target_link_libraries(nuclear_utility PUBLIC mio::mio)

find_package(Aravis REQUIRED)
target_link_libraries(nuclear_utility PUBLIC Aravis::Aravis)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  find_package(libbacktrace REQUIRED)
  target_link_libraries(nuclear_utility PUBLIC libbacktrace::libbacktrace ${CMAKE_DL_LIBS})
endif()

# Create a symlink to recordings so we can access them from build (helpful for docker)
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/recordings"
  COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_SOURCE_DIR}/recordings" "${CMAKE_BINARY_DIR}/recordings"
  COMMENT "Creating a link to the recordings directory"
)
target_sources(nuclear_utility PRIVATE "${CMAKE_BINARY_DIR}/recordings")

target_compile_features(nuclear_utility PUBLIC cxx_std_17)

# Add the scripts directory to the build directory
file(COPY "${nuclear_utility}/skill/scripts" DESTINATION ${PROJECT_BINARY_DIR})
