#==============================================================
#
#  Abs:  Makefile to build an IOC library for the 
#        Hytec ip-adc-8413 module
#
#  Name: Makefile
#
#  Side: None
#
#  Facility: SLAC/LCLS
#
#  Auth: 21-Aug-2006, K. Luchini     (LUCHINI)
#  Rev:  dd-mmm-yyyy, First Lastname (USERNAME)
#
#--------------------------------------------------------------
#  Mod:
#       dd-mmm-yyyy, First Lastname (USERNAME):
#         comments
#
#=============================================================
#
TOP=..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#========================================
#
# build an ioc application

LIBRARY_IOC_RTEMS = Hy8413
DBD += devHy8413.dbd

# Add locally compiled object code
Hy8413_SRCS += drvHy8413.c
Hy8413_SRCS += devAiHy8413.c
Hy8413_SRCS += devBiHy8413.c
Hy8413_SRCS += devBoHy8413.c
Hy8413_SRCS += devLiHy8413.c
Hy8413_SRCS += devMbbiHy8413.c
Hy8413_SRCS += devMbbiDirectHy8413.c
Hy8413_SRCS += devMbboHy8413.c
Hy8413_SRCS += devWfHy8413.c
Hy8413_SRCS += hytecIpm.c

#=======================================
include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

