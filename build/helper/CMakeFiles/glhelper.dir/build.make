# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.4

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
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
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/root/Work/nemoapps

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/root/Work/nemoapps/build

# Include any dependencies generated for this target.
include helper/CMakeFiles/glhelper.dir/depend.make

# Include the progress variables for this target.
include helper/CMakeFiles/glhelper.dir/progress.make

# Include the compile flags for this target's objects.
include helper/CMakeFiles/glhelper.dir/flags.make

helper/CMakeFiles/glhelper.dir/glhelper.c.o: helper/CMakeFiles/glhelper.dir/flags.make
helper/CMakeFiles/glhelper.dir/glhelper.c.o: ../helper/glhelper.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/root/Work/nemoapps/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object helper/CMakeFiles/glhelper.dir/glhelper.c.o"
	cd /home/root/Work/nemoapps/build/helper && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/glhelper.dir/glhelper.c.o   -c /home/root/Work/nemoapps/helper/glhelper.c

helper/CMakeFiles/glhelper.dir/glhelper.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/glhelper.dir/glhelper.c.i"
	cd /home/root/Work/nemoapps/build/helper && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/root/Work/nemoapps/helper/glhelper.c > CMakeFiles/glhelper.dir/glhelper.c.i

helper/CMakeFiles/glhelper.dir/glhelper.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/glhelper.dir/glhelper.c.s"
	cd /home/root/Work/nemoapps/build/helper && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/root/Work/nemoapps/helper/glhelper.c -o CMakeFiles/glhelper.dir/glhelper.c.s

helper/CMakeFiles/glhelper.dir/glhelper.c.o.requires:

.PHONY : helper/CMakeFiles/glhelper.dir/glhelper.c.o.requires

helper/CMakeFiles/glhelper.dir/glhelper.c.o.provides: helper/CMakeFiles/glhelper.dir/glhelper.c.o.requires
	$(MAKE) -f helper/CMakeFiles/glhelper.dir/build.make helper/CMakeFiles/glhelper.dir/glhelper.c.o.provides.build
.PHONY : helper/CMakeFiles/glhelper.dir/glhelper.c.o.provides

helper/CMakeFiles/glhelper.dir/glhelper.c.o.provides.build: helper/CMakeFiles/glhelper.dir/glhelper.c.o


glhelper: helper/CMakeFiles/glhelper.dir/glhelper.c.o
glhelper: helper/CMakeFiles/glhelper.dir/build.make

.PHONY : glhelper

# Rule to build all files generated by this target.
helper/CMakeFiles/glhelper.dir/build: glhelper

.PHONY : helper/CMakeFiles/glhelper.dir/build

helper/CMakeFiles/glhelper.dir/requires: helper/CMakeFiles/glhelper.dir/glhelper.c.o.requires

.PHONY : helper/CMakeFiles/glhelper.dir/requires

helper/CMakeFiles/glhelper.dir/clean:
	cd /home/root/Work/nemoapps/build/helper && $(CMAKE_COMMAND) -P CMakeFiles/glhelper.dir/cmake_clean.cmake
.PHONY : helper/CMakeFiles/glhelper.dir/clean

helper/CMakeFiles/glhelper.dir/depend:
	cd /home/root/Work/nemoapps/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/root/Work/nemoapps /home/root/Work/nemoapps/helper /home/root/Work/nemoapps/build /home/root/Work/nemoapps/build/helper /home/root/Work/nemoapps/build/helper/CMakeFiles/glhelper.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : helper/CMakeFiles/glhelper.dir/depend

