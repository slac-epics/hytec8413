#==============================================================
#
#  Abs:  EPICS RTEMS Startup Script (Example script)
#
#  Name: st.cmd
#
#  Auth: 05-Dec-2007, Kristi Luchini  (LUCHINI)
#  Rev:  dd-mmm-yyyy, Reviewer's Name (USERNAME)
#--------------------------------------------------------------
#  Mod:
#       dd-mmm-yyyy, First Lastname   (USERNAME):
#         comment
#
#==============================================================
#
# Change directory to TOP of application
cd("../..")

# Load application object file.
ld("bin/RTEMS-beatnik/hy8413Test.obj")

# Load EPICS Database      
dbLoadDatabase("dbd/hy8413Test.dbd")
hytec8413Test_registerRecordDeviceDriver(pdbbase)

# Allows gdb to attach to this target
#rtems_gdb_start(0,0)

# Load databases
dbLoadRecords("dbd/ip8413_v2.db")
dbLoadRecords("dbd/ip231.db")

# Initialized ip carriers
# Input arguments are: card info, A16 address
ipacAddCarrier( &xy9660,"0x0000")

# Initialize ip231 dac's
# Input arguments are:  name, carrier, slot, mode
IP231_DRV_DEBUG=0
ip231Create("ao0",0,1,"transparent")

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

# set clock rate to 3.8KHz
# and then place module in SAM mode
debugHy8413=1
drvHy8413_wt_clk_rate( 0x9fff0000,10 )
drvHy8413_init_sam_mode( 0x9f0000 )
debugHy8413=0
  
# Initialize EPICS        
iocInit()
bspExtVerbosity=0

# End of script

