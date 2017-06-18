
# $(call toupper string)
toupper = $(shell echo $1|tr [:lower:] [:upper:])

CROSS := $(SDK_CROSS)
TOP_SRC_DIR := $(TOP_DIR)/src
SDK_BUILDDIR := $($(call toupper,$(SDK))_BUILDDIR)

CC := $(CROSS)gcc
CXX := $(CROSS)g++
AR := $(CROSS)ar
AS := $(CROSS)as
LD := $(CROSS)ld
NM := $(CROSS)nm
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump
STRIP := $(CROSS)strip
EUSTRIP := eu-strip
READELF := $(CROSS)readelf

CD := cd
MV := mv -f
RM := rm -rf
SED := sed
MKDIR := mkdir -p
TEST := test
CAT := cat
CP := cp -rfd
TOUCH := touch
CHMOD := chmod
FIND := find
GZIP := gzip
CPIO := cpio

MAKEFLAGS += --no-print-directory
CFLAGS += -m64 -fno-strict-aliasing -ggdb3 -O2 -fno-inline \
          -W -Wall -Werror -Wno-sign-compare -Wno-pointer-sign \
          -Wno-unused-parameter -Wno-format-zero-length
