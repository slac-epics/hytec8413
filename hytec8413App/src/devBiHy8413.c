/*
=============================================================

  Abs:  EPICS Device Support for the Hytec IP-ADC-8413 Module

  Name: devBiHy8413.c

         *   init_bi           - initialization binary input 
         *   read_bi           - read binary input state
         *   get_ioint_info_bi - get io interrup info

   Proto: None

  Auth: 28-Jan-2007, K. Luchini       (LUCHINI)
  Rev : dd-mmm-yyyy, Reviewer's Name  (USERNAME)

-------------------------------------------------------------
  Mod:
        08-Nov-2006, K. Luchini       (LUCHINI):
          cast arg 1 in hy8413_rd() to void ptr

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
#include "cantProceed.h"
#include "biRecord.h"         /* struct biRecord           */
#include "alarm.h"            /* READ_ALARM,INVALID_ALARM  */    
#include "recGbl.h"           /* recGblRecordError proto   */
#include "dbCommon.h"         /* for dbCommon              */
#include "dbScan.h"           /* IOSCANPVT                 */
#include "devSup.h"           /* DEVSUPFUN, S_dev_badBus   */
#include "drvIpac.h"          /* ipac_idProm_t             */
#include "hytecIpm.h"         /* for IPADC_ID,DPVT_ID      */
#include "hytecIpmLib.h"      /* for hytec_ipmInitDev()    */
#include "drvHy8413.h"        /* for HYT8413_IO            */
#include "drvHy8413Lib.h"     /* for drvHy8413_rd() proto  */
#include "epicsExport.h"


/* Local prototypes */
static long init_bi( void *rec_p );
static long read_bi( void *rec_p ); 
static long get_ioint_info_bi( int cmd, void *rec_p, IOSCANPVT *evt_pp );

/* 
 * Global variables 
 */
extern int debugDevHy8413;

/* Device Entry Table */
typedef struct {
     long       number;       /* number of functions in list        */
     DEVSUPFUN  report;       /* print report - currently not used  */
     DEVSUPFUN  init;         /* called once during ioc init        */
     DEVSUPFUN  init_record;  /* init support for particular record */
     DEVSUPFUN  get_ioint_info;
     DEVSUPFUN  read_write;      /* ptr to write function        */
} DSET;
 
DSET devBiHy8413    = {5, NULL, NULL, init_bi, get_ioint_info_bi, read_bi};
epicsExportAddress(dset,devBiHy8413);


/*=============================================================

  Abs:  Device Support initialization

  Name: init_bi

  Args: rec_p                          Record information
          Use:  struct
          Type: biRecord *
          Acc:  read-write access
          Mech: By reference

  Rem:  This device support routine is called by the record
        support function init_record(). Its purpose it to
        initializes binary input records.

  Side: INST_IO is the only bus type supported

  Ret: long
         OK               - Successful operation
         S_dev_badBus     - Failure, due to bad BUS type
         Otherwise see failed return from routines called:
            hy8413_getConfig()

=============================================================*/
static long init_bi(void *rec_p)
{
    long                          status     = OK;
    unsigned short                type       = TYPE_BI;
    unsigned short                nelm       = 1;
    struct instio                *instio_ps  = NULL;
    struct biRecord              *rec_ps     = NULL;

   /*
    * Is this bus type supported for this module?
    */
    rec_ps = (struct biRecord *)rec_p;
    switch (rec_ps->inp.type) 
    {
        case INST_IO:  /* Instrumentation */
          instio_ps = (struct instio *)&(rec_ps->inp.value);
          rec_ps->dpvt = hytec_ipmInitDev( rec_ps->name,type,nelm,instio_ps->string );
          if ( !rec_ps->dpvt ) 
            status = S_dev_badInpType;
          break;

        default:       /* Bus type is not supported */
          status = S_dev_badBus;
          break;
    }/* End of switch statement */

    if ( status==OK )
       rec_ps->udf = 0;      /* Init completed successfully */
    else
       recGblRecordError(status,rec_p,"devBiHy8413(init): Illegal INP field");

    return(status);
}

