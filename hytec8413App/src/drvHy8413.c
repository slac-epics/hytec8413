/*
=============================================================
 
  Abs:  Driver Support for a VME Hytec ip-adc-8413 module
 
  Name: drvHy8413.c
          *  drvHy8413_init_driver   - Register init adc's with EPICS
          *  drvHy8413_io_report     - Report information of all cards.
          *  drvHy8413_dump          - Report information of a single card
             drvHy8413_dump_adc_data - Report adc data of a single card
             drvHy8413_dump_cal_data - Report calibration data for a single card 
             drvHy8413_rd            - Read specified channel data
	  *  drvHy8413_rd_cal_type   - read calibration type from id prom
          *  drvHy8413_rd_cal_data   - read calibration data from id prom
          *  drvHy8413_rd_cal_page   - read channel data from a specified id prom page
             drvHy8413_cal_adc       - calibrate raw adc data
             drvHy8413_wt_page       - set the id prom page in the auxillary control register
             drvHy8413_ARM           - start/stop sampling adc data at sample rate
             drvHy8413_wt_clk_rate   - set the clock rate register
             drvHy8413_rd_clk_rate   - read the clock rate register
             drvHy8413_init_sam_mode - Initilize the SAM Readout Mode in the ACR (v2 only)
             drvHy8413_init          - Module initialization called before iocInit()
             ip8413Create            - Module specific Wrapper for hyec_addIpAdc()
 
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
static void drvHy8413_dump_cal_data( hytec_ipmConfig_ts const * const card_ps );
static long drvHy8413_rd_cal_type( volatile unsigned short * const id_p, 
                                   hytec_ipmCal_ts         * const cal_ps );
static long drvHy8413_rd_cal_data( hytec_ipmConfig_ts * const card_ps );
static long drvHy8413_rd_cal_page( hytec_ipmConfig_ts * const card_ps,
                                   unsigned short             page );
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
     printf("IP8413: Software V%f\n\n",drvHy8413_ver);
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
  unsigned long      io_addr=(unsigned long)card_ps->io_p;
  unsigned long      id_addr=(unsigned long)card_ps->id_pu->_a;
  static const char *mode_ac[2]={"Standby","Normal"};
  static const char *format_ac[2]={"Two's Compliment","Offset Binary"};


  io_ps = (HY8413_IO)card_ps->io_p;
  printf("\tModel Id=%hx  Serial No=%hd  Rev: %hd  Card=%s is installed on carrier %hd slot %hd\n",
         card_ps->model,
         card_ps->serialNo,
         card_ps->rev,
         card_ps->name_c,
         card_ps->carrier,
         card_ps->slot );
 
  if (level>=1)
  {
     printf("\tIn total %hd channels\tOperating Mode: %s\tData Format: %s\n",
           card_ps->nchan,
           mode_ac[card_ps->mode],
           format_ac[card_ps->format] );
     printf("\tIO space is at 0x%lx  csr: 0x%hx  acr: 0x%hx  vec: 0x%hx\n",
           io_addr,
           io_ps->csr,
           io_ps->acr,
           io_ps->vec );
     printf("\tID space is at 0x%lx\n",id_addr);
     if (card_ps->init) 
       printf("\tInitialized Successfully\n");
     else
       printf("\tFailed Initialization\n");
  }

  if (level>=2)
  {
    /* display adc data */
    drvHy8413_dump_adc_data( (volatile unsigned short const * const)card_ps->io_p );
  }

  if (level>=3) 
  {
    /* display calibration mode and data */
    drvHy8413_dump_cal_data( card_ps );
  }
  return;
}


/*====================================================
 
  Abs:  Display data for a single Hytec ip-adc-8413 Module
 
  Name: drvHy8413_dump_adc_data
 
  Args: io_p                        Io base address
          Type: address        
          Use:  volatile unsigned short const  * const
          Acc:  read-only
          Mech: By reference

  Rem:  The purpose of this function is to display
        module adc data.
 
  Side: Sent to standard output device
  
  Ret:  None
            
=======================================================*/ 
void drvHy8413_dump_adc_data( volatile unsigned short const * const io_p )
{
  unsigned short i;
  unsigned short nchan = HY8413_NUM_CHAN/2;
  HY8413_IO      io_ps = (HY8413_IO)io_p;

  if ( !io_ps ) return;

  printf("\tADC Data:\n"); 
  for (i=0; i<nchan; i++) 
    printf("\tch%.2hd=0x%.4hx  ",i,io_ps->adc_a[i]);
  printf("\n");

  nchan = HY8413_NUM_CHAN;
  for (; i<nchan; i++) 

    printf("\tch%.2hd=0x%.4hx  ",i,io_ps->adc_a[i]);
  printf("\n");
  return;  
}
    
