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
include graph/CMakeFiles/nemodavi.dir/depend.make

# Include the progress variables for this target.
include graph/CMakeFiles/nemodavi.dir/progress.make

# Include the compile flags for this target's objects.
include graph/CMakeFiles/nemodavi.dir/flags.make

graph/CMakeFiles/nemodavi.dir/nemodavi.c.o: graph/CMakeFiles/nemodavi.dir/flags.make
graph/CMakeFiles/nemodavi.dir/nemodavi.c.o: ../graph/nemodavi.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/root/Work/nemoapps/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object graph/CMakeFiles/nemodavi.dir/nemodavi.c.o"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nemodavi.dir/nemodavi.c.o   -c /home/root/Work/nemoapps/graph/nemodavi.c

graph/CMakeFiles/nemodavi.dir/nemodavi.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nemodavi.dir/nemodavi.c.i"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/root/Work/nemoapps/graph/nemodavi.c > CMakeFiles/nemodavi.dir/nemodavi.c.i

graph/CMakeFiles/nemodavi.dir/nemodavi.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nemodavi.dir/nemodavi.c.s"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/root/Work/nemoapps/graph/nemodavi.c -o CMakeFiles/nemodavi.dir/nemodavi.c.s

graph/CMakeFiles/nemodavi.dir/nemodavi.c.o.requires:

.PHONY : graph/CMakeFiles/nemodavi.dir/nemodavi.c.o.requires

graph/CMakeFiles/nemodavi.dir/nemodavi.c.o.provides: graph/CMakeFiles/nemodavi.dir/nemodavi.c.o.requires
	$(MAKE) -f graph/CMakeFiles/nemodavi.dir/build.make graph/CMakeFiles/nemodavi.dir/nemodavi.c.o.provides.build
.PHONY : graph/CMakeFiles/nemodavi.dir/nemodavi.c.o.provides

graph/CMakeFiles/nemodavi.dir/nemodavi.c.o.provides.build: graph/CMakeFiles/nemodavi.dir/nemodavi.c.o


graph/CMakeFiles/nemodavi.dir/nemodavi_data.c.o: graph/CMakeFiles/nemodavi.dir/flags.make
graph/CMakeFiles/nemodavi.dir/nemodavi_data.c.o: ../graph/nemodavi_data.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/root/Work/nemoapps/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object graph/CMakeFiles/nemodavi.dir/nemodavi_data.c.o"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nemodavi.dir/nemodavi_data.c.o   -c /home/root/Work/nemoapps/graph/nemodavi_data.c

graph/CMakeFiles/nemodavi.dir/nemodavi_data.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nemodavi.dir/nemodavi_data.c.i"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/root/Work/nemoapps/graph/nemodavi_data.c > CMakeFiles/nemodavi.dir/nemodavi_data.c.i

graph/CMakeFiles/nemodavi.dir/nemodavi_data.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nemodavi.dir/nemodavi_data.c.s"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/root/Work/nemoapps/graph/nemodavi_data.c -o CMakeFiles/nemodavi.dir/nemodavi_data.c.s

graph/CMakeFiles/nemodavi.dir/nemodavi_data.c.o.requires:

.PHONY : graph/CMakeFiles/nemodavi.dir/nemodavi_data.c.o.requires

graph/CMakeFiles/nemodavi.dir/nemodavi_data.c.o.provides: graph/CMakeFiles/nemodavi.dir/nemodavi_data.c.o.requires
	$(MAKE) -f graph/CMakeFiles/nemodavi.dir/build.make graph/CMakeFiles/nemodavi.dir/nemodavi_data.c.o.provides.build
.PHONY : graph/CMakeFiles/nemodavi.dir/nemodavi_data.c.o.provides

graph/CMakeFiles/nemodavi.dir/nemodavi_data.c.o.provides.build: graph/CMakeFiles/nemodavi.dir/nemodavi_data.c.o


