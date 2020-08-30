### A universal Makefile for GNU Make
# default build configuration
CFG ?= config.mk

# define some useful variables
MAKEFILE_PATH := $(abspath $(firstword $(MAKEFILE_LIST)))
MAKEFILE_DIRPATH := $(dir $(MAKEFILE_PATH))
MAKEFILE_DIR := $(MAKEFILE_DIRPATH:/=)
MAKEFILE_DIRNAME := $(notdir $(MAKEFILE_DIR))
START_DIR := $(PWD:/=)
START_DIRNAME := $(notdir $(START_DIR))

include $(MAKEFILE_DIR)/$(CFG)
include $(MAKEFILE_DIR)/proj.mk
include $(MAKEFILE_DIR)/core.mk # magic happens here (as much as ugliness)
-include $(MAKEFILE_DIR)/custom.mk # custom rules