/*====================================================
 
  Abs:  Display calibration data for a single
        Hytec ip-adc-8413 Module
 
  Name: drvHy8413_dump_cal_data
 
  Args:  card_ps                      Card configuration info
          Type: struct           
          Use:  hytec_ipmConfig_ts *
          Acc:  read-only
          Mech: By reference

  Rem:  The purpose of this function is to display
        the module calibration data if it is available.
 
        Calibration availability is defined in the id prom/
          
          Type   Description 
          -----  ---------------------------
            0    No calibration
            1    Calibration 3 point factor
            2    Calibration 5 point factor
 
  Side: Report is sent to the standard output device
  
  Ret:  None
            
=======================================================*/
static void drvHy8413_dump_cal_data( hytec_ipmConfig_ts const * const card_ps )
{
  unsigned short       i_pts=0;      /* calibration point counter    */
  unsigned short       i=0;          /* channel index                */
  unsigned short       format;       /* data format                  */
  hytec_ipmCalChan_ts *cal_ps=NULL; /* channel calibraiton info     */
  static const char   *type_ac[3] = {"No Calibration","3-Point","5-Point"};
  static const char   *rge_ac[2]  = {"+/-10V","+/-5V"};
  static const char   *format_ac[2] = {"Two's Compliment","Offset Binary"};
  static const char   *use_ac[2] = {"Calibration data NOT In-Use","Calibration data In-Use"};
  static const char   *scale_ac[2][MAX_CAL_PTS]={{"-10V","-5V"  ,"0V","+5V"  ,"+10V"},
                                                { "-5V","-2.5V","0V","+2.5V","+5V"}};

  format = card_ps->format;
  printf("\n\tVoltage Range: %s\n",rge_ac[card_ps->range]);
  printf("\tCalibration Type: %s\n",type_ac[card_ps->cal_s.type] );
  printf("\tCalibration Data Format: %s\n",format_ac[format] );
  printf("\tCalibration Data:\n");
  if (card_ps->cal_s.type)
  {  
     printf("\t         %s\t%s\t%s\t%s\t%s\tCalibration Data\n",
             scale_ac[card_ps->range][0],
             scale_ac[card_ps->range][1],
             scale_ac[card_ps->range][2],
             scale_ac[card_ps->range][3],
             scale_ac[card_ps->range][4] );
      for (i=0; i<card_ps->nchan; i++)
      {   
       /* 
        * Read calibration data for this channel
	* from the card configuration data. The
	* calibration data has already been read
	* from the id prom during initialization and
	* stored locally.
        *
        *       Word  Description
        *       ----  -----------
        *        0     nFS -10V
        *        1     nHS -5V
        *        2     zero 0V
        *        3     pHS +5V
        *        4     pFS +10V
        */ 
        cal_ps =(hytec_ipmCalChan_ts *)&card_ps->cal_s.chan_as[i];
        printf("\tCh: %hd",i);
        for (i_pts=0; i_pts<MAX_CAL_PTS; i_pts++)
        {
          printf("\t0x%hx",cal_ps->gain_a[format][i_pts]);
        }/* End of i_pts FOR loop */
        printf("\t%s\n",use_ac[cal_ps->enb]);
      }/* End of i FOR loop */
      printf("\n");
  }
  return;
}