graph/CMakeFiles/nemodavi.dir/nemodavi_selector.c.o: graph/CMakeFiles/nemodavi.dir/flags.make
graph/CMakeFiles/nemodavi.dir/nemodavi_selector.c.o: ../graph/nemodavi_selector.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/root/Work/nemoapps/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object graph/CMakeFiles/nemodavi.dir/nemodavi_selector.c.o"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nemodavi.dir/nemodavi_selector.c.o   -c /home/root/Work/nemoapps/graph/nemodavi_selector.c

graph/CMakeFiles/nemodavi.dir/nemodavi_selector.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nemodavi.dir/nemodavi_selector.c.i"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/root/Work/nemoapps/graph/nemodavi_selector.c > CMakeFiles/nemodavi.dir/nemodavi_selector.c.i

graph/CMakeFiles/nemodavi.dir/nemodavi_selector.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nemodavi.dir/nemodavi_selector.c.s"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/root/Work/nemoapps/graph/nemodavi_selector.c -o CMakeFiles/nemodavi.dir/nemodavi_selector.c.s

graph/CMakeFiles/nemodavi.dir/nemodavi_selector.c.o.requires:

.PHONY : graph/CMakeFiles/nemodavi.dir/nemodavi_selector.c.o.requires

graph/CMakeFiles/nemodavi.dir/nemodavi_selector.c.o.provides: graph/CMakeFiles/nemodavi.dir/nemodavi_selector.c.o.requires
	$(MAKE) -f graph/CMakeFiles/nemodavi.dir/build.make graph/CMakeFiles/nemodavi.dir/nemodavi_selector.c.o.provides.build
.PHONY : graph/CMakeFiles/nemodavi.dir/nemodavi_selector.c.o.provides

graph/CMakeFiles/nemodavi.dir/nemodavi_selector.c.o.provides.build: graph/CMakeFiles/nemodavi.dir/nemodavi_selector.c.o


graph/CMakeFiles/nemodavi.dir/nemodavi_attr.c.o: graph/CMakeFiles/nemodavi.dir/flags.make
graph/CMakeFiles/nemodavi.dir/nemodavi_attr.c.o: ../graph/nemodavi_attr.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/root/Work/nemoapps/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object graph/CMakeFiles/nemodavi.dir/nemodavi_attr.c.o"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nemodavi.dir/nemodavi_attr.c.o   -c /home/root/Work/nemoapps/graph/nemodavi_attr.c

graph/CMakeFiles/nemodavi.dir/nemodavi_attr.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nemodavi.dir/nemodavi_attr.c.i"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/root/Work/nemoapps/graph/nemodavi_attr.c > CMakeFiles/nemodavi.dir/nemodavi_attr.c.i

graph/CMakeFiles/nemodavi.dir/nemodavi_attr.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nemodavi.dir/nemodavi_attr.c.s"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/root/Work/nemoapps/graph/nemodavi_attr.c -o CMakeFiles/nemodavi.dir/nemodavi_attr.c.s

graph/CMakeFiles/nemodavi.dir/nemodavi_attr.c.o.requires:

.PHONY : graph/CMakeFiles/nemodavi.dir/nemodavi_attr.c.o.requires

graph/CMakeFiles/nemodavi.dir/nemodavi_attr.c.o.provides: graph/CMakeFiles/nemodavi.dir/nemodavi_attr.c.o.requires
	$(MAKE) -f graph/CMakeFiles/nemodavi.dir/build.make graph/CMakeFiles/nemodavi.dir/nemodavi_attr.c.o.provides.build
.PHONY : graph/CMakeFiles/nemodavi.dir/nemodavi_attr.c.o.provides

graph/CMakeFiles/nemodavi.dir/nemodavi_attr.c.o.provides.build: graph/CMakeFiles/nemodavi.dir/nemodavi_attr.c.o


graph/CMakeFiles/nemodavi.dir/nemodavi_transition.c.o: graph/CMakeFiles/nemodavi.dir/flags.make
graph/CMakeFiles/nemodavi.dir/nemodavi_transition.c.o: ../graph/nemodavi_transition.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/root/Work/nemoapps/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object graph/CMakeFiles/nemodavi.dir/nemodavi_transition.c.o"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nemodavi.dir/nemodavi_transition.c.o   -c /home/root/Work/nemoapps/graph/nemodavi_transition.c

