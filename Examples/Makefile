#----------------------------------------------------------------------
# File Makefile: The Reckoning
# Written by Chris Frisz
# Created 30 Nov 2011
# Last updated 30 Nov 2011
#
# Builds the example programs for the Pochoir stencil compiler
#
# This time let's do it right
#----------------------------------------------------------------------

#----- VARIABLES -----#

#- Path and compiler variables -#

# Pochoir depends on an environment variable that isn't documented
# Since the repo comes packaged with Pochoir in the parent directory,
# let's just assume that it's in the parent directory
ifndef $(POCHOIR_LIB_PATH)
	export POCHOIR_LIB_PATH=..
endif

# Pochoir compiler
PC=$(POCHOIR_LIB_PATH)/pochoir

#- Compiler flags -#

# Common flags
WARNING_FLAGS=-Wall -Werror
UNROLL_FLAGS=-unroll-aggressive -funroll-loops
OPT_FLAGS=$(UNROLL_FLAGS) -ipo
ALIAS_FLAGS=-fno-alias -fno-fnalias
CPP_STD_FLAG=-std=c++0x

# Phase flags
VERIFY_FLAGS=-O0 -g3 -DDEBUG -debug $(WARNING_FLAGS) $(CPP_STD_FLAG)
COMPILE_FLAGS=-O3 -DNDEBUG $(WARNING_FLAGS) $(OPT_FLAGS) -xHOST $(ALIAS_FLAGS) $(CPP_STD_FLAG)
CURRENT_FLAGS=$(COMPILE_FLAGS)


#- Cleanup variables -#
RM=rm
RM_FLAGS=-f
INTERMED_FILES=*.o *.i *_pochoir.cpp *.out
FINAL_FILES=*_pochoir


#----- RULES -----#

# Build a given example
% : %.cpp
	$(PC) $(CURRENT_FLAGS) $< -o $@_pochoir
	@echo "NOTE: Executable is named $@_pochoir"

verify : 
	$(eval CURRENT_FLAGS=$(VERIFY_FLAGS))
	@echo "Performing Phase I (verification) compilation"

# Set the flags for the compile (phase II) compilation (default)
compile : 
	$(eval CURRENT_FLAGS=$(COMPILE_FLAGS))
	@echo "Performing Phase II (final) compilation"

# Tacky, but so is having the necessary environment variable
set-path : 
	export POCHOIR_LIB_PATH=$(POCHOIR_LIB_PATH)

# Clean up intermediate files
clean :
	$(RM) $(RM_FLAGS) $(INTERMED_FILES)

# Clean up all files
clobber : clean
	$(RM) $(RM_FLAGS) $(FINAL_FILES)
