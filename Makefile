#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS += configure
DIRS += hytec8413App
#DIRS += hytec8413TestApp
#DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *App))
#DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *app))

include $(TOP)/configure/RULES_TOP
