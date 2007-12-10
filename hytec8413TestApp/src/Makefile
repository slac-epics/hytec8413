#==============================================================
#
#  Abs:  Makefile to build EPICS Test Application
#
#  Name: Makefile
#
#  Auth: 05-Dec-2007, K. Luchini          (LUCHINI)
#  Rev:  dd-mmm-yyyy, Reviewer's Name     (USERNAME)
#--------------------------------------------------------------
#  Mod:
#        dd-mmm-yyyy, First Lastname      (USERNAME):
#          comments
#
#==============================================================
#
TOP=../../
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#========================================
#
# build an object
PROD_RTEMS = testHy8413

# for gdb/ddd debugger:
USR_CFLAGS_DEFAULT += -DHOST_TEST -g
USR_CFLAGS_RTEMS   += -g -O4

# Build the ioc database
# Build the database definition file.  
DBD += testHy8413.dbd 

testHy8413_DBD += base.dbd
testHy8413_DBD += drvXy9660.dbd
testHy8413_DBD += drvIpac.dbd 
testHy8413_DBD += devHy8413.dbd
testHy8413_DBD += IP231.dbd

# The <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
testHy8413_SRCS          += testHy8413_registerRecordDeviceDriver.cpp
testHy8413_SRCS_DEFAULT  += testHy8413Main.cpp
testHy8413_SRCS_vxWorks  += -nil-
testHy8413_SRCS_RTEMS    += -nil-

# The following adds support from base/src/vxWorks
testHy8413_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

testHy8413_LIBS += IP231
testHy8413_LIBS += Hy8413
testHy8413_LIBS += Ipac

testHy8413_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
#========================================
#
# End of file