/*====================================================
 
  Abs:  Read specified adc channel data
 
  Name: drvHy8413_rd
 
  Args: io_p                            Base io address Card
          Type: pointer               
          Use:  volatile unsigned short * const           
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
long drvHy8413_rd( volatile unsigned short  * const  io_p,
                   unsigned short                    chan, 
                   short                    * const  val_p )
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
    *val_p = io_ps->adc_a[chan];
  }
  return(status);
}

/*====================================================
 
  Abs:  Calculate adc value using calibration data
 
  Name: drvHy8413_cal_adc
 
  Args: gain_a                          Gain values 
          Type: array                   Note: 5 element.
          Use:  unsigned short const * const           
          Acc:  read-only               
          Mech: By reference            
 
        calType                         Calibration type  
          Type: integer                 Note: none
          Use:  unsigned short                1 = 3-point
          Acc:  read-only                     2 = 5-point
          Mech: By value

        format                          Data format  
          Type: integer                 Note: 1=offste binary
          Use:  unsigned short                0=two's compliment
          Acc:  read-only
          Mech: By value

        rval                             Raw data value
          Type: integer                  
          Use:  long               
          Acc:  read-only                
          Mech: By value                      
 
 
  Rem: This function calibrates the raw adc value 
       supplied as an input argument. 

  Side: None
 
  Ret:  long 
             calibrated 32-bit value
 
=======================================================*/ 
long drvHy8413_cal_adc( unsigned short  const * const gain_a,
                        unsigned short                calType,
                        unsigned short                format,
                        long                          rval )
{
     hy8413_calData_ts *cal_ps = (hy8413_calData_ts *)gain_a;
     double             scale=0.0;
     double             dval=0.0;
     long               offset=0;
     long               resol=65535;
     long               rng=0;
 /*  long               zero_a[2]={TWOS_COMPLIMENT_ZERO,OFFSET_BINARY_ZERO}; */
     long               ref_rng_a[2]={0x3ff8,0x3ff9};
     long               offset_a[4]= {0xbff8,  /* -15V in counts remaining, from -10 to +5V  */
                                      0x7fff,  /* -10V in counts remaining, from -10 to  0V  */
                                      0x8000,  /* +10V in counts remaining, from   0 to +10V */
                                      0x4007}; /* +15V in counts remaining, from +10 to -5V  */
                                     
     switch(calType) 
     {
       /* 
	* Five point calibration with reference voltages at
	* -10V, -5V, 0V, +5V and +10V.
        * This gives us four linear interpolations. 
        */
       case factor_5pt:
         /* Is the raw value larger than the positive half scale? */
         if ( rval > cal_ps->posHS )
         {
	    rng    = ref_rng_a[0];
	    offset = offset_a[0]; /* absolute difference between -10 and +5 (full scale) */
	    scale  = (double)(cal_ps->posFS - cal_ps->posHS);
	    dval   = ((double)(rval - cal_ps->posHS) * (double)rng)/scale + (double)offset;
         }
         /* Is the raw value between zero and positive half scale? DOES NOT WORK! */
         else if ((rval >= cal_ps->zero) && (rval <= cal_ps->posHS) )
         {
	    rng    = ref_rng_a[0];
	    offset = offset_a[1]; /* absolute difference between -10 and 0 */
            scale  = (double)(cal_ps->posHS - cal_ps->zero);
            dval   = ((double)(rval - cal_ps->zero) * (double)rng)/scale + (double)offset;
         }
         /* Is the raw value between zero and nagative half scale? */
         else if ((cal_ps->zero > rval) && ( rval >= cal_ps->negHS) )
         {
	   rng    = ref_rng_a[1];
	   offset = offset_a[2]; /* absolute difference between 0 and +10 ((full scale) */
           scale  = (double)(cal_ps->zero - cal_ps->negHS);
	   dval   = ((double)(rval - cal_ps->zero) * (double)rng)/ scale + (double)offset;
         }
         /* Is the raw value less than nagative half scale? */
         else if ( rval < cal_ps->negHS )
         {
	   rng    = ref_rng_a[1];
	   offset = offset_a[3]; /* absolute difference between -5 and +10 (full scale)*/
           scale  = (double)(cal_ps->negHS - cal_ps->negFS);
           dval   = ((double)(rval - cal_ps->negHS) * (double)rng)/scale + (double)offset;
         }
         break;
 
        /*
	 * Three point calibration with reference voltages at 
	 * -10V, 0V and +10V. This givs us two linear interpolations. 
         */
        case factor_3pt: 
        /* Is the raw value less nagative? */
	rng = ref_rng_a[0];
        if ( rval < cal_ps->zero )
        {
	   offset = offset_a[2]; /* difference between 0 and +10 (full scale)*/
           scale  = (double)(cal_ps->zero - cal_ps->negFS);
           dval   = ((double)(rval - cal_ps->negFS) * (double)rng)/scale  + (double)offset;
        }
        /* Is the raw value greater than or equal to zero? */
        else
        {
	   offset = offset_a[1]; /* difference between 0 and -10 (full scale) */
	   scale  = (double)(cal_ps->posFS - cal_ps->zero);
           dval   = ((double)rval * (double)rng)/scale + (double)offset;
        }
 	break;

  }/* End of switch statement */
 
  /* if necssary, clip calibrated value */
  if(dval > (double)resol )
    dval = resol;
  else if( dval < 0)
   dval = 0.0;

  return( (long)dval ); 
}

