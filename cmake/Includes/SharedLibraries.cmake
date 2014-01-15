# Find our globally shared libraries
FIND_PACKAGE(NUClear REQUIRED)
FIND_PACKAGE(ZMQ REQUIRED)
FIND_PACKAGE(Armadillo REQUIRED)
FIND_PACKAGE(Protobuf REQUIRED)
FIND_PACKAGE(CATCH REQUIRED)
INCLUDE_DIRECTORIES(${NUCLEAR_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${ARMADILLO_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${ZMQ_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CATCH_INCLUDE_DIRS})

SET(NUBOTS_SHARED_LIBRARIES
    ${NUCLEAR_LIBRARIES}
    ${ZMQ_LIBRARIES}
    ${ARMADILLO_LIBRARIES}
    ${PROTOBUF_LIBRARIES})

# These libraries are actually built as part of the build process but are treated as ordinary libraries
ADD_SUBDIRECTORY(lib/jsmn)
INCLUDE_DIRECTORIES(${JSMN_INCLUDE_DIRS})