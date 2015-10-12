# - try to find OSVR RenderManager library
#
# Cache Variables:
#  OSVRRenderManager_LIBRARY
#  OSVRRenderManager_INCLUDE_DIR
#
# Non-cache variables you might use in your CMakeLists.txt:
#  OSVRRENDERMANAGER_FOUND
#  OSVRRenderManager_LIBRARIES
#  OSVRRenderManager_INCLUDE_DIRS
#
# OSVRRenderManager_ROOT_DIR is searched preferentially for these files
#
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#
# Original Author:
# 2015 Russell Taylor <russ@reliasolve.com>
# Based on the FindVRPN.cmake file created by Ryan Pavlik
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

set(OSVRRenderManager_ROOT_DIR
	"${OSVRRenderManager_ROOT_DIR}"
	CACHE
	PATH
	"Root directory to search for OSVRRenderManager")

if("${CMAKE_SIZEOF_VOID_P}" MATCHES "8")
	set(_libsuffixes lib64 lib RelWithDebInfo Release Debug)

	# 64-bit dir: only set on win64
	file(TO_CMAKE_PATH "$ENV{ProgramW6432}" _progfiles)
else()
	set(_libsuffixes lib RelWithDebInfo Release Debug)
	set(_PF86 "ProgramFiles(x86)")
	if(NOT "$ENV{${_PF86}}" STREQUAL "")
		# 32-bit dir: only set on win64
		file(TO_CMAKE_PATH "$ENV{${_PF86}}" _progfiles)
	else()
		# 32-bit dir on win32, useless to us on win64
		file(TO_CMAKE_PATH "$ENV{ProgramFiles}" _progfiles)
	endif()
endif()

set(_OSVRRenderManager_quiet)
if(OSVRRenderManager_FIND_QUIETLY)
	set(_OSVRRenderManager_quiet QUIET)
endif()

###
# Configure OSVRRenderManager
###

find_path(OSVRRenderManager_INCLUDE_DIR
	NAMES
	RenderManager.h
	PATH_SUFFIXES
	include
	HINTS
	"${OSVRRenderManager_ROOT_DIR}"
	PATHS
	"${_progfiles}/osvrRenderManager"
	C:/usr/local
	/usr/local)

find_library(OSVRRenderManager_LIBRARY
	NAMES
	osvrRenderManager
	PATH_SUFFIXES
	${_libsuffixes}
	HINTS
	"${OSVRRenderManager_ROOT_DIR}"
	PATHS
	"${_progfiles}/osvrRenderManager"
	C:/usr/local
	/usr/local)

###
# Dependencies
###
set(_deps_libs)
set(_deps_includes)
set(_deps_check)

find_package(quatlib ${_OSVRRenderManager_quiet})
list(APPEND _deps_libs ${QUATLIB_LIBRARIES})
list(APPEND _deps_includes ${QUATLIB_INCLUDE_DIRS})
list(APPEND _deps_check QUATLIB_FOUND)

if(NOT WIN32)
	find_package(Threads ${_OSVRRenderManager_quiet})
	list(APPEND _deps_libs ${CMAKE_THREAD_LIBS_INIT})
	list(APPEND _deps_check CMAKE_HAVE_THREADS_LIBRARY)
endif()

if(WIN32)
	find_package(Libusb1 ${_OSVRRenderManager_quiet})
	if(LIBUSB1_FOUND)
		list(APPEND _deps_libs ${LIBUSB1_LIBRARIES})
		list(APPEND _deps_includes ${LIBUSB1_INCLUDE_DIRS})
	endif()
endif()


# handle the QUIETLY and REQUIRED arguments and set xxx_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OSVRRenderManager
	DEFAULT_MSG
	OSVRRenderManager_LIBRARY
	OSVRRenderManager_INCLUDE_DIR
	${_deps_check})

if(OSVRRENDERMANAGER_FOUND)
	set(OSVRRenderManager_INCLUDE_DIRS "${OSVRRenderManager_INCLUDE_DIR}" ${_deps_includes})
	set(OSVRRenderManager_LIBRARIES "${OSVRRenderManager_LIBRARY}" ${_deps_libs})

	mark_as_advanced(OSVRRenderManager_ROOT_DIR)
endif()

mark_as_advanced(OSVRRenderManager_LIBRARY OSVRRenderManager_INCLUDE_DIR)