graph/CMakeFiles/nemodavi.dir/nemodavi_transition.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nemodavi.dir/nemodavi_transition.c.i"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/root/Work/nemoapps/graph/nemodavi_transition.c > CMakeFiles/nemodavi.dir/nemodavi_transition.c.i

graph/CMakeFiles/nemodavi.dir/nemodavi_transition.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nemodavi.dir/nemodavi_transition.c.s"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/root/Work/nemoapps/graph/nemodavi_transition.c -o CMakeFiles/nemodavi.dir/nemodavi_transition.c.s

graph/CMakeFiles/nemodavi.dir/nemodavi_transition.c.o.requires:

.PHONY : graph/CMakeFiles/nemodavi.dir/nemodavi_transition.c.o.requires

graph/CMakeFiles/nemodavi.dir/nemodavi_transition.c.o.provides: graph/CMakeFiles/nemodavi.dir/nemodavi_transition.c.o.requires
	$(MAKE) -f graph/CMakeFiles/nemodavi.dir/build.make graph/CMakeFiles/nemodavi.dir/nemodavi_transition.c.o.provides.build
.PHONY : graph/CMakeFiles/nemodavi.dir/nemodavi_transition.c.o.provides

graph/CMakeFiles/nemodavi.dir/nemodavi_transition.c.o.provides.build: graph/CMakeFiles/nemodavi.dir/nemodavi_transition.c.o


graph/CMakeFiles/nemodavi.dir/nemodavi_layout.c.o: graph/CMakeFiles/nemodavi.dir/flags.make
graph/CMakeFiles/nemodavi.dir/nemodavi_layout.c.o: ../graph/nemodavi_layout.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/root/Work/nemoapps/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object graph/CMakeFiles/nemodavi.dir/nemodavi_layout.c.o"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nemodavi.dir/nemodavi_layout.c.o   -c /home/root/Work/nemoapps/graph/nemodavi_layout.c

graph/CMakeFiles/nemodavi.dir/nemodavi_layout.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nemodavi.dir/nemodavi_layout.c.i"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/root/Work/nemoapps/graph/nemodavi_layout.c > CMakeFiles/nemodavi.dir/nemodavi_layout.c.i

graph/CMakeFiles/nemodavi.dir/nemodavi_layout.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nemodavi.dir/nemodavi_layout.c.s"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/root/Work/nemoapps/graph/nemodavi_layout.c -o CMakeFiles/nemodavi.dir/nemodavi_layout.c.s

graph/CMakeFiles/nemodavi.dir/nemodavi_layout.c.o.requires:

.PHONY : graph/CMakeFiles/nemodavi.dir/nemodavi_layout.c.o.requires

graph/CMakeFiles/nemodavi.dir/nemodavi_layout.c.o.provides: graph/CMakeFiles/nemodavi.dir/nemodavi_layout.c.o.requires
	$(MAKE) -f graph/CMakeFiles/nemodavi.dir/build.make graph/CMakeFiles/nemodavi.dir/nemodavi_layout.c.o.provides.build
.PHONY : graph/CMakeFiles/nemodavi.dir/nemodavi_layout.c.o.provides

graph/CMakeFiles/nemodavi.dir/nemodavi_layout.c.o.provides.build: graph/CMakeFiles/nemodavi.dir/nemodavi_layout.c.o


graph/CMakeFiles/nemodavi.dir/nemodavi_item.c.o: graph/CMakeFiles/nemodavi.dir/flags.make
graph/CMakeFiles/nemodavi.dir/nemodavi_item.c.o: ../graph/nemodavi_item.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/root/Work/nemoapps/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object graph/CMakeFiles/nemodavi.dir/nemodavi_item.c.o"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nemodavi.dir/nemodavi_item.c.o   -c /home/root/Work/nemoapps/graph/nemodavi_item.c

