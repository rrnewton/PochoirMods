#-----PATH VARIABLES-----#

# Path variables #
PC_PATH=..
EXAMPLE_PATH=.

# Compiler variables #
CC=
PC=$(PC_PATH)/pochoir
ICC=icpc
CPP_FLAGS=-Wall -Werror -std=c++0x -o
PHASE_I_FLAGS=-O3 -DNDEBUG  -unroll-aggressive -funroll-loops -ipo\
 -xHOST -fno-alias -fno-fnalias $(CPP_FLAGS)
PHASE_II_FLAGS=-O0 -g3 -DDEBUG -debug $(CPP_FLAGS) 
# Default to the phase I flags
FLAGS=$(PHASE_I_FLAGS)

# File and target variables #
EXAMPLE_SOURCE=$(wildcard $(EXAMPLE_PATH)/*.cpp)
TARGETS=$(basename $(EXAMPLE_SOURCE))

# Cleanup flags #
RM=rm
RM_FLAGS=-f
CLEAN_FILES=*.o *.i *_pochoir *_pochoir.cpp *.out


#-----COMPILE RULES-----#

all : $(TARGETS)

help :
	@echo Available examples:
	@for EX in $(notdir $(TARGETS)); do echo "\t$$EX"; done;

% : %.cpp 
	$(ICC) $(PHASE_I_FLAGS) $@ $<
	@if [ $$? -eq 0 ]
	@then
		$(PC) $(PHASE_II_FLAGS) $@ $<
	@else
	@	echo Phase I compilation failed for $@.
	@	echo Aborting compilation.
	@	echo No Pochoir Guarantee(TM) here
	@fi

clean : 
	$(RM) $(RM_FLAGS) $(CLEAN_FILES)

clobber : clean
	$(RM) $(RM_FLAGS) $(EXAMPLES)

