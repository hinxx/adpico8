#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS += configure
DIRS += pico8App
#DIRS += vendor
#pico8App_DEPEND_DIRS += vendor

ifeq ($(BUILD_IOCS), YES)
DIRS += pico8DemoApp
pico8DemoApp_DEPEND_DIRS += pico8App
iocBoot_DEPEND_DIRS += pico8DemoApp
DIRS += iocBoot
endif

include $(TOP)/configure/RULES_TOP