graph/CMakeFiles/nemodavi.dir/nemodavi_item.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nemodavi.dir/nemodavi_item.c.i"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/root/Work/nemoapps/graph/nemodavi_item.c > CMakeFiles/nemodavi.dir/nemodavi_item.c.i

graph/CMakeFiles/nemodavi.dir/nemodavi_item.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nemodavi.dir/nemodavi_item.c.s"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/root/Work/nemoapps/graph/nemodavi_item.c -o CMakeFiles/nemodavi.dir/nemodavi_item.c.s

graph/CMakeFiles/nemodavi.dir/nemodavi_item.c.o.requires:

.PHONY : graph/CMakeFiles/nemodavi.dir/nemodavi_item.c.o.requires

graph/CMakeFiles/nemodavi.dir/nemodavi_item.c.o.provides: graph/CMakeFiles/nemodavi.dir/nemodavi_item.c.o.requires
	$(MAKE) -f graph/CMakeFiles/nemodavi.dir/build.make graph/CMakeFiles/nemodavi.dir/nemodavi_item.c.o.provides.build
.PHONY : graph/CMakeFiles/nemodavi.dir/nemodavi_item.c.o.provides

graph/CMakeFiles/nemodavi.dir/nemodavi_item.c.o.provides.build: graph/CMakeFiles/nemodavi.dir/nemodavi_item.c.o


graph/CMakeFiles/nemodavi.dir/layouts/histogram.c.o: graph/CMakeFiles/nemodavi.dir/flags.make
graph/CMakeFiles/nemodavi.dir/layouts/histogram.c.o: ../graph/layouts/histogram.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/root/Work/nemoapps/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building C object graph/CMakeFiles/nemodavi.dir/layouts/histogram.c.o"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nemodavi.dir/layouts/histogram.c.o   -c /home/root/Work/nemoapps/graph/layouts/histogram.c

graph/CMakeFiles/nemodavi.dir/layouts/histogram.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nemodavi.dir/layouts/histogram.c.i"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/root/Work/nemoapps/graph/layouts/histogram.c > CMakeFiles/nemodavi.dir/layouts/histogram.c.i

graph/CMakeFiles/nemodavi.dir/layouts/histogram.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nemodavi.dir/layouts/histogram.c.s"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/root/Work/nemoapps/graph/layouts/histogram.c -o CMakeFiles/nemodavi.dir/layouts/histogram.c.s

graph/CMakeFiles/nemodavi.dir/layouts/histogram.c.o.requires:

.PHONY : graph/CMakeFiles/nemodavi.dir/layouts/histogram.c.o.requires

graph/CMakeFiles/nemodavi.dir/layouts/histogram.c.o.provides: graph/CMakeFiles/nemodavi.dir/layouts/histogram.c.o.requires
	$(MAKE) -f graph/CMakeFiles/nemodavi.dir/build.make graph/CMakeFiles/nemodavi.dir/layouts/histogram.c.o.provides.build
.PHONY : graph/CMakeFiles/nemodavi.dir/layouts/histogram.c.o.provides

graph/CMakeFiles/nemodavi.dir/layouts/histogram.c.o.provides.build: graph/CMakeFiles/nemodavi.dir/layouts/histogram.c.o


graph/CMakeFiles/nemodavi.dir/layouts/donut.c.o: graph/CMakeFiles/nemodavi.dir/flags.make
graph/CMakeFiles/nemodavi.dir/layouts/donut.c.o: ../graph/layouts/donut.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/root/Work/nemoapps/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building C object graph/CMakeFiles/nemodavi.dir/layouts/donut.c.o"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nemodavi.dir/layouts/donut.c.o   -c /home/root/Work/nemoapps/graph/layouts/donut.c

graph/CMakeFiles/nemodavi.dir/layouts/donut.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nemodavi.dir/layouts/donut.c.i"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/root/Work/nemoapps/graph/layouts/donut.c > CMakeFiles/nemodavi.dir/layouts/donut.c.i

graph/CMakeFiles/nemodavi.dir/layouts/donut.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nemodavi.dir/layouts/donut.c.s"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/root/Work/nemoapps/graph/layouts/donut.c -o CMakeFiles/nemodavi.dir/layouts/donut.c.s