/*====================================================
 
  Abs:  Read calibration data from id prom for all channels
 
  Name: drvHy8413_rd_cal_data


   Args: card_ps                         Card infomramtion
          Type: pointer               
          Use:  hytec_ipmConfig_ts const *           
          Acc:  read-write               
          Mech: By reference  
 
  Rem: This function reads calibration data from id prom
       memory fro all channels.

  Side: None
 
  Ret:  long
             OK    - Successful operation
             ERROR - Failure, due to invalid card/chan
 
=======================================================*/
static long drvHy8413_rd_cal_data( hytec_ipmConfig_ts * const card_ps )
{
   long                status = OK;   /* return status            */
   unsigned short      page;          /* page index counter       */
   unsigned short      max;
   static const struct
   {
     unsigned short imin;
     unsigned short imax;
   } page_as[2] = {{1,3,},{3,6}};
  
 
   switch( card_ps->cal_s.type )
   {
       default:
	  status = ERROR;
       case nocal: 
 	  break;

       case factor_3pt:
       case factor_5pt: 
	  max = page_as[card_ps->range].imax;
	  for (page = page_as[card_ps->range].imin && (status==OK);page <= max; page++ )
	    status = drvHy8413_rd_cal_page( card_ps, page );
         
          /*
	   *  No matter what be sure to reset the
	   *  id prom back to page zero before exiting
           */
          page = 0;
          drvHy8413_wt_page( card_ps->io_p, page );
	  break;
   }/* End of switch statement */

   if (status!=OK)
     errlogPrintf("IP8413: Failed to read calibration data from id prom - card: %hd  slot: %hd\n",
                  card_ps->carrier, 
                  card_ps->slot );
  
   return(status);
}


