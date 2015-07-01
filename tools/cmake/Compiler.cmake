# Default to do a debug build
IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Debug CACHE STRING
       "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel LTO MinSizeRel."
       FORCE)
ENDIF()

SET(CMAKE_FIND_LIBRARY_SUFFIXES
    ${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${CMAKE_SHARED_LIBRARY_SUFFIX})

# Common C++ Flags
# Enable c++11
SET(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -std=c++11")

# We need noncall exceptions so we can throw exceptions from signal handlers
SET(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -fnon-call-exceptions")

IF(NOT "${CMAKE_GENERATOR}" MATCHES "Xcode")
    # 32 bit builds for darwins
    SET(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -m32")
    SET(COMMON_C_FLAGS "${COMMON_C_FLAGS} -m32")
ENDIF()

# If/When we go to 64 bit we need position independent code
SET(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -fPIC")
SET(COMMON_C_FLAGS "${COMMON_C_FLAGS} -fPIC")

# Make armadillo not use the wrapper:
SET(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -DARMA_DONT_USE_WRAPPER -DARMA_32BIT_WORD")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DARMA_NO_DEBUG")

# Enable super strict warnings
SET(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -Wall -Wpedantic -Wextra")
SET(COMMON_C_FLAGS "${COMMON_C_FLAGS} -Wall -Wpedantic -Wextra")

# Tune for the intel atom (Darwin-OP runs on intel atom)
SET(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -march=atom -mtune=atom")
SET(COMMON_C_FLAGS "${COMMON_C_FLAGS} -march=atom -mtune=atom")

# XCode support
IF("${CMAKE_GENERATOR}" MATCHES "Xcode")
    message("Enabling xcode support")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
ENDIF()

# We support compiling with G++
IF("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")

    # Check for version
    SET(GCC_MINIMUM_VERSION 4.8)
    EXECUTE_PROCESS(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)

    # We need at least g++ 4.8 (before that c++11 support was terrible)
    IF(GCC_VERSION VERSION_LESS ${GCC_MINIMUM_VERSION})
        message(FATAL_ERROR "${PROJECT_NAME} requires g++ 4.8 or greater.")
    ENDIF()

    # If we have g++4.9 or later, then enable coloured output
    IF(GCC_VERSION VERSION_GREATER "4.9" OR GCC_VERSION VERSION_EQUAL "4.9")
        SET(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -fdiagnostics-color=always")
        SET(COMMON_C_FLAGS "${COMMON_C_FLAGS} -fdiagnostics-color=always")
    ENDIF()

# We support compiling with clang
ELSEIF("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    SET(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS}")
    # No changes required

# We don't support other compilers (but if you wanna try then change this line)
ELSE()
    MESSAGE(FATAL_ERROR "Unsupported compiler!")
ENDIF()

# Now build our compiler flags for each release type
SET(CMAKE_CXX_FLAGS                "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")
SET(CMAKE_C_FLAGS                  "${CMAKE_C_FLAGS} ${COMMON_C_FLAGS}")
SET(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG} ${COMMON_CXX_FLAGS}")
SET(CMAKE_C_FLAGS_DEBUG            "${CMAKE_C_FLAGS_DEBUG} ${COMMON_C_FLAGS}")
SET(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE} ${COMMON_CXX_FLAGS}")
SET(CMAKE_C_FLAGS_RELEASE          "${CMAKE_C_FLAGS_RELEASE} ${COMMON_C_FLAGS}")
SET(CMAKE_CXX_FLAGS_LTO            "-fuse-linker-plugin -ffast-math -flto=jobserver -fno-fat-lto-objects ${CMAKE_CXX_FLAGS_RELEASE}  ${COMMON_CXX_FLAGS}")
SET(CMAKE_C_FLAGS_LTO              "-fuse-linker-plugin -ffast-math -flto=jobserver -fno-fat-lto-objects ${CMAKE_C_FLAGS_RELEASE} ${COMMON_C_FLAGS}")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${COMMON_CXX_FLAGS}")
SET(CMAKE_C_FLAGS_RELWITHDEBINFO   "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${COMMON_C_FLAGS}")
SET(CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_MINSIZEREL} ${COMMON_CXX_FLAGS}")
SET(CMAKE_C_FLAGS_MINSIZEREL       "${CMAKE_C_FLAGS_MINSIZEREL} ${COMMON_C_FLAGS}")
