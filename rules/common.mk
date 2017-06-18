# $(call find_sub_dir dir file)
find_sub_dir = $(patsubst %/,%,$(sort $(dir $(filter-out $1/$2,\
                   $(shell find $1 -name $2)))))

# $(call filter_out_substr subs strings)
filter_out_substr = $(filter-out $(addsuffix %,$1),$2)

SUBMAKE_DIRS := $(call find_sub_dir,$(SRC_DIR),Makefile)
SUBMOD_DIRS := $(call find_sub_dir,$(SRC_DIR),module.mk)
SOURCES := $(call filter_out_substr,$(SUBMOD_DIRS),\
              $(call filter_out_substr,$(SUBMAKE_DIRS),\
                  $(shell find $(SRC_DIR) -name \*.c)))

TARGET := $(strip $(TARGET))
TARGET_LIBA := $(filter lib%.a,$(TARGET))
TARGET_LIBSO := $(filter lib%.so,$(TARGET))
TARGET_OBJ := $(filter %.o,$(TARGET))
TARGET_BIN := $(filter-out $(TARGET_LIBA) $(TARGET_LIBSO) $(TARGET_OBJ),$(TARGET))

vpath %.h $(INCLUDE_DIRS)
vpath %.c $(SRC_DIR)
vpath lib%.a $(LIB_DIRS)
vpath lib%.so $(LIB_DIRS)

CPPFLAGS += $(addprefix -I ,$(INCLUDE_DIRS))

.PHONY: all clean $(SUBMAKE_DIRS)

all:

include $(addsuffix /module.mk,$(SUBMOD_DIRS))

	echo $(SRC_DIR)
OBJECTS = $(subst .c,.o, $(patsubst $(SRC_DIR)/%,%,$(SOURCES)))
OBJECT_DIRS = $(sort $(dir $(OBJECTS)))
DEPENDS = $(subst .o,.d,$(OBJECTS))

ifneq ($(MAKECMDGOALS),clean)
create_build_dirs := $(shell for f in $(OBJECT_DIRS);\
                         do\
                             $(TEST) -d $$f || $(MKDIR) $$f;\
                         done)
-include $(DEPENDS)
endif

all: $(TARGET) $(SUBMAKE_DIRS)

clean: $(SUBMAKE_DIRS)
	$(RM) $(TARGET) $(OBJECTS) $(DEPENDS)

$(TARGET): $(SUBMAKE_DIRS)
$(SUBMAKE_DIRS): subbuild_dir = $(CURDIR)/$(patsubst $(SRC_DIR)/%,%,$@)
$(SUBMAKE_DIRS):
ifneq ($(MAKECMDGOALS),clean)
	$(TEST) -d $(subbuild_dir) || $(MKDIR) $(subbuild_dir)
endif
	$(TEST) -d $(subbuild_dir) && $(MAKE) --file=$@/Makefile --directory=$(subbuild_dir) $(MAKECMDGOALS)

ifneq ($(TARGET_LIBA),)
$(TARGET_LIBA): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $^
endif

ifneq ($(TARGET_LIBSO),)
$(TARGET_LIBSO): CFLAGS += -shared -fPIC
$(TARGET_LIBSO): $(OBJECTS)
endif

ifneq ($(TARGET_OBJ),)
$(TARGET_OBJ): CFLAGS += -nostdlib -r
$(TARGET_OBJ): $(OBJECTS)
endif

ifneq ($(TARGET_BIN),)
$(TARGET_BIN): $(OBJECTS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
endif

%.d: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M $< |\
      $(SED) 's,\($(notdir $*)\.o\) *:,$(dir $@)\1 $@: ,' > $@.tmp
	$(MV) $@.tmp $@