/*====================================================
 
  Abs:  Read calibration data from the specifid page
 
  Name: drvHy8413_rd_cal_page

  Args: card_ps                         Card infomramtion
          Type: pointer               
          Use:  hytec_ipmConfig_ts const *           
          Acc:  read-write               
          Mech: By reference  
 
  Rem: This function reads calibration data from the
       specified id prom page. This function sets the
       id prom page in the auxilliary controlr register 
       and then reads the channel data. Upon return from
       this function, the id prom page number is NOT reset.

  Side: Id prom page upon return has been altered.
 
  Ret:  long
             OK    - Successful operation
             ERROR - Failure, due to invalid card/chan
 
=======================================================*/
static long drvHy8413_rd_cal_page( hytec_ipmConfig_ts * const card_ps,
                                   unsigned short             page )
{
  long           status = OK;               /* status return      */
  unsigned short nchan=HY8413_MAX_PG_CHAN;  /* num chans per page */
  unsigned short npages=HY8413_NUM_PG;      /* num of pages per range  */                
  unsigned short offset=0;                  /* word offset to id prom  */   
  unsigned short i_pts=0;                   /* num of calib points     */
  unsigned short i_chan=0;                  /* channel number          */
  unsigned short i=0;                       /* index counters          */
  unsigned short gain;                      /* gain value read from id */
  hytec_ipmCalChan_ts     *cal_ps=NULL;     /* local calibration info  */
  volatile unsigned short *id_a=NULL;


  if ( (page<pg1) || (page>pg6) )
  { 
    errlogPrintf("IP8413: No calibration data on page %hd - card: %hd  slot: %hd\n",
                 page,
                 card_ps->carrier,
                 card_ps->slot);           
    status = ERROR;
    return(status);
  }

  /* Set page number */
  status = drvHy8413_wt_page( card_ps->io_p, page );
  if ( status==OK) 
  {
    /* 
     * Calculate the number of channels on this page.
     * Note: 
     * there are 16 total channels total and the
     * calibration data is divided into 3
     * 3 id prom pages, with 5 calibration data
     * values available per channel. As there are 2
     * different voltage ranges that can be selected
     * on the ip-adc-8413, we have 2 sets of calibration
     * and 7 total id prom pages, page 1-6 with calibration
     * data and page 0 with the general id prom data.
     * The calibration data for the channels is divided
     * amount three pages, such that the first 2 pages 
     * stores 6 channels worth of data and
     * the last page stores the remaing 4 channels. 
     */
      if ((page % npages)==0) 
       nchan=HY8413_MIN_PG_CHAN;
     
      /* calcuate first channel number on this page */
      i_chan = (page-1)*HY8413_MAX_PG_CHAN;
      
      /* 
       * Read calibration data for all channels on this page.
       * Remember, that we only have a subset of channels
       * per page.
       */
      if (debugHy8413)
        printf("Page: %hd\n",page);

      for (i=0; i<nchan; i++,i_chan++)
      {   
       /* 
        * Read calibration data for this channel
        *
        *       Word  Description
        *       ----  -----------
        *        0     nFS -10V
        *        1     nHS -5V
        *        2     zero 0V
        *        3     pHS +5V
        *        4     pFS +10V
        */ 
        id_a   = card_ps->id_pu->_a;
        card_ps->cal_s.chan_as[i_chan].init = 1;
        card_ps->cal_s.chan_as[i_chan].enb  = 1;
        cal_ps = &card_ps->cal_s.chan_as[i_chan];
        if (debugHy8413)
          printf("\tCh: %hd",i_chan+i_pts);
	for (i_pts=0, offset=0; i_pts<MAX_CAL_PTS; i_pts++,offset++)
        {
	   gain = id_a[offset];
	   cal_ps->gain_a[offset_binary][i_pts]   = gain;
           cal_ps->gain_a[twos_compliment][i_pts] = gain & TWOS_COMPLIMENT_MAX;
           if (debugHy8413)
              printf("\t0x%hx",gain);
	}
        if (debugHy8413)
          printf("\n");
      }
   } 
   return(status);
}

/*====================================================
 
  Abs:  Set the id prom page number 
 
  Name: drvHy8413_wt_page
 
  Args: io_p                          Ptr to io memory 
          Type: address               
          Use:  volatile void * const           
          Acc:  read-write               
          Mech: By reference   

        val                           Page number (0-6)
          Type: integer                  
          Use:  unsigned short                 
          Acc:  read-only                
          Mech: By value                      
 
  Rem: This function writes the selects the id prom page number
       by writting to the auxillary control register.

  Side: None
 
  Ret:  long
             OK    - Successful operation
             ERROR - Failed operation
 
=======================================================*/
long drvHy8413_wt_page( volatile void  * const io_p,unsigned short  val )
{
  long           status = OK;          /* status retur                */
  unsigned short rd_val;               /* read data from acr reg      */
  unsigned short rbk_val;              /* read back data from acr reg */
  unsigned short wt_val;               /* write data to acr reg       */ 
  unsigned short pg,rge,ns,mode;       /* bitmasks                    */
  unsigned short page=val;             /* page number to set          */
  HY8413_IO      io_ps=NULL;           /* ptr to io memory            */


  if ( page>pg6 )
  { 
    errlogPrintf("IP8413: Invalid id prom page requested (%hd)\n",page );
    status = ERROR;
  }
  else 
  {
     if (debugHy8413)
       printf("IP8413: Setting id prom page to %hd as requested\n",page );
     io_ps = (HY8413_IO)io_p;

     /* Read Auxillary Control Register */
     rd_val = io_ps->acr; 
     pg     = (page << HY8413_ACR_PG_SHFT);
     rge    = rd_val & HY8413_ACR_RGE;
     ns     = rd_val & HY8413_ACR_NS;
     mode   = rd_val & HY8413_ACR_2C;
     if (debugHy8413)
        printf("drvHy8413(wt_page):  acr= 0x%hx  pg=%hd rge=%hd ns%hd 2c=%hd\n",rd_val,pg,rge ,ns,mode);

     /* Set desired page */
     wt_val     = pg | rge | ns | mode;
     io_ps->acr = wt_val;

     /* Readback auxillary register and verify that page latched */
     rbk_val = io_ps->acr;
     if ((rbk_val & HY8413_ACR_PG) != pg)
     { 
        errlogPrintf("IP8413: Failed to set id prom page %hd - wt: %0xhx  rbk: 0x%hx\n",
                     page,
	             wt_val,
                     rbk_val );
        status = ERROR;
     }
  }
  return( status );
}

