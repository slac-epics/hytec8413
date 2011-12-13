/*
=============================================================

  Abs:  EPICS Device Support for the Hytec IP-ADC-8413 Module

  Name: devBoHy8413.c

         *   init_bo   - initialization binary output 
         *   write_bo  - set binary output

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
#if (EPICS_REVISION == 14 && EPICS_MODIFICATION >= 11)
#include  "errlog.h"
#endif
#include "cantProceed.h"
#include "boRecord.h"         /* struct boRecord           */
#include "alarm.h"            /* WRITE_ALARM,INVALID_ALARM */    
#include "recGbl.h"           /* recGblRecordError proto   */
#include "dbCommon.h"         /* for dbCommon              */
#include "dbScan.h"           /* IOSCANPVT                 */
#include "devSup.h"           /* DEVSUPFUN, S_dev_badBus   */
#include "drvIpac.h"          /* ipac_idProm_t             */
#include "hytecIpm.h"         /* for IPADC_ID,DPVT_ID      */
#include "hytecIpmLib.h"      /* for hytec_ipmInitDev()    */
#include "drvHy8413.h"        /* for HY8413_IO             */
#include "drvHy8413Lib.h"     /* for drvHy8413_rd() proto  */
#include "epicsExport.h"


/* Local prototypes */
static long init_bo( void *rec_p );
static long write_bo( void *rec_p ); 

/* Global variables */
extern int debugDevHy8413;

/* device support entry table */
typedef struct {
     long       number;       /* number of functions in list        */
     DEVSUPFUN  report;       /* print report - currently not used  */
     DEVSUPFUN  init;         /* called once during ioc init        */
     DEVSUPFUN  init_record;     /* init support for particular record */
     DEVSUPFUN  get_ioint_info;
     DEVSUPFUN  read_write;      /* ptr to write function           */
} DSET;
 
DSET devBoHy8413 = {5, NULL, NULL, init_bo, NULL, write_bo};
epicsExportAddress(dset,devBoHy8413);


/*=============================================================

  Abs:  Device Support initialization

  Name: init_bo

  Args: rec_p                          Record information
          Use:  struct
          Type: boRecord *
          Acc:  read-write access
          Mech: By reference

  Rem:  This device support routine is called by the record
        support function init_record(). Its purpose it to
        initializes a binary output.

  Side: INST_IO is the only bus type supported

  Ret: long
         OK               - Successful operation
         S_dev_badBus     - Failure, due to bad BUS type
         Otherwise see failed return from routines called:
            hy8413_getConfig()

=============================================================*/
static long init_bo(void *rec_p)
{
    long                          status     = OK;
    unsigned short                nelm       = 1;
    unsigned short                type       = TYPE_BO;
    struct instio                *instio_ps  = NULL;
    struct boRecord              *rec_ps     = NULL;

   /*
    * Is this bus type supported for this module?
    */
    rec_ps = (struct boRecord *)rec_p;
    switch (rec_ps->out.type) {
        case INST_IO:  /* Instrumentation */
          instio_ps    = (struct instio *)&(rec_ps->out.value);
	  rec_ps->dpvt = hytec_ipmInitDev( rec_ps->name,type,nelm,instio_ps->string );
          if ( !rec_ps->dpvt ) 
            status = S_dev_badOutType;
          break;

        default:       /* Bus type is not supported */
          status = S_dev_badBus;
          break;
    }/* End of switch statement */

    if ( status==OK )
       rec_ps->udf = 0;      /* Init completed successfully */
    else 
       recGblRecordError(status,rec_p,"devBoHy8413(init): Illegal OUT field");

    return(status);
}


/*=============================================================

  Abs:  Binary Output device support 

  Name: write_bo

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
         WRITE_ALARM        - Failure occured during write
         INVALID_ALARM      - Failure occured during write
         Otherwise, see return from
             alarmStatusChk()

=============================================================*/
static long write_bo(void *rec_p)
{
   long               status=OK;       /* status return            */
   unsigned short     i =0;            /* channel number           */
   unsigned short     mask  = 1;       /* bit mask                 */
   unsigned short     cur_stat   = WRITE_ALARM;  /* alarm status   */
   unsigned short     cur_sevr   = INVALID_ALARM;/* alarm severity */
   HY8413_IO          io_ps      = NULL;
   DPVT_ID            devPvt_ps  = NULL;
   IPADC_ID           card_ps    = NULL;
   struct boRecord   *rec_ps      = (struct boRecord *)rec_p;
   char              *taskName_c = "devBoHy8413( write )";


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
       case SetACR:
          mask <<= devPvt_ps->i;
	  if ( rec_ps->rval ) 

	   io_ps->acr &= ~mask;  /* clear bit */
          else
            io_ps->acr |= mask;  /* set bit  */
          break;

       case SetCSR:
         mask  <<= devPvt_ps->i;
	 if ( rec_ps->rval )
            io_ps->csr &= ~mask;  /* clear bit */
          else
            io_ps->csr |= mask;   /* set bit  */  
          break;

     /* 
      * First check to see if calibration data is available  
      * on this boad. If it is, then enable the use of the calibration data.
      * Otherwise, set a error upon return.
      * Note: the calibration use enable bit is looked at by the 
      * the devAiHy8413 driver. If the calibration data is to be
      * used for a particular channel, then the analog conversion
      * routine uses the gain data which was read during initialization
      */
      case SetCAL: /* use calibration data */
	 i = devPvt_ps->i;  /* channel number */
	 if ( rec_ps->rval )
	 { 
           if ( card_ps->cal_s.enb )
	     card_ps->cal_s.chan_as[i].enb = 0; /* don't use calibration data */
         }
         else
           card_ps->cal_s.chan_as[i].enb = 1;   /* use calibration data       */
	 break;

       default:
          status = ERROR;
          break;
   }/* End of switch statement */

   if (status!=OK)
      recGblSetSevr((dbCommon *)rec_p,cur_stat,cur_sevr);

   return(status);
}


