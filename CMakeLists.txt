cmake_minimum_required(VERSION 3.15 )

set(version 0.5)
project(icehydra VERSION ${version} LANGUAGES C)

set(project_prefix "icehydra")

aux_source_directory(src DIR_LIB_SRCS)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
endif()
#message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

include_directories(include)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
	link_directories(deplib/x86)
	add_compile_definitions (x86)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
	link_directories(deplib/arm64)
	add_compile_definitions (arm64)
endif()

add_compile_definitions(EXPORT_IH_LIB)
list(APPEND GLOBAL_C_FLAGS "-Wall")

option(BUILD_SHARED_LIBS  "Compile into a shared library " ON)
message(STATUS "BUILD_SHARED_LIBS - ${BUILD_SHARED_LIBS}")
option(SYSTEM-V-SHM  "SYSTEM-V-SHM : Use SYS-V SHM " ON)
message(STATUS "SYSTEM-V-SHM - ${SYSTEM-V-SHM}")
option(POSIX-SHM  "POSIX-SHM : Use shm_open() " OFF)
message(STATUS "POSIX-SHM - ${POSIX-SHM}")
option(MEMFD_OPEN-SHM  "MEMFD_OPEN-SHM : Use memfd_create() " OFF)
message(STATUS "MEMFD_OPEN-SHM - ${MEMFD_OPEN-SHM}")

if(SYSTEM-V-SHM)
	add_compile_definitions (IH_HAVE_SYSVSHM=1)
elseif(POSIX-SHM)
	add_compile_definitions (IH_HAVE_POSIXSHM=1)
elseif(MEMFD_OPEN-SHM)
	add_compile_definitions (IH_HAVE_MEMFDSHM=1)
endif()

include(GNUInstallDirs)
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
#    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
#    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
#    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

# Offer the user the choice of overriding the installation directories
set(INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR} CACHE PATH "Installation directory for libraries")
set(INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR} CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE PATH "Installation directory for header files")
set(DEF_INSTALL_CMAKEDIR share/cmake/${PROJECT_NAME})
set(INSTALL_CMAKEDIR ${DEF_INSTALL_CMAKEDIR} CACHE PATH "Installation directory for CMake files")

foreach(p LIB BIN INCLUDE CMAKE)
  file(TO_NATIVE_PATH ${CMAKE_INSTALL_PREFIX}/${INSTALL_${p}DIR} _path )
  message(STATUS "Installing ${p} components to ${_path}")
  unset(_path)
endforeach()

#message(STATUS "[debug]cmake info  ${CMAKE_C_FLAGS_RELEASE}")

add_library (${project_prefix}-objs   OBJECT ${DIR_LIB_SRCS})
set_target_properties(${project_prefix}-objs 
				PROPERTIES
				C_VISIBILITY_PRESET hidden
				POSITION_INDEPENDENT_CODE 1)
target_compile_options(${project_prefix}-objs
				PRIVATE
				${GLOBAL_C_FLAGS})

add_library (${project_prefix}-api-objs  OBJECT icehydra.c icehydra.h)
set_target_properties(${project_prefix}-api-objs 
				PROPERTIES
				C_VISIBILITY_PRESET hidden
				POSITION_INDEPENDENT_CODE 1)
target_compile_options(${project_prefix}-api-objs
				PRIVATE
				${GLOBAL_C_FLAGS})
				
if(BUILD_SHARED_LIBS)
	add_library (${project_prefix}-shared 
				SHARED 
				$<TARGET_OBJECTS:${project_prefix}-objs>
				$<TARGET_OBJECTS:${project_prefix}-api-objs>)
				
	target_compile_options(${project_prefix}-shared PRIVATE ${GLOBAL_C_FLAGS} )
	set_target_properties(${project_prefix}-shared 
				PROPERTIES 
				VERSION ${version} 
				SOVERSION 0 
				PUBLIC_HEADER "icehydra.h"
				C_VISIBILITY_PRESET hidden
				OUTPUT_NAME ${PROJECT_NAME})

	add_library (${project_prefix}-static STATIC $<TARGET_OBJECTS:${project_prefix}-objs>)
	target_compile_options(${project_prefix}-static PRIVATE ${GLOBAL_C_FLAGS})
	set_target_properties(${project_prefix}-static 
				PROPERTIES 
				PUBLIC_HEADER "icehydra.h"
				C_VISIBILITY_PRESET hidden
				OUTPUT_NAME ${PROJECT_NAME})
	
	add_executable(${PROJECT_NAME} main.c)
	target_compile_options(${PROJECT_NAME} PRIVATE ${GLOBAL_C_FLAGS})
	target_link_libraries(${PROJECT_NAME} ${project_prefix}-static)
	
endif()


install(
  TARGETS
    ${PROJECT_NAME}
	${project_prefix}-shared
  ARCHIVE
    DESTINATION ${INSTALL_LIBDIR}
    COMPONENT lib
  RUNTIME
    DESTINATION ${INSTALL_BINDIR}
    COMPONENT bin
  LIBRARY
    DESTINATION ${INSTALL_LIBDIR}
    COMPONENT lib
  PUBLIC_HEADER
    DESTINATION ${INSTALL_INCLUDEDIR}
    COMPONENT INCLUDE
  )