#==============================================================
#
#  Abs:  Makefile to generate databases for the a 
#        Hytec ip-adc-8413 module
#
#  Name: Makefile
#
#  Auth: 05-Dec-2007, K. Luchini     (LUCHINI)
#  Rev:  dd-mmm-yyyy, First Lastname (USERNAME)
#--------------------------------------------------------------
#  Mod:
#       dd-mmm-yyyy, First Lastname (USERNAME):
#         comments
#
#=============================================================
#
TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#----------------------------------------------------
#  Optimization of db files using dbst (DEFAULT: NO)
#DB_OPT = YES

#----------------------------------------------------
# Create and install (or just install)
# databases, templates, substitutions like this
#
DB += ip8413.db
DB += ip8413_v2.db
DB += ip231.db

#----------------------------------------------------
# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
# 

