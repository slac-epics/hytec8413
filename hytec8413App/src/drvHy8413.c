/*
=============================================================
 
  Abs:  Driver Support for a VME Hytec ip-adc-8413 module
 
  Name: drvHy8413.c
          *  drvHy8413_init_driver - Register init adc's with EPICS
          *  drvHy8413_io_report   - Report information of all cards.
          *  drvHy8413_dump        - Report information of a single card
             drvHy8413_dump_data   - Report adc data of a single card
             drvHy8413_rd          - Read specified channel data
             ip8413Create          - Module specific Wrapper for hyec_addIpAdc()
 
          * indicates static routines
  
  Proto: drvHy8413Lib.h
 
  Auth: 27-Aug-2006, K. Luchini       (LUCHINI)
  Rev : dd-mmm-yyyy, Reviewer's Name  (USERNAME)
 
-------------------------------------------------------------
  Mod:  
        08-Nov-2006, K. Luchini (LUCHINI):
           Fix formatting error in hy8413_dump         
 
=============================================================
*/
 
/* Header Files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "epicsVersion.h"
#include "epicsMutex.h"
#include "epicsString.h"
#include "epicsInterrupt.h"
#include "cantProceed.h"
#include "drvSup.h"
#include "dbScan.h"
#include "drvIpac.h"
#include "drvHy8413.h"
#include "hytecIpm.h"
#include "hytecIpmLib.h"
#include "drvHy8413Lib.h" 
#include "epicsExport.h"     

/* Local Prototypes */
static long drvHy8413_init_driver( void );
static long drvHy8413_io_report( int level );
static void drvHy8413_dump( int level, 
                            hytec_ipmConfig_ts const * const card_ps );

static const float drvHy8413_ver = 1.0;

/*
 * Global structure for the
 * Driver Entry Table
 */
int debugHy8413 = 0;
struct {
   long        number;
   DRVSUPFUN   report;
   DRVSUPFUN   init;
}drvHy8413 = { 2,
               drvHy8413_io_report,
               drvHy8413_init_driver };

epicsExportAddress(drvet,drvHy8413);
 
/*====================================================
 
  Abs:  Initialize all Hytec ip-adc-8413 modules
 
  Name: drvHy8413_init_driver
 
  Args: None
 
  Rem: This function is called once during ioc
       initialization. Its purpose is first
       determine which ip-adc-8413 module(s) are
       present in the local ioc and then to
       perform the initialization sequence on
       each.
 
  Side: None
 
  Ret:  long
            OK  - Successful operation (always)
 
=======================================================*/
static long drvHy8413_init_driver( void )
{
   long      status  = OK;
   IPADC_ID  card_ps = NULL;

  
  /* Process each card in the list */
   card_ps = (IPADC_ID)hytec_ipmGetFirst();
   while( card_ps ) 
   {
      card_ps->init = 1;
      card_ps = (IPADC_ID)ellNext((ELLNODE *)card_ps);
   }/* End of while statement */
   return(status);
}

/*====================================================
 
  Abs:  Display data for all Hytec ip-adc-8413 Modules
 
  Name: drvHy8413_io_report
 
  Args: level                        Level of info tobe
          Type: integer              displayed.
          Use:  int
          Acc:  read-only
          Mech: By value
 
  Rem:  The report routine is called by the dbior,
        an ioc test utilities provided by EPICS.
        It is responsible for producing a report 
        describing the hardware found at initialization.
        One input argument is passed, which describes the
        "level" of information to be displayed.

        This report is output to the stdout
        when the following is entered from the
        target shell:
 
            dbior ("drvHy8413",<interest level>)
 
  Side: Report is sent to the standard output device
  
  Ret:  long
           OK  - Successful operation
            
=======================================================*/
static long drvHy8413_io_report( int level )
{
  long      status  = OK; 
  IPADC_ID  card_ps = NULL;
 
  /*
   * Are VHQ 203M modules present?
   * If yes, then display information
   * for each card.
   */
   card_ps = (IPADC_ID)hytec_ipmGetFirst();
   if ( !card_ps ) 
      printf("No Hytec IP-ADC-8413 modules are present\n");
   else
   {
     printf("IP8413 driver V%f\n\n",drvHy8413_ver);
     while ( card_ps )
     {
        if ( card_ps->model==HYTEC_IP8413_MODEL )
	  drvHy8413_dump( level,card_ps );
        card_ps = (IPADC_ID)ellNext((ELLNODE *)card_ps);
     } /* End of WHILE statement */
   }
   return( status );
}
    
