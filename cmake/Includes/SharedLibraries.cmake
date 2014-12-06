# Find our globally shared libraries
FIND_PACKAGE(NUClear REQUIRED)
FIND_PACKAGE(Tcmalloc REQUIRED)
FIND_PACKAGE(ZMQ REQUIRED)
FIND_PACKAGE(BLAS REQUIRED)
FIND_PACKAGE(LAPACK REQUIRED)
FIND_PACKAGE(Armadillo REQUIRED)
FIND_PACKAGE(Protobuf REQUIRED)
FIND_PACKAGE(CATCH REQUIRED)
FIND_PACKAGE(YAML REQUIRED)
FIND_PACKAGE(MATHEVAL REQUIRED)

INCLUDE_DIRECTORIES(${BLAS_INCLUDE_DIRS} REQUIRED)
INCLUDE_DIRECTORIES(${LAPACK_INCLUDE_DIRS} REQUIRED)
INCLUDE_DIRECTORIES(${NUCLEAR_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${ARMADILLO_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${ZMQ_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CATCH_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${YAML_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${MATHEVAL_INCLUDE_DIRS})

SET(NUBOTS_SHARED_LIBRARIES
    ${TCMALLOC_LIBRARIES}
    ${NUCLEAR_LIBRARIES}
    ${BLAS_LIBRARIES}
    ${LAPACK_LIBRARIES}
    ${ZMQ_LIBRARIES}
    ${ARMADILLO_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    ${YAML_LIBRARIES}
    ${MATHEVAL_LIBRARIES})
