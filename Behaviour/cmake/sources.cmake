# A CMake file for the layman
#   - add your source files to YOUR_SRCS
#   - to include subdirectories either
#       - put each source file in YOUR_SRCS including a *relative* path
#       - include add each subdirectory to YOUR_DIRS and put a cmake/sources.cmake in each
#
#    Copyright (c) 2009 Jason Kulk
#    This file is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This file is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.


########## List your source files here! ############################################
SET (YOUR_SRCS  Job.cpp Job.h
		Behaviour.cpp Behaviour.h
)
####################################################################################
########## List your subdirectories here! ##########################################
SET (YOUR_DIRS Jobs)
####################################################################################

# I need to prefix each file and directory with the correct path
STRING(REPLACE "/cmake/sources.cmake" "" THIS_SRC_DIR ${CMAKE_CURRENT_LIST_FILE})

# Now I need to append each element to NUBOT_SRCS
FOREACH(loop_var ${YOUR_SRCS}) 
    LIST(APPEND NUBOT_SRCS "${THIS_SRC_DIR}/${loop_var}" )
ENDFOREACH(loop_var ${YOUR_SRCS})

# Do the same thing for each subdirectory
FOREACH(loop_var ${YOUR_DIRS}) 
    INCLUDE("${THIS_SRC_DIR}/${loop_var}/cmake/sources.cmake" )
ENDFOREACH(loop_var ${YOUR_DIRS})
