# Get the path to our parent directory above the message folder
get_filename_component(message_parent_dir "${PROJECT_SOURCE_DIR}/${NUCLEAR_MESSAGE_DIR}/.." ABSOLUTE)

# Generate the protocol buffers that are built into NUClear
set(builtin_dir "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(message_dir "${PROJECT_SOURCE_DIR}/${NUCLEAR_MESSAGE_DIR}")
file(GLOB_RECURSE builtin_protobufs "${builtin_dir}/**.proto")
file(GLOB_RECURSE message_protobufs "${message_dir}/**.proto")

# Locations to store each of the output components
set(pb_out "${CMAKE_CURRENT_BINARY_DIR}/protobuf")
set(nt_out "${CMAKE_CURRENT_BINARY_DIR}/neutron")
set(py_out "${CMAKE_CURRENT_BINARY_DIR}/python")

# Files that are used to generate the neutron files
file(GLOB_RECURSE message_class_generator_files "${CMAKE_CURRENT_SOURCE_DIR}/generator/**.py")

# We need protobuf and python to generate the neutron messages
find_package(Protobuf REQUIRED)
find_package(PythonInterp 3 REQUIRED)

# We need Eigen for neutron messages
find_package(Eigen3 REQUIRED)

# If we have pybind11 we need to generate our final binding class
if(pybind11_FOUND)
  find_package(PythonLibsNew 3 REQUIRED)

  include_directories(SYSTEM ${pybind11_INCLUDE_DIRS})
  include_directories(SYSTEM ${PYTHON_INCLUDE_DIRS})
endif()

# Build the builtin protocol buffers as normal
foreach(proto ${builtin_protobufs})

  get_filename_component(file_we ${proto} NAME_WE)

  add_custom_command(
    OUTPUT "${pb_out}/${file_we}.pb.cc" "${pb_out}/${file_we}.pb.h" "${py_out}/${file_we}_pb2.py"
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} ARGS --cpp_out=lite:${pb_out} --python_out=${py_out} -I${builtin_dir}
            "${CMAKE_CURRENT_SOURCE_DIR}/proto/${file_we}.proto"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/proto/${file_we}.proto"
    COMMENT "Compiling protocol buffer ${proto}"
  )

  list(APPEND protobuf_src "${pb_out}/${file_we}.pb.cc" "${pb_out}/${file_we}.pb.h")

  # Compile flags prevent Effective C++ and unused parameter error checks being performed on generated files.
  set_source_files_properties(
    "${pb_out}/${file_we}.pb.cc" "${pb_out}/${file_we}.pb.h" "${pb_out}/${file_we}_pb2.py"
    PROPERTIES GENERATED TRUE COMPILE_FLAGS "-Wno-unused-parameter -Wno-error=unused-parameter -Wno-error"
  )
endforeach(proto ${builtin_protobufs})