/*====================================================
 
  Abs:  Start/Stop sampling inputs at the sample clock rate 
 
  Name: drvHy8413_ARM
 
  Args: io_p                          Ptr to io memory 
          Type: address               
          Use:  volatile unsigned short * const           
          Acc:  read-write               
          Mech: By reference   

        val                            State to set
          Type: bitmask                Note: 0=start sampling
          Use:  unsigned short               1=stop sampling
          Acc:  read-only                
          Mech: By value    

  Rem: This function reads the clock rate register

  Side: None
 
  Ret:  long
            OK    - Successful
            ERROR - Failure, due to invalid carrier/slot
    
=======================================================*/
long drvHy8413_ARM( volatile unsigned short  *  const  io_p, 
                    unsigned short val )
{
   long       status = OK;
   HY8413_IO  io_ps  = NULL;


  io_ps = (HY8413_IO)io_p;
  if ( val ) 
     io_ps->acr |= HY8413_CSR_ARM; /* Start sampling */
  else
     io_ps->acr &= ~HY8413_CSR_ARM;   /* Stop sampling  */
  return(status); 
}

/*====================================================
 
  Abs:  Write clock rate 
 
  Name: drvHy8413_wt_clk_rate
 
  Args: carrier                        IP Carrier Card Number 
          Type: integer                Note: 0...196
          Use:  int
          Acc:  read-only
          Mech: By value

        slot                           IP Carrier Port Number  
          Type: integer                Note: 0=port A
          Use:  int                          1=port B
          Acc:  read-only                    2=port C
          Mech: By value                     4=port D   

         val                           Clock rate (0-15)
          Type: bitmask                  
          Use:  unsigned short                 
          Acc:  read-only                
          Mech: By value                      
 
  Rem: This function sets the clock rate.

  Side: None
 
  Ret:  long
             OK    - Successful operation
             ERROR - Failure
 
=======================================================*/
long drvHy8413_wt_clk_rate( volatile unsigned short  *  const  io_p,
                            unsigned short                     val )
{
  long           status  = OK;
  HY8413_IO      io_ps   = NULL;

  /*
   * Get the io address of this ipac module 
   * and then verify that the clock rate 
   * requested is valid
   */  
    io_ps = (HY8413_IO)io_p;
    if ( val > HY8413_MAX_CLK_RATE )
      status = ERROR;
    else { 
     io_ps->clk_rate = val & HY8413_CLK_RATE_MASK;

      /* Verify that the clock rate has been set */
      if ( io_ps->clk_rate != val ) 
         status = ERROR;
    }
    return( status );
}

/*====================================================
 
  Abs:  Read the clock rate 
 
  Name: drvHy8413_rd_clk_rate
 
  Args: carrier                        IP Carrier Card Number 
          Type: integer                Note: 0...196
          Use:  int
          Acc:  read-only
          Mech: By value

        slot                           IP Carrier Port Number  
          Type: integer                Note: 0=port A
          Use:  int                          1=port B
          Acc:  read-only                    2=port C
          Mech: By value                     4=port D   

  Rem: This function reads the clock rate register

  Side: None
 
  Ret:  short
            OK  = clock rate (0-15)
            -1  = Failure, due to invalid carrier and/or slot
    
=======================================================*/
short  drvHy8413_rd_clk_rate( volatile unsigned short  *  const  io_p)
{
  HY8413_IO  io_ps   = NULL;
 
  io_ps = (HY8413_IO)io_p;
  return( io_ps->clk_rate );
}