graph/CMakeFiles/nemodavi.dir/layouts/donut.c.o.requires:

.PHONY : graph/CMakeFiles/nemodavi.dir/layouts/donut.c.o.requires

graph/CMakeFiles/nemodavi.dir/layouts/donut.c.o.provides: graph/CMakeFiles/nemodavi.dir/layouts/donut.c.o.requires
	$(MAKE) -f graph/CMakeFiles/nemodavi.dir/build.make graph/CMakeFiles/nemodavi.dir/layouts/donut.c.o.provides.build
.PHONY : graph/CMakeFiles/nemodavi.dir/layouts/donut.c.o.provides

graph/CMakeFiles/nemodavi.dir/layouts/donut.c.o.provides.build: graph/CMakeFiles/nemodavi.dir/layouts/donut.c.o


graph/CMakeFiles/nemodavi.dir/layouts/donutbar.c.o: graph/CMakeFiles/nemodavi.dir/flags.make
graph/CMakeFiles/nemodavi.dir/layouts/donutbar.c.o: ../graph/layouts/donutbar.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/root/Work/nemoapps/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building C object graph/CMakeFiles/nemodavi.dir/layouts/donutbar.c.o"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nemodavi.dir/layouts/donutbar.c.o   -c /home/root/Work/nemoapps/graph/layouts/donutbar.c

graph/CMakeFiles/nemodavi.dir/layouts/donutbar.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nemodavi.dir/layouts/donutbar.c.i"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/root/Work/nemoapps/graph/layouts/donutbar.c > CMakeFiles/nemodavi.dir/layouts/donutbar.c.i

graph/CMakeFiles/nemodavi.dir/layouts/donutbar.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nemodavi.dir/layouts/donutbar.c.s"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/root/Work/nemoapps/graph/layouts/donutbar.c -o CMakeFiles/nemodavi.dir/layouts/donutbar.c.s

graph/CMakeFiles/nemodavi.dir/layouts/donutbar.c.o.requires:

.PHONY : graph/CMakeFiles/nemodavi.dir/layouts/donutbar.c.o.requires

graph/CMakeFiles/nemodavi.dir/layouts/donutbar.c.o.provides: graph/CMakeFiles/nemodavi.dir/layouts/donutbar.c.o.requires
	$(MAKE) -f graph/CMakeFiles/nemodavi.dir/build.make graph/CMakeFiles/nemodavi.dir/layouts/donutbar.c.o.provides.build
.PHONY : graph/CMakeFiles/nemodavi.dir/layouts/donutbar.c.o.provides

graph/CMakeFiles/nemodavi.dir/layouts/donutbar.c.o.provides.build: graph/CMakeFiles/nemodavi.dir/layouts/donutbar.c.o


graph/CMakeFiles/nemodavi.dir/layouts/donutarray.c.o: graph/CMakeFiles/nemodavi.dir/flags.make
graph/CMakeFiles/nemodavi.dir/layouts/donutarray.c.o: ../graph/layouts/donutarray.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/root/Work/nemoapps/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Building C object graph/CMakeFiles/nemodavi.dir/layouts/donutarray.c.o"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nemodavi.dir/layouts/donutarray.c.o   -c /home/root/Work/nemoapps/graph/layouts/donutarray.c

graph/CMakeFiles/nemodavi.dir/layouts/donutarray.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nemodavi.dir/layouts/donutarray.c.i"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/root/Work/nemoapps/graph/layouts/donutarray.c > CMakeFiles/nemodavi.dir/layouts/donutarray.c.i

graph/CMakeFiles/nemodavi.dir/layouts/donutarray.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nemodavi.dir/layouts/donutarray.c.s"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/root/Work/nemoapps/graph/layouts/donutarray.c -o CMakeFiles/nemodavi.dir/layouts/donutarray.c.s

graph/CMakeFiles/nemodavi.dir/layouts/donutarray.c.o.requires:

.PHONY : graph/CMakeFiles/nemodavi.dir/layouts/donutarray.c.o.requires

