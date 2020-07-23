
#---------------------------------------------------
# Paths
#---------------------------------------------------

vpath %.h include
vpath %.cpp src
vpath %.o build

# Headers directory
DIR_INC := include
# Src directory
DIR_SRC := src
# Build directory
DIR_OBJ := build
# Bin directory
DIR_BIN := bin
# Docs directory
DIR_DOC := docs/doxygen


#---------------------------------------------------
# Files
#---------------------------------------------------

# Paths of all the cpp file
SRC_PATHS = $(shell find ./ -type f -name \*.$(SRC_EXT))
# File name of all the file
SOURCES = $(notdir $(SRC_PATHS))
# Objects files of all the source files
OBJECTS = $(addprefix $(DIR_OBJ)/,$(SOURCES:.$(SRC_EXT)=.o))

# Doxygen input file
DOXYFILE = docs/Doxyfile.in
# Extra option to prevent prints when generating Doxygen
DOXYFLAGS = > /dev/null 2> /dev/null


#---------------------------------------------------
# Flags
#---------------------------------------------------

# Phony targets are always executed
.PHONY: main help compile docs clean

# Compiler
CC := g++
#Archiver
AR := ar rcs
# Source extension
SRC_EXT := cpp
# Library extension
LIB_EXT := a
# Compilation options
CFLAGS = -fPIC -Wall -Wextra -pedantic -g -O0
# Include paths
CPPFLAGS = -I$(DIR_INC)
#Target library
TARGET = librdt.$(LIB_EXT)

# Doxygen command
DOXYGEN = doxygen


#---------------------------------------------------
# Phony Rules
#---------------------------------------------------

main: compile

help:
	@echo "List of all accepted targets from command line."
	@echo ""
	@echo "compile \t compile the program"
	@echo "docs \t\t generates doxygen documentation"
	@echo "clean \t\t clears the build tree"
	@echo ""

# Default compilation command
compile: $(DIR_BIN)/$(TARGET)


docs:
	@mkdir -p $(DIR_DOC)
	@echo "Creating documentation..."
	@$(DOXYGEN) $(DOXYFILE) $(DOXYFLAGS)


# Clean all make sub-products
clean::
	@echo "Deleting: $(DIR_OBJ) $(DIR_BIN) $(DIR_DOC)..."
	@rm -rf $(DIR_OBJ) $(DIR_BIN) $(DIR_DOC)


#---------------------------------------------------
# File-specific Rules
#---------------------------------------------------

$(DIR_BIN)/$(TARGET): $(OBJECTS)
	@mkdir -p $(DIR_BIN)
	@echo "Creating static library:\nGenerating $@ from $^..."
	@$(AR) $@ $^


#---------------------------------------------------
# Generic Rules
#---------------------------------------------------

$(DIR_OBJ)/%.o: %.$(SRC_EXT)
	@mkdir -p $(DIR_OBJ)
	@echo "Compiling Phase:\nGenerating $@ from $<..."
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<





