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
include helper/CMakeFiles/oshelper.dir/depend.make

# Include the progress variables for this target.
include helper/CMakeFiles/oshelper.dir/progress.make

# Include the compile flags for this target's objects.
include helper/CMakeFiles/oshelper.dir/flags.make

helper/CMakeFiles/oshelper.dir/oshelper.c.o: helper/CMakeFiles/oshelper.dir/flags.make
helper/CMakeFiles/oshelper.dir/oshelper.c.o: ../helper/oshelper.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/root/Work/nemoapps/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object helper/CMakeFiles/oshelper.dir/oshelper.c.o"
	cd /home/root/Work/nemoapps/build/helper && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/oshelper.dir/oshelper.c.o   -c /home/root/Work/nemoapps/helper/oshelper.c

helper/CMakeFiles/oshelper.dir/oshelper.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/oshelper.dir/oshelper.c.i"
	cd /home/root/Work/nemoapps/build/helper && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/root/Work/nemoapps/helper/oshelper.c > CMakeFiles/oshelper.dir/oshelper.c.i

helper/CMakeFiles/oshelper.dir/oshelper.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/oshelper.dir/oshelper.c.s"
	cd /home/root/Work/nemoapps/build/helper && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/root/Work/nemoapps/helper/oshelper.c -o CMakeFiles/oshelper.dir/oshelper.c.s

helper/CMakeFiles/oshelper.dir/oshelper.c.o.requires:

.PHONY : helper/CMakeFiles/oshelper.dir/oshelper.c.o.requires

helper/CMakeFiles/oshelper.dir/oshelper.c.o.provides: helper/CMakeFiles/oshelper.dir/oshelper.c.o.requires
	$(MAKE) -f helper/CMakeFiles/oshelper.dir/build.make helper/CMakeFiles/oshelper.dir/oshelper.c.o.provides.build
.PHONY : helper/CMakeFiles/oshelper.dir/oshelper.c.o.provides

helper/CMakeFiles/oshelper.dir/oshelper.c.o.provides.build: helper/CMakeFiles/oshelper.dir/oshelper.c.o


oshelper: helper/CMakeFiles/oshelper.dir/oshelper.c.o
oshelper: helper/CMakeFiles/oshelper.dir/build.make

.PHONY : oshelper

# Rule to build all files generated by this target.
helper/CMakeFiles/oshelper.dir/build: oshelper

.PHONY : helper/CMakeFiles/oshelper.dir/build

helper/CMakeFiles/oshelper.dir/requires: helper/CMakeFiles/oshelper.dir/oshelper.c.o.requires

.PHONY : helper/CMakeFiles/oshelper.dir/requires

helper/CMakeFiles/oshelper.dir/clean:
	cd /home/root/Work/nemoapps/build/helper && $(CMAKE_COMMAND) -P CMakeFiles/oshelper.dir/cmake_clean.cmake
.PHONY : helper/CMakeFiles/oshelper.dir/clean

helper/CMakeFiles/oshelper.dir/depend:
	cd /home/root/Work/nemoapps/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/root/Work/nemoapps /home/root/Work/nemoapps/helper /home/root/Work/nemoapps/build /home/root/Work/nemoapps/build/helper /home/root/Work/nemoapps/build/helper/CMakeFiles/oshelper.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : helper/CMakeFiles/oshelper.dir/depend

