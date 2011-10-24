#-----PATH VARIABLES-----#

# Path variables #
PC_PATH=..
EXAMPLE_PATH=.

# Compiler variables #
PC=$(PC_PATH)/pochoir
CPP_FLAGS=-Wall -Werror -std=c++0x -o
PHASE_I_FLAGS=-O3 -DNDEBUG  -unroll-aggressive -funroll-loops -ipo\
 -xHOST -fno-alias -fno-fnalias $(CPP_FLAGS)
PHASE_II_FLAGS=-O0 -g3 -DDEBUG -debug $(CPP_FLAGS) 
# Default to the phase I flags
FLAGS=$(PHASE_I_FLAGS)

# File and target variables #
EXAMPLE_SOURCE=$(wildcard $(EXAMPLE_PATH)/*.cpp)

# Cleanup flags #
RM=rm
RM_FLAGS=-f
CLEAN_FILES=*.o *.i *_pochoir *_pochoir.cpp *.out


#-----COMPILE RULES-----#

help :
	@echo Available examples:
	@echo $(notdir $(basename $(EXAMPLE_SOURCE)))

% : %.cpp
	$(PC) $(FLAGS) $@ $<

phase-i : FLAGS=$(PHASE_I_FLAGS)

phase-ii : FLAGS=$(PHASE_II_FLAGS)

clean : 
	$(RM) $(RM_FLAGS) $(CLEAN_FILES)

clobber : clean
	$(RM) $(RM_FLAGS) $(EXAMPLES)

