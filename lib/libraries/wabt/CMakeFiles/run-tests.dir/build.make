# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.13

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
CMAKE_COMMAND = /root/led/1.8/bin/cmake

# The command to remove a file.
RM = /root/led/1.8/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/ledgis

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ledgis/build

# Utility rule file for run-tests.

# Include the progress variables for this target.
include libraries/wabt/CMakeFiles/run-tests.dir/progress.make

libraries/wabt/CMakeFiles/run-tests:
	cd /home/ledgis/libraries/wabt && /home/ledgis/build/wabt-unittests
	cd /home/ledgis/libraries/wabt && /usr/bin/python2.7 /home/ledgis/libraries/wabt/test/run-tests.py --bindir /home/ledgis/build

run-tests: libraries/wabt/CMakeFiles/run-tests
run-tests: libraries/wabt/CMakeFiles/run-tests.dir/build.make

.PHONY : run-tests

# Rule to build all files generated by this target.
libraries/wabt/CMakeFiles/run-tests.dir/build: run-tests

.PHONY : libraries/wabt/CMakeFiles/run-tests.dir/build

libraries/wabt/CMakeFiles/run-tests.dir/clean:
	cd /home/ledgis/build/libraries/wabt && $(CMAKE_COMMAND) -P CMakeFiles/run-tests.dir/cmake_clean.cmake
.PHONY : libraries/wabt/CMakeFiles/run-tests.dir/clean

libraries/wabt/CMakeFiles/run-tests.dir/depend:
	cd /home/ledgis/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ledgis /home/ledgis/libraries/wabt /home/ledgis/build /home/ledgis/build/libraries/wabt /home/ledgis/build/libraries/wabt/CMakeFiles/run-tests.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : libraries/wabt/CMakeFiles/run-tests.dir/depend

