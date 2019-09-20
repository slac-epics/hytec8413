/*
=============================================================

  Abs:  EPICS Device Support for the Hytec IP-ADC-8413 Module

  Name: devLiHy8413.c

         *   init_li  - initialization binary input 
         *   read_li  - read binary input state

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
#if (EPICS_REVISION == 14 && EPICS_MODIFICATION >= 11 || (EPICS_REVISION == 15)) || (EPICS_VERSION == 7)
#include  "errlog.h"
#endif
#include "cantProceed.h"
#include "longinRecord.h"     /* struct longinRecord       */
#include "alarm.h"            /* READ_ALARM,INVALID_ALARM  */    
#include "recGbl.h"           /* recGblRecordError proto   */
#include "dbCommon.h"         /* for dbCommon              */
#include "dbScan.h"           /* IOSCANPVT                 */
#include "devSup.h"           /* DEVSUPFUN, S_dev_badBus   */
#include "drvIpac.h"          /* ipac_idProm_t             */
#include "hytecIpm.h"         /* for IPADC_ID,DPVT_ID_LI   */
#include "hytecIpmLib.h"      /* for hytec_ipmInitDev()    */
#include "drvHy8413.h"        /* for HYT8413_IO            */
#include "drvHy8413Lib.h"     /* for drvHy8413_rd() proto  */
#include "epicsExport.h"


/* Local prototypes */
static long init_li( void *rec_p );
static long read_li( void *rec_p ); 

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
 
DSET devLiHy8413    = {5, NULL, NULL, init_li, NULL, read_li};
epicsExportAddress(dset,devLiHy8413);


/*=============================================================

  Abs:  Device Support initialization

  Name: init_li

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
static long init_li(void *rec_p)
{
    long                          status     = OK;
    unsigned short                type       = TYPE_LI;
    unsigned short                nelm       = 1;
    struct instio                *instio_ps  = NULL;
    struct longinRecord          *rec_ps     = NULL;

   /*
    * Is this bus type supported for this module?
    */
    rec_ps = (struct longinRecord *)rec_p;
    switch (rec_ps->inp.type) 
    {
        case INST_IO:  /* Instrumentation */
          instio_ps = (struct instio *)&(rec_ps->inp.value);
          rec_ps->dpvt = hytec_ipmInitDev( rec_ps->name,type,nelm,instio_ps->string );
          if ( !rec_ps->dpvt  ) 
            status = S_dev_badInpType;
          break;

        default:       /* Bus type is not supported */
          status = S_dev_badBus;
          break;
    }/* End of switch statement */

    if ( status==OK )
       rec_ps->udf = 0;      /* Init completed successfully */
    else 
       recGblRecordError(status,rec_p,"devLiHy8413(init): Illegal INP field"); 

    return(status);
}


/*=============================================================

  Abs:  Long Input device support 

  Name: read_li

  Args: rec_p                      Record information
          Use:  struct
          Type: void *
          Acc:  read-write access
          Mech: By reference

  Rem: This routine processes a binary output  record.
       The following hardware registers 
         ID - Id Prom Registers 

  Side: None

  Ret: long
         OK                 - Successful operation (convert rval->val)
         READ_ALARM         - Failure occured during read
         INVALID_ALARM      - Failure occured during read
         Otherwise, see return from
             alarmStatusChk()

=============================================================*/
static long read_li(void *rec_p)
{
   long                      status=OK;       /* status return            */
   unsigned short            val;             /* raw value                */
   unsigned short            i = 0;           /* channel index            */
   unsigned short            cur_stat   = READ_ALARM;  /* alarm status    */
   unsigned short            cur_sevr   = INVALID_ALARM;/* alarm severity */
   DPVT_ID                   devPvt_ps  = NULL;
   IPADC_ID                  card_ps    = NULL;
   volatile unsigned short  *io_a       = NULL;
   struct longinRecord      *rec_ps     = (struct longinRecord *)rec_p;
   char                     *taskName_c = "devLiHy8413( read )";


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
   io_a      = (volatile unsigned short *)card_ps->io_p;
   i = devPvt_ps->i;
   switch( devPvt_ps->func ) 
   {
       case ReadID:
         val = card_ps->id_pu->_a[i];
         rec_ps->val = (long)val;
         if ( debugDevHy8413==0x8)
           printf("%s: idprom offset=0x%hx val=0x%hx (dec=%hd) for %s\n",
                 taskName_c,i,val,val,rec_ps->name);
         break;

       case ReadIO:
         val = io_a[i];
	 rec_ps->val = (long)val;
         if ( debugDevHy8413==0x8)
           printf("%s: io offset=0x%hx val=0x%hx (dec=%hd) for %s\n",
		taskName_c,i,val,val,rec_ps->name);
         break;

       default:
          if ( debugDevHy8413==0x8)
          printf("%s:  devSup (func=%hd) has not been implimented for %s\n",
                 taskName_c,devPvt_ps->func,rec_ps->name);
          status = ERROR;
          break;
   }/* End of switch statement */

   if (status!=OK)
      recGblSetSevr((dbCommon *)rec_p,cur_stat,cur_sevr);

   return(status);
}


