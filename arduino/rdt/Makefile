
include ../make/board.mk
include ../make/Arduino.mk

DIR_OBJ = build-$(BOARD_TAG)
TARGET = librdt.a


.PHONY: main compile clean


main: compile


compile: $(DIR_OBJ)/$(TARGET)


clean::
	@rm -rf $(DIR_OBJ)


$(DIR_OBJ)/$(TARGET): $(LOCAL_OBJS)
	@$(AR) rcs $@ $^