/*====================================================
 
  Abs:  Initialize SAM Readout Mode
 
  Name: drvHy8413_init_sam_mode
 
  Args: carrier                        IP Carrier Card Number 
          Type: integer                Note: 0...196
          Use:  int
          Acc:  read-only
          Mech: By value

        slot                           IP Carrier Port Number  
          Type: integer                Note: 0=port A
          Use:  int                          1=port B
          Acc:  read-only                    2=port C
          Mech: By value                     4=port D   
         
  Rem: This function initilizes the module in 
       SAM Readout Mode. This mode is only available
       in the SLAC modified version (v2).

  Side: None
 
  Ret:  long
             OK    - Successful operation
             ERROR - Failure, due to invalid card/chan
 
=======================================================*/
long drvHy8413_init_sam_mode( volatile unsigned short  *  const  io_p )
{
  long           status  = OK;
  unsigned short val     = 0;
  HY8413_IO      io_ps   = NULL;
 
  

  /* Check if the module is armed. If so, the disarm from sampling data */
  io_ps = (HY8413_IO)io_p;

  /* Make sure the modules is not ARMed */
  io_ps->csr &= ~HY8413_CSR_ARM;   /* Clear the ARM bit in the csr  */

  /* Pulse the Initialize the Averager bit in the Auxiliary Control Register (ACR) */
  io_ps->acr |= HY8413_ACR_AINI;   /* Set the AINI bit in the acr   */    
  io_ps->acr &= ~HY8413_ACR_AINI;  /* Clear the AINI bit in the acr */

  /* Enable the Averager */
  io_ps->acr |= HY8413_ACR_AEN;
  
  /* Enable SAM Mode */
  io_ps->acr |= HY8413_ACR_SAM;

  /*
   * Set board to begin sampling:
   *
   * First, set the Auxiliary Control Register (ACR) to normal operating modd.
   */ 
  io_ps->acr |= HY8413_ACR_NS;


  /* Next set the Control Status Register (CSR) to ARM the board */
  io_ps->csr |= HY8413_CSR_ARM;

  /* Set the ADN/BUF bit and then check for a state change */
  io_ps->csr |= HY8413_ACR_ADN;
  val = io_ps->csr & HY8413_ACR_ADN;

  /* 
   * Finally, set Auxiliary Control Register (ACR)
   * ADC Readout Select to Averaged Data readout mode. 
   */
  io_ps->acr |= HY8413_ACR_ARS;
   
  return( status );
}