/*====================================================
 
  Abs:  Display data for a single Hytec ip-adc-8413 Module
 
  Name: drvHy8413_dump
 
  Args: level                        Level of info tobe
          Type: integer              displayed.
          Use:  int
          Acc:  read-only
          Mech: By value

        card_ps                      Card configuration info
          Type: struct           
          Use:  hytec_ipmConfig_ts *
          Acc:  read-only
          Mech: By reference

  Rem:  The purpose of this function is to display
        module information. The detail of inforamtion 
        is dependent upon the level argument specified.
        The higher the level the more detailed the information
 
        The level of detail is as follows:
          
          Level  Report Informati Displayed
          -----  ---------------------------
            0    base io and mem addr, model, num of chans
            1    base io and mem addr, model, and status register
            2    base io amd mem addr, model, and all registers
 
  Side: Report is sent to the standard output device
  
  Ret:  None
            
=======================================================*/ 
static void drvHy8413_dump( int  level, 
                            hytec_ipmConfig_ts const * const card_ps )
{
  HY8413_IO          io_ps=NULL;
  unsigned short     mode = 0;
  unsigned long      addr=(unsigned long)card_ps->io_p;
  static const char *op_apc[2]={"Standby","Normal Operating"};


  io_ps = (HY8413_IO)card_ps->io_p;
  printf("\tIP8413 (ID=%hd)  %s is installed on carrier %hd slot %hd\n",
         card_ps->id_ps->modelId,
         card_ps->name_c,
         card_ps->carrier,
         card_ps->slot );

  if (level==1) 
  {
    if ( io_ps->acr & HY8413_ACR_NS ) mode=1;
    printf("\tIn total %hd channels,  ADC is in %s Mode\n",
           card_ps->nchan,
           op_apc[mode] );
    printf("\tIO space is at 0x%lx  csr: 0x%hx  acr: 0x%hx\n",
           addr,
           io_ps->csr,
           io_ps->acr );
    if (card_ps->init) 
      printf("\tInitialized Successfully\n");
    else
      printf("\tFailed Initialization\n");
  }

  if (level==2)
  {
    /* display adc data */
    drvHy8413_dump_data( (void const * const)card_ps->io_p );
  }
  return;
}


/*====================================================
 
  Abs:  Display data for a single Hytec ip-adc-8413 Module
 
  Name: drvHy8413_dump
 
  Args: io_p                        Io base address
          Type: address        
          Use:  void  *
          Acc:  read-only
          Mech: By reference

  Rem:  The purpose of this function is to display
        module adc data.
 
  Side: Sent to standard output device
  
  Ret:  None
            
=======================================================*/ 
void drvHy8413_dump_data( void const * const io_p )
{
  unsigned short i;
  unsigned short nchan = HY8413_NUM_CHAN/2;
  HY8413_IO       io_ps=(HY8413_IO)io_p;

  if ( !io_ps ) return;

  printf("\t: "); 
  for (i=0; i<nchan; i++) 
  {
    printf(" ch%d=%.2hd  ",i,io_ps->data_s.adc_a[i]);
  }/* End of FOR loop */

  printf("\n\t");
  nchan = HY8413_NUM_CHAN;
  for (; i<nchan; i++) 
  {
    printf("ch%d=%.2hd  ",i,io_ps->data_s.adc_a[i]);
  }/* End of FOR loop */
  printf("\n");
  return;  
}


/*====================================================
 
  Abs:  Read specified adc channel data
 
  Name: drvHy8413_rd
 
  Args: dpvt_p                          Card infomramtion
          Type: pointer               
          Use:  void * const           
          Acc:  read-write               
          Mech: By reference            
 
        chan                            Channel number 
          Type: integer                 Note: 0..16
          Use:  short
          Acc:  read-only
          Mech: By value

        val_p                           Data value read
          Type: integer                  
          Use:  short *                 
          Acc:  read-write                
          Mech: By reference                      
 
 
  Rem: This function reads adc data for the specified
       channel from io memory.

  Side: None
 
  Ret:  long
             OK    - Successful operation
             ERROR - Failure, due to invalid card/chan
 
=======================================================*/
long drvHy8413_rd( void  *  const  io_p,
                   unsigned short  chan, 
                   short *  const  val_p )
{
  long       status = OK;  
  HY8413_IO  io_ps = NULL;

 
  if (chan >= HY8413_NUM_CHAN)
  {
    status =  ERROR;
  }
  else
  {
    io_ps  = (HY8413_IO)io_p;
    *val_p = io_ps->data_s.adc_a[chan];
  }
  return(status);
}

