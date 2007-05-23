/*
=============================================================

  Abs:  EPICS Device Support for the Hytec IP-ADC-8413 Module

  Name: devMbboDirectHy8413.c
         *   init_mbboDirect  - initialization multi-bit binary direct output
         *   write_mbboDirect - set multi-bit binary direct output

  Proto: None

  Auth: 28-Jan-2007, K. Luchini       (LUCHINI)
  Rev : dd-mmm-yyyy, Reviewer's Name  (USERNAME)

-------------------------------------------------------------
  Mod:
        dd-mmm-yyyy, First Lastname   (USERNAME):
          comment

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
#include "mbboDirectRecord.h" /* struct mbboDirectRecord   */
#include "alarm.h"            /* WRITE_ALARM,INVALID_ALARM */    
#include "recGbl.h"           /* recGblRecordError proto   */
#include "dbCommon.h"         /* for dbCommon              */
#include "dbScan.h"           /* IOSCANPVT                 */
#include "devSup.h"           /* DEVSUPFUN, S_dev_badBus   */
#include "drvIpac.h"          /* ipac_idProm_t             */
#include "hytecIpm.h"         /* for IPADC_ID,DPVT_ID      */
#include "hytecIpmLib.h"      /* for hytec_ipmInitDev()    */
#include "drvHy8413Lib.h"     /* for drvHy8413_rd() proto  */
#include "epicsExport.h"


/* Local prototypes */
static long init_mbboDirect( void *rec_p );
static long write_mbboDirect( void *rec_p ); 

/* Global variables */
extern int debugDevHy8413;

/* device support entry table */
typedef struct {
     long       number;       /* number of functions in list        */
     DEVSUPFUN  report;       /* print report - currently not used  */
     DEVSUPFUN  init;         /* called once during ioc init        */
     DEVSUPFUN  init_record;  /* init support for particular record */
     DEVSUPFUN  get_ioint_info;
     DEVSUPFUN  read_write;      /* ptr to write function        */
} DSET;
 
DSET devMbboDirectHy8413 = {5, NULL, NULL, init_mbboDirect, NULL, write_mbboDirect};
epicsExportAddress(dset,devMbboDirectHy8413);


/*=============================================================

  Abs:  Device Support initialization

  Name: init_mbboDirect

  Args: rec_p                          Record information
          Use:  struct
          Type: boRecord *
          Acc:  read-write access
          Mech: By reference

  Rem:  This device support routine is called by the record
        support function init_record(). Its purpose it to
        initializes a multi-bit binary direct output.

  Side: INST_IO is the only bus type supported

  Ret: long
         OK               - Successful operation
         S_dev_badBus     - Failure, due to bad BUS type
         Otherwise see failed return from routines called:
            hy8413_getConfig()

=============================================================*/
static long init_mbboDirect(void *rec_p)
{
    long                          status     = OK;
    unsigned short                type       = TYPE_MBBO_DIRECT;
    unsigned short                nelm       = 1;
    struct instio                *instio_ps  = NULL;
    struct mbboDirectRecord      *rec_ps     = NULL;

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
       recGblRecordError(status,rec_p,"devMbboDirectHy8413(init): Illegal OUT field");

    return(status);
}


/*=============================================================

  Abs:  Multi-bit Binary Direct Output device support 

  Name: write_mbboDirect

  Args: rec_p                      Record information
          Use:  struct
          Type: void *
          Acc:  read-write access
          Mech: By reference

  Rem: This routine processes a binary output  record.

  Side: None

  Ret: long
         OK                 - Successful operation (convert rval->val)
         WRITE_ALARM        - Failure occured during write
         INVALID_ALARM      - Failure occured during write
         Otherwise, see return from
             alarmStatusChk()

=============================================================*/
static long write_mbboDirect(void *rec_p)
{
   long               status=OK;       /* status return            */
   short              i;               /* channel                  */
   unsigned short     cur_stat   = WRITE_ALARM;  /* alarm status   */
   unsigned short     cur_sevr   = INVALID_ALARM;/* alarm severity */
   DPVT_ID            devPvt_ps  = NULL;
   struct mbboDirectRecord   *rec_ps     = (struct boRecord *)rec_p;
   char                      *taskName_c = "devMbboDirectHy8413( write )";


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
      return(ERROR);
   }

   devPvt_ps = (DPVT_ID)rec_ps->dpvt;
   switch( devPvt_ps->func) 
   {
       default:
          if ( debugDevHy8413==0x12)
	    printf("%s: devSup has not been implimented to support ACR\n",taskName_c);
          status = ERROR;
          break;
   }/* End of switch statement */

   if (status!=OK)
      recGblSetSevr((dbCommon *)rec_p,cur_stat,cur_sevr);

   return(status);
}


