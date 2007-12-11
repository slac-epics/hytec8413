#==============================================================
#
#  Abs:  RTEMS Script to Initialize IPAC ADC hardware
#
#  Name: init_hytec8413.cmd
#
#  Facility:  LCLS Magnet Controls
#
#  Auth: 05-Dec-2007, Kristi Luchini  (LUCHINI)
#  Rev:  dd-mmm-yyyy, Reviewer's Name (USERNAME)
#--------------------------------------------------------------
#  Mod:
#       dd-mmm-yyyy, First Lastname   (USERNAME):
#         add comment
#
#==============================================================
#
# Initialize 8413 adc's
# Input arguments are: name, carrier, slot, csr, vector
#
# WARNING: set bspExtVerbosity to 3 to slow down processor because
# otherwise the IDprom name  of this modules is not returned correctly.
# This check is done in the function hytec_ipmValidate() which
# is in the source file hytecIpm.c
#
bspExtVerbosity=3
debugHy8413=0
ip8413Create("ai0",0,0,0,0)
ip8413Create("ai1",1,0,0,0)
ip8413Create("ai2",2,0,0,0)
ip8413Create("ai3",3,0,0,0)

# For ai0 only:
# set clock rate to 3.8KHz
# and then place module in SAM mode
debugHy8413=1
drvHy8413_wt_clk_rate( 0x9fff0000,10 )
drvHy8413_init_sam_mode( 0x9f0000 )
debugHy8413=0

# End of script

