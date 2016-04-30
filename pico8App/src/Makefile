TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

LIBRARY_IOC += pico8

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# install pico8.dbd into <top>/dbd
DBD += pico8Support.dbd

# specify all source files to be compiled and added to the library
pico8_SRCS += pico8.cpp

pico8_LIBS += $(EPICS_BASE_IOC_LIBS)

USR_INCLUDES += -I$(AMCPICODRV)

include $(ADCORE)/ADApp/commonLibraryMakefile

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

