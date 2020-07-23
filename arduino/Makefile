
include make/board.mk

DIR_OBJ = build-$(BOARD_TAG)
TARGET = librdt.a
DIR_LIB = rdt

LIB_ARCHIVE = $(DIR_OBJ)/$(TARGET)

CXXFLAGS += -I$(DIR_LIB)

OTHER_OBJS = $(DIR_LIB)/$(LIB_ARCHIVE)


include make/Arduino.mk


$(DIR_LIB)/$(LIB_ARCHIVE):
	@if [ ! -f "$@" ] ; then \
	$(MAKE) -C $(DIR_LIB) $(LIB_ARCHIVE) ; \
	fi


clean::
	@rm -rf $(DIR_OBJ)


cleanall:
	@rm -rf $(DIR_OBJ)
	$(MAKE) -C $(DIR_LIB) clean