# Build the user protocol buffers
foreach(proto ${message_protobufs})

  # Extract the components of the filename that we need
  get_filename_component(file_we ${proto} NAME_WE)

  file(RELATIVE_PATH output_path ${message_parent_dir} ${proto})
  get_filename_component(output_path ${output_path} PATH)
  set(pb ${pb_out}/${output_path}/${file_we})
  set(py ${py_out}/${output_path}/${file_we})
  set(nt ${nt_out}/${output_path}/${file_we})

  #
  # DEPENDENCIES
  #
  # Ideally ninja would do this at runtime, but for now we have to work out what dependencies each of the protocol
  # buffer files have. If these change we just have to hope that it'll work until someone runs cmake again
  execute_process(
    COMMAND
      ${PROTOBUF_PROTOC_EXECUTABLE} --dependency_out=${CMAKE_CURRENT_BINARY_DIR}/dependencies.txt
      --descriptor_set_out=${CMAKE_CURRENT_BINARY_DIR}/descriptor.pb -I${message_parent_dir} -I${builtin_dir} ${proto}
  )

  file(READ ${CMAKE_CURRENT_BINARY_DIR}/dependencies.txt dependencies)
  string(REGEX REPLACE "\\\\\n" ";" dependencies ${dependencies})
  file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/dependencies ${CMAKE_CURRENT_BINARY_DIR}/descriptor.pb)

  # Clean our dependency list
  unset(source_depends)
  unset(binary_depends)

  foreach(depend ${dependencies})
    string(STRIP ${depend} depend)
    file(RELATIVE_PATH depend_rel ${message_parent_dir} ${depend})

    # Absolute dependencies
    if(depend_rel MATCHES "^\\.\\.")
      list(APPEND source_depends ${depend})
      list(APPEND binary_depends ${depend})
      # Relative dependencies
    else()
      list(APPEND source_depends "${message_parent_dir}/${depend_rel}")
      list(APPEND binary_depends "${pb_out}/${depend_rel}")
    endif()
  endforeach()

  #
  # PROTOCOL BUFFERS
  #
  # Repackage our protocol buffers so they don't collide with the actual classes when we make our c++ protobuf classes
  # by adding protobuf to the package
  add_custom_command(
    OUTPUT "${pb}.proto"
    COMMAND ${PYTHON_EXECUTABLE} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/repackage_message.py" "${proto}" "${pb}.proto"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/repackage_message.py" ${proto}
    COMMENT "Repackaging protobuf ${proto}"
  )

  # Run the protocol buffer compiler on these new protobufs
  add_custom_command(
    OUTPUT "${pb}.pb.cc" "${pb}.pb.h" "${py}_pb2.py"
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} ARGS --cpp_out=lite:${pb_out} --python_out=${py_out} -I${pb_out}
            -I${builtin_dir} "${pb}.proto"
    DEPENDS ${binary_depends} "${pb}.proto"
    COMMENT "Compiling protocol buffer ${proto}"
  )

  #
  # NEUTRONS
  #
  # Extract the protocol buffer information so we can generate code off it
  add_custom_command(
    OUTPUT "${nt}.pb"
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} ARGS --descriptor_set_out="${nt}.pb" -I${message_parent_dir} -I${builtin_dir}
            ${proto}
    DEPENDS ${source_depends} ${proto}
    COMMENT "Extracting protocol buffer information from ${proto}"
  )

  # Build our outer python binding wrapper class
  add_custom_command(
    OUTPUT "${nt}.cpp" "${nt}.py.cpp" "${nt}.hpp"
    COMMAND ${PYTHON_EXECUTABLE} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/build_message_class.py" "${nt}"
    WORKING_DIRECTORY ${nt_out}
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/build_message_class.py" ${message_class_generator_files} "${nt}.pb"
    COMMENT "Building classes for ${proto}"
  )

  # Prevent Effective C++ and unused parameter error checks being performed on generated files.
  set_source_files_properties(
    "${nt}.pb"
    "${pb}.proto"
    "${pb}.pb.cc"
    "${pb}.pb.h"
    "${py}_pb2.py"
    "${nt}.cpp"
    "${nt}.py.cpp"
    "${nt}.hpp"
    PROPERTIES GENERATED TRUE COMPILE_FLAGS "-Wno-unused-parameter -Wno-error=unused-parameter -Wno-error"
  )

  # Add the generated files to our list
  list(APPEND src "${pb}.pb.cc" "${pb}.pb.h" "${nt}.cpp" "${nt}.hpp")

  # If we have pybind11 also add the python bindings
  if(pybind11_FOUND)
    list(APPEND src "${nt}.py.cpp")
  endif()

  # Add to the respective outputs
  list(APPEND protobuf_src "${pb}.pb.cc" "${pb}.pb.h")
  list(APPEND neutron_src "${nt}.cpp" "${nt}.hpp")

endforeach(proto ${message_protobufs})

# * Make this library be a system include when it's linked into other libraries
# * This will prevent clang-tidy from looking at the headers
add_library(nuclear_message_protobuf OBJECT ${protobuf_src})
set_target_properties(nuclear_message_protobuf PROPERTIES CXX_CLANG_TIDY "")
target_include_directories(nuclear_message_protobuf PRIVATE ${pb_out})
target_include_directories(nuclear_message_protobuf SYSTEM INTERFACE ${pb_out})
target_link_libraries(nuclear_message_protobuf protobuf::libprotobuf)

add_library(nuclear_message ${NUCLEAR_LINK_TYPE} ${neutron_src})
target_compile_features(nuclear_message PUBLIC cxx_std_17)
target_link_libraries(nuclear_message PUBLIC nuclear_message_protobuf)
target_link_libraries(nuclear_message PUBLIC Eigen3::Eigen)
target_include_directories(nuclear_message PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(nuclear_message PUBLIC ${nt_out})

# Generate in the lib folder so it gets installed
if(NUCLEAR_LINK_TYPE STREQUAL "SHARED")
  set_property(TARGET nuclear_message PROPERTY LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/lib")
endif()

# Alias to the namespaced version
add_library(nuclear::message ALIAS nuclear_message)
