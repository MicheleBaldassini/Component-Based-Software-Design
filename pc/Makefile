
#---------------------------------------------------
# Paths
#---------------------------------------------------

# Library directory
DIR_LIB := rdt/bin
# Build directory
DIR_OBJ := build


#---------------------------------------------------
# Files
#---------------------------------------------------

# Target file
TARGET = test
# Paths of all the cpp file
PATHS = $(shell ls *.cpp)
# File name of all the file
SOURCES = $(notdir $(PATHS))
# Objects files of all the source files
OBJECTS = $(addprefix $(DIR_OBJ)/,$(SOURCES:.$(SRC_EXT)=.o))


#---------------------------------------------------
# Flags
#---------------------------------------------------

# Phony tagets are always executed
.PHONY: main compile clean cleanall

# Compiler
CC := g++
# Source extension
SRC_EXT := cpp
# Compilation options
CFLAGS = -Wall -Wextra -pedantic -g -O0
# Third part Library paths
LDFLAGS := -L$(DIR_LIB)
# Linking options
LDLIBS := -lpthread -lm -lrt -lrdt


#---------------------------------------------------
# Phony Rules
#---------------------------------------------------

main: compile

# Default compilation command
compile: $(TARGET)

# Clean all make sub-products
clean::
	@echo "Deleting: $(TARGET)..."
	@rm -rf $(TARGET)

cleanall:
	@rm -rf $(TARGET)
	$(MAKE) -C rdt clean



#---------------------------------------------------
# File-specific Rules
#---------------------------------------------------

$(TARGET): $(OBJECTS)
	@if [ ! -f "$@" ] ; \
	then $(MAKE) -C rdt compile ; \
	fi
	@echo "Linking Phase:\nGenerating $@ from $^..."
	@$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@
	@rm -rf $(DIR_OBJ)


#---------------------------------------------------
# Generic Rules
#---------------------------------------------------

$(DIR_OBJ)/%.o: %.$(SRC_EXT)
	@mkdir -p $(DIR_OBJ)
	@echo "Compiling Phase:\nGenerating $@ from $<..."
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<





