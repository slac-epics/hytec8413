#==============================================================
#
#  Name: Makefile to install EDM displays
#
#  Auth: 05-Dec-2007, Kristi Luchini  (USERNAME)
#  Rev:  dd-mmm-yyyy, First Lastname  (USERNAME)
#--------------------------------------------------------------
#  Mod:
#        dd-mmm-yyyy, First Lastname   (USERNAME):
#          comment
#
#==============================================================
#
T_A=$(EPICS_HOST_ARCH)
TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

# EDM Screens to install
# You can override the default installation path.
#INSTALL_LOCATION=

EDLS += ip8413.edl
EDLS += ip8413_v2.edl

# Install display script
SCRIPTS += ip8413_display

# Install images to be used by EDM (Gif and Png are supported)
# PNG is better and preferred.
#PNGS +=
#GIFS +=

# EDM Screens to install
# If you want to install all screens and are
# lazy use the wildcard method. :)
#EDLS+=$(wildcard *.edl)
#PNGS+=$(wildcard *.png)
#GIFS+=$(wildcard *.gif)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
