# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

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
CMAKE_COMMAND = /opt/local/bin/cmake

# The command to remove a file.
RM = /opt/local/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/leo/Documents/genomics/relate

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/leo/Documents/genomics/relate/build

# Include any dependencies generated for this target.
include include/evaluate/CMakeFiles/RelateSelection.dir/depend.make

# Include the progress variables for this target.
include include/evaluate/CMakeFiles/RelateSelection.dir/progress.make

# Include the compile flags for this target's objects.
include include/evaluate/CMakeFiles/RelateSelection.dir/flags.make

include/evaluate/CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.o: include/evaluate/CMakeFiles/RelateSelection.dir/flags.make
include/evaluate/CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.o: ../include/evaluate/selection/RelateSelection.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/leo/Documents/genomics/relate/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object include/evaluate/CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.o"
	cd /Users/leo/Documents/genomics/relate/build/include/evaluate && /Library/Developer/CommandLineTools/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.o -c /Users/leo/Documents/genomics/relate/include/evaluate/selection/RelateSelection.cpp

include/evaluate/CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.i"
	cd /Users/leo/Documents/genomics/relate/build/include/evaluate && /Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/leo/Documents/genomics/relate/include/evaluate/selection/RelateSelection.cpp > CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.i

include/evaluate/CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.s"
	cd /Users/leo/Documents/genomics/relate/build/include/evaluate && /Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/leo/Documents/genomics/relate/include/evaluate/selection/RelateSelection.cpp -o CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.s

include/evaluate/CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.o.requires:

.PHONY : include/evaluate/CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.o.requires

include/evaluate/CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.o.provides: include/evaluate/CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.o.requires
	$(MAKE) -f include/evaluate/CMakeFiles/RelateSelection.dir/build.make include/evaluate/CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.o.provides.build
.PHONY : include/evaluate/CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.o.provides

include/evaluate/CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.o.provides.build: include/evaluate/CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.o


# Object files for target RelateSelection
RelateSelection_OBJECTS = \
"CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.o"

# External object files for target RelateSelection
RelateSelection_EXTERNAL_OBJECTS =

../bin/RelateSelection: include/evaluate/CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.o
../bin/RelateSelection: include/evaluate/CMakeFiles/RelateSelection.dir/build.make
../bin/RelateSelection: ../bin/librelateStatic.a
../bin/RelateSelection: ../bin/libgzstreamStatic.a
../bin/RelateSelection: /opt/local/lib/libz.dylib
../bin/RelateSelection: include/evaluate/CMakeFiles/RelateSelection.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/leo/Documents/genomics/relate/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../../bin/RelateSelection"
	cd /Users/leo/Documents/genomics/relate/build/include/evaluate && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/RelateSelection.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
include/evaluate/CMakeFiles/RelateSelection.dir/build: ../bin/RelateSelection

.PHONY : include/evaluate/CMakeFiles/RelateSelection.dir/build

include/evaluate/CMakeFiles/RelateSelection.dir/requires: include/evaluate/CMakeFiles/RelateSelection.dir/selection/RelateSelection.cpp.o.requires

.PHONY : include/evaluate/CMakeFiles/RelateSelection.dir/requires

include/evaluate/CMakeFiles/RelateSelection.dir/clean:
	cd /Users/leo/Documents/genomics/relate/build/include/evaluate && $(CMAKE_COMMAND) -P CMakeFiles/RelateSelection.dir/cmake_clean.cmake
.PHONY : include/evaluate/CMakeFiles/RelateSelection.dir/clean

include/evaluate/CMakeFiles/RelateSelection.dir/depend:
	cd /Users/leo/Documents/genomics/relate/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/leo/Documents/genomics/relate /Users/leo/Documents/genomics/relate/include/evaluate /Users/leo/Documents/genomics/relate/build /Users/leo/Documents/genomics/relate/build/include/evaluate /Users/leo/Documents/genomics/relate/build/include/evaluate/CMakeFiles/RelateSelection.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : include/evaluate/CMakeFiles/RelateSelection.dir/depend