/*=============================================================

  Abs:  Device Support for io scanner init

  Name: get_ioint_info_bi

  Args: cmd                        Command being performed
          Use:  integer
          Type: int
          Acc:  read-only
          Mech: By value

        rec_p                      Record information
          Use:  struct
          Type: void *
          Acc:  read-write access
          Mech: By reference

        evt_pp                     I/O scan event
          Use:  struct
          Type: IOSCANPVT *
          Acc:  read-write access
          Mech: By reference

  Rem:  This device support provides access to the IOSCANPVT
        structure associated with the specified adc card
        defined for this pv..

  Side: This routine can be called at interrupt level
        to process an event.

  Ret: long
            OK - Successful operation (always returned)

=============================================================*/
static long get_ioint_info_bi( int cmd, void *rec_p, IOSCANPVT *evt_pp )
{
    long               status=OK;          /* status return         */
    unsigned short     bitNo;              /* bit number            */
    unsigned short     reg;                /* register              */
    DPVT_ID            devPvt_ps = NULL;   /* private device info   */
    struct biRecord   *rec_ps    = NULL;   /* ptr to record         */

    rec_ps = (struct biRecord *)rec_p;
    if (rec_ps->dpvt)
    {
       devPvt_ps = (DPVT_ID)rec_ps->dpvt;
       bitNo     = devPvt_ps->i;
       switch ( devPvt_ps->func )
       {
         case ReadCAL:
            *evt_pp = devPvt_ps->card_ps->calEnbScan;
	    break;

         case ReadCSR:
         case ReadACR:
           bitNo = devPvt_ps->i;
	   reg = devPvt_ps->func;
           *evt_pp   = devPvt_ps->card_ps->biScan_a[reg][bitNo];
	   break;

         default:
           errlogPrintf("devBiHy8413(init): devSup has not been implimented for %s\n",rec_ps->name);
	   break;
       }/* End of switch statement */
    }
    return( status );
}


/*=============================================================

  Abs:  Binary Input device support 

  Name: read_bi

  Args: rec_p                      Record information
          Use:  struct
          Type: void *
          Acc:  read-write access
          Mech: By reference

  Rem: This routine processes a binary output  record.
       The following hardware registers 
         CSR - Control Register
         ACR - Auxilliary Control Register 

  Side: None

  Ret: long
         OK                 - Successful operation (convert rval->val)
         READ_ALARM         - Failure occured during read
         INVALID_ALARM      - Failure occured during read
         Otherwise, see return from
             alarmStatusChk()

=============================================================*/
static long read_bi(void *rec_p)
{
   long               status=OK;       /* status return            */
   unsigned short     mask  = 1;       /* bit mask                 */
   unsigned short     val   = 0;       /* register data            */
   unsigned short     i = 0;           /* channel index            */
   unsigned short     cur_stat   = READ_ALARM;  /* alarm status    */
   unsigned short     cur_sevr   = INVALID_ALARM;/* alarm severity */
   HY8413_IO          io_ps      = NULL;
   DPVT_ID            devPvt_ps  = NULL;
   IPADC_ID           card_ps    = NULL;
   struct biRecord   *rec_ps      = (struct biRecord *)rec_p;
   char              *taskName_c = "devBiHy8413( read )";


   /*
    * If the private device info has not been
    * initialized then we have a problem, so 
    * exit with an error status.
    */
   if ( !rec_ps->dpvt ) 
   {
       status = recGblSetSevr(rec_ps,cur_stat,cur_sevr);
       if ( status  &&  errVerbose && 
           ((rec_ps->stat!=cur_stat) || 
            (rec_ps->sevr!=cur_sevr)) )
         recGblRecordError(ERROR,(void *)rec_ps,taskName_c ); 
      status = ERROR;
      return(status);
   }

   devPvt_ps = (DPVT_ID)rec_ps->dpvt;
   card_ps   = devPvt_ps->card_ps;
   io_ps     = (HY8413_IO)card_ps->io_p;
   switch( devPvt_ps->func ) 
   {
       case ReadACR:
          mask <<= devPvt_ps->i-1;
          val  = io_ps->acr & mask;
          rec_ps->rval = (val) ? 1 : 0;
          if (debugDevHy8413==1)
            printf("%s: mask=0x%hd\tval=0x%hd for %s\n",taskName_c,mask,val,rec_ps->name);
          break;

       case ReadCSR:
  	  mask <<= devPvt_ps->i-1;
          val  = io_ps->csr & mask;
          rec_ps->rval = (val) ? 1 : 0;
          if (debugDevHy8413==1)
            printf("%s: mask=0x%hd\tval=0x%hd for %s\n",taskName_c,mask,val,rec_ps->name);
          break;

      case ReadCAL:  /* Is calibration in use for this channel? */
         i    = devPvt_ps->i;      /* channel number */
	 val  = card_ps->cal_s.chan_as[i].enb;
         rec_ps->rval = (val) ? 1 : 0;
         if (debugDevHy8413==1)
           printf("%s: mask=0x%hd\tval=0x%hd for %s\n",taskName_c,mask,val,rec_ps->name);
         break;

       default:
          if (debugDevHy8413==1)
            printf("%s:  devSup has not been implimented for %s\n",taskName_c,rec_ps->name);
          status = ERROR;
          break;
   }/* End of switch statement */

   if (status!=OK)
      recGblSetSevr((dbCommon *)rec_p,cur_stat,cur_sevr);

   return(status);
}


