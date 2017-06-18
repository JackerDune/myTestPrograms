SRC_DIR := $(shell readlink -f $(patsubst %/,%,$(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))))

TOP_DIR := $(SRC_DIR)/..

include $(TOP_DIR)/rules/public.mk
TARGET := main 

include $(TOP_DIR)/rules/common.mk