graph/CMakeFiles/nemodavi.dir/layouts/donutarray.c.o.provides: graph/CMakeFiles/nemodavi.dir/layouts/donutarray.c.o.requires
	$(MAKE) -f graph/CMakeFiles/nemodavi.dir/build.make graph/CMakeFiles/nemodavi.dir/layouts/donutarray.c.o.provides.build
.PHONY : graph/CMakeFiles/nemodavi.dir/layouts/donutarray.c.o.provides

graph/CMakeFiles/nemodavi.dir/layouts/donutarray.c.o.provides.build: graph/CMakeFiles/nemodavi.dir/layouts/donutarray.c.o


graph/CMakeFiles/nemodavi.dir/layouts/arc.c.o: graph/CMakeFiles/nemodavi.dir/flags.make
graph/CMakeFiles/nemodavi.dir/layouts/arc.c.o: ../graph/layouts/arc.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/root/Work/nemoapps/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_12) "Building C object graph/CMakeFiles/nemodavi.dir/layouts/arc.c.o"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nemodavi.dir/layouts/arc.c.o   -c /home/root/Work/nemoapps/graph/layouts/arc.c

graph/CMakeFiles/nemodavi.dir/layouts/arc.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nemodavi.dir/layouts/arc.c.i"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/root/Work/nemoapps/graph/layouts/arc.c > CMakeFiles/nemodavi.dir/layouts/arc.c.i

graph/CMakeFiles/nemodavi.dir/layouts/arc.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nemodavi.dir/layouts/arc.c.s"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/root/Work/nemoapps/graph/layouts/arc.c -o CMakeFiles/nemodavi.dir/layouts/arc.c.s

graph/CMakeFiles/nemodavi.dir/layouts/arc.c.o.requires:

.PHONY : graph/CMakeFiles/nemodavi.dir/layouts/arc.c.o.requires

graph/CMakeFiles/nemodavi.dir/layouts/arc.c.o.provides: graph/CMakeFiles/nemodavi.dir/layouts/arc.c.o.requires
	$(MAKE) -f graph/CMakeFiles/nemodavi.dir/build.make graph/CMakeFiles/nemodavi.dir/layouts/arc.c.o.provides.build
.PHONY : graph/CMakeFiles/nemodavi.dir/layouts/arc.c.o.provides

graph/CMakeFiles/nemodavi.dir/layouts/arc.c.o.provides.build: graph/CMakeFiles/nemodavi.dir/layouts/arc.c.o


graph/CMakeFiles/nemodavi.dir/layouts/area.c.o: graph/CMakeFiles/nemodavi.dir/flags.make
graph/CMakeFiles/nemodavi.dir/layouts/area.c.o: ../graph/layouts/area.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/root/Work/nemoapps/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_13) "Building C object graph/CMakeFiles/nemodavi.dir/layouts/area.c.o"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nemodavi.dir/layouts/area.c.o   -c /home/root/Work/nemoapps/graph/layouts/area.c

graph/CMakeFiles/nemodavi.dir/layouts/area.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nemodavi.dir/layouts/area.c.i"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/root/Work/nemoapps/graph/layouts/area.c > CMakeFiles/nemodavi.dir/layouts/area.c.i

graph/CMakeFiles/nemodavi.dir/layouts/area.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nemodavi.dir/layouts/area.c.s"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/root/Work/nemoapps/graph/layouts/area.c -o CMakeFiles/nemodavi.dir/layouts/area.c.s

graph/CMakeFiles/nemodavi.dir/layouts/area.c.o.requires:

.PHONY : graph/CMakeFiles/nemodavi.dir/layouts/area.c.o.requires

graph/CMakeFiles/nemodavi.dir/layouts/area.c.o.provides: graph/CMakeFiles/nemodavi.dir/layouts/area.c.o.requires
	$(MAKE) -f graph/CMakeFiles/nemodavi.dir/build.make graph/CMakeFiles/nemodavi.dir/layouts/area.c.o.provides.build
.PHONY : graph/CMakeFiles/nemodavi.dir/layouts/area.c.o.provides