/*====================================================
 
  Abs:  Write to the control register
 
  Name: drvHy8413_wt_csr
 
  Args: io_p                          Ptr to io memory 
          Type: address               
          Use:  void * const           
          Acc:  read-write               
          Mech: By reference   
         
        mask                          Bitmask 
          Type: bitmask                  
          Use:  unsigned short                 
          Acc:  read-only                
          Mech: By value     

        val                           Stat to write
          Type: bitmask                  
          Use:  unsigned short                 
          Acc:  read-only                
          Mech: By value                      
 
  Rem: This function writes the data value to the
       control register.

  Side: None
 
  Ret:  long
             OK    - Successful operation
             ERROR - Failure, due to invalid card/chan
 
=======================================================*/
long drvHy8413_wt_csr( volatile void  *  const  io_p, 
                       unsigned short           mask,
                       unsigned short           val_p )
{
  long status = OK;
  return( status );
}

/*====================================================
 
  Abs:  Write to the auxilary control register
 
  Name: drvHy8413_wt_acr
 
  Args: io_p                          Ptr to io memory 
          Type: address               
          Use:  void * const           
          Acc:  read-write               
          Mech: By reference            

        mask                          Bitmask 
          Type: bitmask                  
          Use:  unsigned short                 
          Acc:  read-only                
          Mech: By value     

        val                           Stat to write
          Type: bitmask                  
          Use:  unsigned short                 
          Acc:  read-only                                     
 
  Rem: This function writes the data value to the
       auxiliary control register.

  Side: None
 
  Ret:  long
             OK    - Successful operation
             ERROR - Failure, due to invalid card/chan
 
=======================================================*/
long drvHy8413_wt_acr( volatile void  *  const  io_p, 
                       unsigned short           mask,
                       unsigned short           val_p )
{
  long status = OK;
  return( status );
}


/*====================================================
 
  Abs:  Initialize the adc
 
  Name: drvHy8413_init
 
  Args: card_ps                         Card infomramtion
          Type: pointer               
          Use:  void * const           
          Acc:  read-write               
          Mech: By reference            
 
        mask                            Bit mask describing
          Type: bitmask                 the module setup
          Use:  unsigned long
          Acc:  read-only
          Mech: By value
 
  Rem: This function reads the data from the io memory for
       the specified channel.

  Side: None
 
  Ret:  long
             OK    - Successful operation
             ERROR - Failure, due to invalid card/chan
 
=======================================================*/
long drvHy8413_init( void * const card_p,unsigned long mask )
{
  long           status = OK;
  unsigned short val;
  IPADC_ID       card_ps = NULL;
  HY8413_IO      io_ps   = NULL;
 
  /* 
   * Set module in normal operating mode
   * and use the two's compliment data format.
   */
  card_ps    = (IPADC_ID)card_p;
  io_ps      = (HY8413_IO)card_ps->io_p;
  val        = HY8413_ACR_NS;
  io_ps->acr = val;
  
  val = HY8413_CSR_ARM | HY8413_CSR_IC;
  io_ps->csr = val;
  card_ps->nchan = HY8413_NUM_CHAN;
  card_ps->init = 1;
  return( status );
}


/*====================================================

  Abs:  Add the ipac module a card configuration

  Name: ip8413Create

  Args: name_c                          Card description
          Type: ascii-string            Note: must be NULL
          Use:  char *                  terminated.
          Acc:  read-only
          Mech: By reference

        carrier                         IP Carrier Card Number
          Type: integer                 Note: 0...196
          Use:  int
          Acc:  read-only
          Mech: By value

        slot                            IP Carrier Port Number
          Type: integer                 Note: 0=port A
          Use:  int                           1=port B
          Acc:  read-only                     2=port C
          Mech: By value                      4=port D

        mask                            Bitmask
          Type: integer                 
          Use:  unsigned long                          
          Acc:  read-only                   
          Mech: By value    

        vector                         Vector (0-255)
          Type: integer  
          Use:  unsigned char                          
          Acc:  read-only                   
          Mech: By value    


  Rem: This function determines if this card has already been
       added to the card list. If it has not, then memory is
       allocated to store the card information in a structure.
       This structure is then added to the card linked list.

       This is a wrapper to the generic function hytec_ipAddAdc() 
       providing a module specific interface call from the
       ioc shell.

  Side: This is function is called prior to iocInit() and
        after the ip carrier initialization function. 

  Ret:  long
             OK    - Successful operation
             ERROR - Failure, see return codes from
                     the function hytec_ipAddAdc()

=======================================================*/
long ip8413Create(char const * const name_c,  
              unsigned short         carrier,    
              unsigned short         slot,
              unsigned long          mask,
              unsigned char          vector )
{
  static const unsigned long model=HYTEC_IP8413_MODEL;
  return( hytec_ipmAdd(name_c,carrier,slot,model,mask,vector ));
}   