/*====================================================
 
  Abs:  Write to the control register
 
  Name: drvHy8413_wt_csr
 
  Args: io_p                          Ptr to io memory 
          Type: address               
          Use:  volatile unsigned short * const           
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
long drvHy8413_wt_csr( volatile unsigned short  *  const  io_p, 
                       unsigned short                     mask,
                       unsigned short                     val )
{
  long status = OK;
  return( status );
}

/*====================================================
 
  Abs:  Write to the auxilary control register
 
  Name: drvHy8413_wt_acr
 
  Args: io_p                          Ptr to io memory 
          Type: address               
          Use:  volatile unsigned short * const           
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
long drvHy8413_wt_acr( volatile unsigned short  *  const  io_p, 
                       unsigned short                     mask,
                       unsigned short                     val )
{
  long           status = OK;
  unsigned long  ival   = 0;
  HY8413_IO      io_ps  = NULL;
 
  io_ps = (HY8413_IO)io_p;
  ival  = io_ps->acr & HY8413_ACR_MASK;
  io_ps->acr = (ival & mask) | val;
  return( status );
}


/*====================================================
 
  Abs:  Read calibration type from the id prom
 
  Name: drvHy8413_rd_cal_type
 
  Args: id_p                           Base address of id prom
          Type: pointer               
          Use:  volatile unsigned short * const         
          Acc:  read-only              
          Mech: By reference            
 
        cal_ps                         Calibration information 
          Type: pointer                 
          Use:  hytec_ipmCal_ts * const
          Acc:  read-write access
          Mech: By reference
 
  Rem: This function reads the calibration type from the
       id prom of the ipac module.

  Side: None
 
  Ret:  long
             OK    - Successful operation
             ERROR - Failure, due to invalid card/chan
 
=======================================================*/
static long drvHy8413_rd_cal_type( volatile unsigned short * const id_p, 
                                   hytec_ipmCal_ts         * const cal_ps )
{
  long                status  = OK;
  unsigned short      type    = 0;
  static const short  npts_a[NUM_CAL_TYPES] = {0,MIN_CAL_PTS,MAX_CAL_PTS};
  HY8413_ID           id_ps = (HY8413_ID)id_p;


  type  = id_ps->calType;
  switch( type )
  {
     case nocal:
       cal_ps->type = type;
       cal_ps->enb  = 0;
       cal_ps->npts = npts_a[type];
       break;

     case factor_3pt:
     case factor_5pt:
       cal_ps->type = type;
       cal_ps->enb  = 1;
       cal_ps->npts = npts_a[type];
       break;

     default: 
       cal_ps->type = nocal;
       cal_ps->enb  = 0;
       cal_ps->npts = 0;
       status = ERROR;
       errlogPrintf("IP8413: Invalid calibration type found (%hd)\n",type);
       break;
  }/* End of switch statement */

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
  long             status  = OK;
  unsigned short   val     = 0;
  IPADC_ID         card_ps = NULL;
  HY8413_IO        io_ps   = NULL;

  
  /* 
   * Always set module in normal operating mode
   * and use the two's compliment data format.
   */
  card_ps    = (IPADC_ID)card_p;
  io_ps      = (HY8413_IO)card_ps->io_p;

  /* Set the number of channels for an ip-adc-8413 module */
  card_ps->nchan = HY8413_NUM_CHAN;
  
  /* Set the Auxiliary Control Register (ACR) to normal operating mode and offset binary */
  val = HY8413_ACR_NS | HY8413_ACR_2C;  
  io_ps->acr = val;

  /* Set the Control Register (CSR) to sampling */
  val = HY8413_CSR_ARM;
  io_ps->csr = val;

  /* read operating mode */
  val = io_ps->acr;
  card_ps->mode  = (val & HY8413_ACR_NS) >> HY8413_ACR_NS_SHFT;

  /* read adc data format */
  card_ps->format = (val & HY8413_ACR_2C) >> HY8413_ACR_2C_SHFT;

  /* read adc voltage range */
  card_ps->range = (val & HY8413_ACR_RGE) >> HY8413_ACR_RGE_SHFT;

  /* Read the interrupt vector address */
  card_ps->intVec = io_ps->vec; 

  /* 
   * Read calibration type and then read calibration data from id prom 
   * If the calibration data is available then go ahead and read it.
   */
  status = drvHy8413_rd_cal_type( card_ps->id_pu->_a, &card_ps->cal_s );
  if ( status == OK )
    status = drvHy8413_rd_cal_data( card_ps );
  
  /* Set anything special for the init */
  if ( (status == OK) && mask ) {
     val = (mask >>16) & HY8413_CLK_RATE_MASK;
     status = drvHy8413_wt_clk_rate( card_ps->io_p,val );
     if ((status==OK) && (mask && 1))
       status = drvHy8413_init_sam_mode( card_ps->io_p );
  }      
    
  /* flag init complete */
  if ( status==OK ) {
      card_ps->init = 1;
      printf("drvHy8413: Successfully initalized card %s\n",card_ps->name_c);
  }
  else 
     printf("drvHy8413: Failed to initalize card %s\n",card_ps->name_c);

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
                  unsigned short     carrier,    
                  unsigned short     slot,
                  unsigned long      mask,
                  unsigned char      vector )
{
  long   status = OK;
  static const unsigned long model=HYTEC_IP8413_MODEL;

  status = hytec_ipmAdd(name_c,carrier,slot,model,mask,vector);
  return( status );
}   




/*====================================================

  Abs:  Add the ipac module a card configuration

  Name: ip8413test

  Args: carrier                         IP Carrier Card Number 
          Type: integer                Note: 0...196
          Use:  int
          Acc:  read-only
          Mech: By value

        slot                            IP Carrier Port Number  
          Type: integer                 Note: 0=port A
          Use:  int                           1=port B
          Acc:  read-only                     2=port C
          Mech: By value                      4=port D   
 
  Rem: This function is a test for the firmwawr if this card has already been
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