graph/CMakeFiles/nemodavi.dir/layouts/area.c.o.provides.build: graph/CMakeFiles/nemodavi.dir/layouts/area.c.o


graph/CMakeFiles/nemodavi.dir/layouts/test.c.o: graph/CMakeFiles/nemodavi.dir/flags.make
graph/CMakeFiles/nemodavi.dir/layouts/test.c.o: ../graph/layouts/test.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/root/Work/nemoapps/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_14) "Building C object graph/CMakeFiles/nemodavi.dir/layouts/test.c.o"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/nemodavi.dir/layouts/test.c.o   -c /home/root/Work/nemoapps/graph/layouts/test.c

graph/CMakeFiles/nemodavi.dir/layouts/test.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/nemodavi.dir/layouts/test.c.i"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/root/Work/nemoapps/graph/layouts/test.c > CMakeFiles/nemodavi.dir/layouts/test.c.i

graph/CMakeFiles/nemodavi.dir/layouts/test.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/nemodavi.dir/layouts/test.c.s"
	cd /home/root/Work/nemoapps/build/graph && /usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/root/Work/nemoapps/graph/layouts/test.c -o CMakeFiles/nemodavi.dir/layouts/test.c.s

graph/CMakeFiles/nemodavi.dir/layouts/test.c.o.requires:

.PHONY : graph/CMakeFiles/nemodavi.dir/layouts/test.c.o.requires

graph/CMakeFiles/nemodavi.dir/layouts/test.c.o.provides: graph/CMakeFiles/nemodavi.dir/layouts/test.c.o.requires
	$(MAKE) -f graph/CMakeFiles/nemodavi.dir/build.make graph/CMakeFiles/nemodavi.dir/layouts/test.c.o.provides.build
.PHONY : graph/CMakeFiles/nemodavi.dir/layouts/test.c.o.provides

graph/CMakeFiles/nemodavi.dir/layouts/test.c.o.provides.build: graph/CMakeFiles/nemodavi.dir/layouts/test.c.o


# Object files for target nemodavi
nemodavi_OBJECTS = \
"CMakeFiles/nemodavi.dir/nemodavi.c.o" \
"CMakeFiles/nemodavi.dir/nemodavi_data.c.o" \
"CMakeFiles/nemodavi.dir/nemodavi_selector.c.o" \
"CMakeFiles/nemodavi.dir/nemodavi_attr.c.o" \
"CMakeFiles/nemodavi.dir/nemodavi_transition.c.o" \
"CMakeFiles/nemodavi.dir/nemodavi_layout.c.o" \
"CMakeFiles/nemodavi.dir/nemodavi_item.c.o" \
"CMakeFiles/nemodavi.dir/layouts/histogram.c.o" \
"CMakeFiles/nemodavi.dir/layouts/donut.c.o" \
"CMakeFiles/nemodavi.dir/layouts/donutbar.c.o" \
"CMakeFiles/nemodavi.dir/layouts/donutarray.c.o" \
"CMakeFiles/nemodavi.dir/layouts/arc.c.o" \
"CMakeFiles/nemodavi.dir/layouts/area.c.o" \
"CMakeFiles/nemodavi.dir/layouts/test.c.o"

# External object files for target nemodavi
nemodavi_EXTERNAL_OBJECTS =

