#-----VARIABLES-----#

# Compiler utilities and flags #
HC=ghc
TARGET_FLAGS=-o
HC_FLAGS=-O --make

# Project files #
TARGET=pochoir
MAIN=PMain.hs 
PP_FILE=PMainParser.hs \
	PBasicParser.hs \
	PData.hs \
	PShow.hs \
	PUtils.hs \
	PParser2.hs

# Cleanup utilities, flags, and files #
RM=rm
RM_FLAGS=-f
RM_FILES=*.o *.hi

#-----RULES-----#

# Do everything #
all : $(TARGET)

# Compile the target #
$(TARGET) : $(MAIN) $(PP_FILE)
	$(HC) $(TARGET_FLAGS) $(TARGET) $(HC_FLAGS) $(MAIN)

# Clean up intermediate files #
clean : 
	$(RM) $(RM_FLAGS) $(RM_FILES) 

# Remove all intermediate files plus the target #
clobber : clean
	$(RM) $(RM_FLAGS) $(TARGET)