# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /mnt/d/shared/icehydra

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /mnt/d/shared/icehydra/build

# Include any dependencies generated for this target.
include CMakeFiles/icehydra.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/icehydra.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/icehydra.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/icehydra.dir/flags.make

CMakeFiles/icehydra.dir/main.c.o: CMakeFiles/icehydra.dir/flags.make
CMakeFiles/icehydra.dir/main.c.o: ../main.c
CMakeFiles/icehydra.dir/main.c.o: CMakeFiles/icehydra.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/d/shared/icehydra/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/icehydra.dir/main.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/icehydra.dir/main.c.o -MF CMakeFiles/icehydra.dir/main.c.o.d -o CMakeFiles/icehydra.dir/main.c.o -c /mnt/d/shared/icehydra/main.c

CMakeFiles/icehydra.dir/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/icehydra.dir/main.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/d/shared/icehydra/main.c > CMakeFiles/icehydra.dir/main.c.i

CMakeFiles/icehydra.dir/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/icehydra.dir/main.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/d/shared/icehydra/main.c -o CMakeFiles/icehydra.dir/main.c.s

# Object files for target icehydra
icehydra_OBJECTS = \
"CMakeFiles/icehydra.dir/main.c.o"

# External object files for target icehydra
icehydra_EXTERNAL_OBJECTS =

icehydra: CMakeFiles/icehydra.dir/main.c.o
icehydra: CMakeFiles/icehydra.dir/build.make
icehydra: libicehydra.a
icehydra: CMakeFiles/icehydra.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/mnt/d/shared/icehydra/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable icehydra"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/icehydra.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/icehydra.dir/build: icehydra
.PHONY : CMakeFiles/icehydra.dir/build

CMakeFiles/icehydra.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/icehydra.dir/cmake_clean.cmake
.PHONY : CMakeFiles/icehydra.dir/clean

CMakeFiles/icehydra.dir/depend:
	cd /mnt/d/shared/icehydra/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/d/shared/icehydra /mnt/d/shared/icehydra /mnt/d/shared/icehydra/build /mnt/d/shared/icehydra/build /mnt/d/shared/icehydra/build/CMakeFiles/icehydra.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/icehydra.dir/depend