graph/libnemodavi.so: graph/CMakeFiles/nemodavi.dir/nemodavi.c.o
graph/libnemodavi.so: graph/CMakeFiles/nemodavi.dir/nemodavi_data.c.o
graph/libnemodavi.so: graph/CMakeFiles/nemodavi.dir/nemodavi_selector.c.o
graph/libnemodavi.so: graph/CMakeFiles/nemodavi.dir/nemodavi_attr.c.o
graph/libnemodavi.so: graph/CMakeFiles/nemodavi.dir/nemodavi_transition.c.o
graph/libnemodavi.so: graph/CMakeFiles/nemodavi.dir/nemodavi_layout.c.o
graph/libnemodavi.so: graph/CMakeFiles/nemodavi.dir/nemodavi_item.c.o
graph/libnemodavi.so: graph/CMakeFiles/nemodavi.dir/layouts/histogram.c.o
graph/libnemodavi.so: graph/CMakeFiles/nemodavi.dir/layouts/donut.c.o
graph/libnemodavi.so: graph/CMakeFiles/nemodavi.dir/layouts/donutbar.c.o
graph/libnemodavi.so: graph/CMakeFiles/nemodavi.dir/layouts/donutarray.c.o
graph/libnemodavi.so: graph/CMakeFiles/nemodavi.dir/layouts/arc.c.o
graph/libnemodavi.so: graph/CMakeFiles/nemodavi.dir/layouts/area.c.o
graph/libnemodavi.so: graph/CMakeFiles/nemodavi.dir/layouts/test.c.o
graph/libnemodavi.so: graph/CMakeFiles/nemodavi.dir/build.make
graph/libnemodavi.so: graph/CMakeFiles/nemodavi.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/root/Work/nemoapps/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_15) "Linking C shared library libnemodavi.so"
	cd /home/root/Work/nemoapps/build/graph && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/nemodavi.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
graph/CMakeFiles/nemodavi.dir/build: graph/libnemodavi.so

.PHONY : graph/CMakeFiles/nemodavi.dir/build

graph/CMakeFiles/nemodavi.dir/requires: graph/CMakeFiles/nemodavi.dir/nemodavi.c.o.requires
graph/CMakeFiles/nemodavi.dir/requires: graph/CMakeFiles/nemodavi.dir/nemodavi_data.c.o.requires
graph/CMakeFiles/nemodavi.dir/requires: graph/CMakeFiles/nemodavi.dir/nemodavi_selector.c.o.requires
graph/CMakeFiles/nemodavi.dir/requires: graph/CMakeFiles/nemodavi.dir/nemodavi_attr.c.o.requires
graph/CMakeFiles/nemodavi.dir/requires: graph/CMakeFiles/nemodavi.dir/nemodavi_transition.c.o.requires
graph/CMakeFiles/nemodavi.dir/requires: graph/CMakeFiles/nemodavi.dir/nemodavi_layout.c.o.requires
graph/CMakeFiles/nemodavi.dir/requires: graph/CMakeFiles/nemodavi.dir/nemodavi_item.c.o.requires
graph/CMakeFiles/nemodavi.dir/requires: graph/CMakeFiles/nemodavi.dir/layouts/histogram.c.o.requires
graph/CMakeFiles/nemodavi.dir/requires: graph/CMakeFiles/nemodavi.dir/layouts/donut.c.o.requires
graph/CMakeFiles/nemodavi.dir/requires: graph/CMakeFiles/nemodavi.dir/layouts/donutbar.c.o.requires
graph/CMakeFiles/nemodavi.dir/requires: graph/CMakeFiles/nemodavi.dir/layouts/donutarray.c.o.requires
graph/CMakeFiles/nemodavi.dir/requires: graph/CMakeFiles/nemodavi.dir/layouts/arc.c.o.requires
graph/CMakeFiles/nemodavi.dir/requires: graph/CMakeFiles/nemodavi.dir/layouts/area.c.o.requires
graph/CMakeFiles/nemodavi.dir/requires: graph/CMakeFiles/nemodavi.dir/layouts/test.c.o.requires

.PHONY : graph/CMakeFiles/nemodavi.dir/requires

graph/CMakeFiles/nemodavi.dir/clean:
	cd /home/root/Work/nemoapps/build/graph && $(CMAKE_COMMAND) -P CMakeFiles/nemodavi.dir/cmake_clean.cmake
.PHONY : graph/CMakeFiles/nemodavi.dir/clean

graph/CMakeFiles/nemodavi.dir/depend:
	cd /home/root/Work/nemoapps/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/root/Work/nemoapps /home/root/Work/nemoapps/graph /home/root/Work/nemoapps/build /home/root/Work/nemoapps/build/graph /home/root/Work/nemoapps/build/graph/CMakeFiles/nemodavi.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : graph/CMakeFiles/nemodavi.dir/depend

