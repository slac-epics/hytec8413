/*
=============================================================

  Abs:  EPICS Device Support for the Hytec IP-ADC-8413 Module

  Name: devMbbiDirectHy8413.c

         *   init_mbbiDirect            - initialization multi-bit binary input
         *   get_ioint_info_mbbiDirect  - io interrupt muiti-bit binary input 
         *   read_mbbi                  - read multi-bit binary input

   Proto: 

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
#include <math.h>

#include "epicsVersion.h"
#include "epicsMutex.h"
#include "epicsString.h"
#include "cantProceed.h"
#if (EPICS_REVISION == 14 && EPICS_MODIFICATION >= 11) || (EPICS_REVISION == 15)
#include  "errlog.h"
#endif
#include "mbbiDirectRecord.h" /* struct mbbiDirectRecord   */
#include "alarm.h"            /* READ_ALARM,INVALID_ALARM  */    
#include "recGbl.h"           /* recGblRecordError proto   */
#include "dbCommon.h"         /* for dbCommon              */
#include "dbScan.h"           /* IOSCANPVT                 */
#include "devSup.h"           /* DEVSUPFUN, S_dev_badBus   */
#include "drvIpac.h"          /* ipac_idProm_t             */
#include "hytecIpm.h"         /* for IPADC_ID,DPVT_ID_BI   */
#include "hytecIpmLib.h"      /* for hytec_ipmInitDev()    */
#include "drvHy8413.h"        /* for HY8413_IO             */
#include "drvHy8413Lib.h"     /* for drvHy8413_rd() proto  */
#include "epicsExport.h"


/* Local prototypes */
static long init_mbbiDirect( void *rec_p );
static long read_mbbiDirect( void *rec_p );
static long get_ioint_info_mbbiDirect( int  cmd, void  *rec_p, IOSCANPVT *evt_pp );

/* Global variables */
extern int debugDevHy8413;

/* device support entry table */
typedef struct {
     long       number;       /* number of functions in list        */
     DEVSUPFUN  report;       /* print report - currently not used  */
     DEVSUPFUN  init;         /* called once during ioc init        */
     DEVSUPFUN  init_record;     /* init support for particular record */
     DEVSUPFUN  get_ioint_info;  /* ioc event scanning function        */
     DEVSUPFUN  read_write;      /* ptr to write function              */
} DSET;
DSET devMbbiDirectHy8413  = {5, 
                             NULL, 
                             NULL, 
                             init_mbbiDirect, 
                             get_ioint_info_mbbiDirect, 
                             read_mbbiDirect };
epicsExportAddress(dset,devMbbiDirectHy8413);


/*=============================================================

  Abs:  Device Support initialization

  Name: init_mbbiDirect

  Args: rec_p                          Record information
          Use:  struct
          Type: biRecord *
          Acc:  read-write access
          Mech: By reference

  Rem:  This device support routine is called by the record
        support function init_record(). Its purpose it to
        initializes multi-bit binary direct input records.

  Side: INST_IO is the only bus type supported

  Ret: long
         OK               - Successful operation
         S_dev_badBus     - Failure, due to bad BUS type
         Otherwise see failed return from routines called:
            hy8413_getConfig()

=============================================================*/
static long init_mbbiDirect(void *rec_p)
{
    long                          status     = OK;
    unsigned short                nelm       = 1;
    unsigned short                type       = TYPE_MBBI_DIRECT;
    struct instio                *instio_ps  = NULL;
    struct mbbiDirectRecord      *rec_ps     = NULL;

   /*
    * Is this bus type supported for this module?
    */
    rec_ps = (struct mbbiDirectRecord *)rec_p;
    switch (rec_ps->inp.type)
    {
        case INST_IO:  /* Instrumentation */
          instio_ps     = (struct instio *)&(rec_ps->inp.value);
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
       recGblRecordError(status,rec_p,"devMbbiDirectHy8413(init): Illegal INP field");

    return(status);
}

/*=============================================================

  Abs:  Device Support for io scanner init

  Name: get_ioint_info_mbbiDIrect

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

        This routine is required for any device
        type that can use the ioEvent scanner.
        It is called by the ioEventScan system each
        time the record is added or deleted from
        an I/O event scan list.  The argument "cmd"
        of this function, has the value (0,1) if the
        record is being added to or deleted from,
        an I/O scan list.  It must be provided for
       any device type that can use the ioEvent scanner.

  Side: This routine can be called at interrupt level
        to process an event.

  Ret: long
            OK - Successful operation (always returned)

=============================================================*/
static long get_ioint_info_mbbiDirect( int        cmd,
                                       void      *rec_p, 
                                       IOSCANPVT *evt_pp )
{
    long                      status=OK;          /* status return        */
    unsigned short            bitNo;              /* bit number           */
    DPVT_ID                   devPvt_ps = NULL;   /* private device info  */
    struct mbbiDirectRecord  *rec_ps;             /* Analog input record  */


    rec_ps  = (struct mbbiDirectRecord *)rec_p;
    if (rec_ps->dpvt)
    {
       devPvt_ps = rec_ps->dpvt;
       bitNo     = devPvt_ps->i;
       *evt_pp = devPvt_ps->card_ps->mbbiScan_a[bitNo];
    }
    return( status );
}


/*=============================================================

  Abs:  Mulit-bit Binary Direct Input device support 

  Name: read_mbbiDirect

  Args: rec_p                      Record information
          Use:  struct
          Type: void *
          Acc:  read-write access
          Mech: By reference

 Rem:  This routine processes a multi-bit binary direct input record.
       The following hardware registers 

  Side: None

  Ret: long
         OK                 - Successful operation (convert rval->val)
         READ_ALARM         - Failure occured during read
         INVALID_ALARM      - Failure occured during read
         Otherwise, see return from
             alarmStatusChk()

=============================================================*/
static long read_mbbiDirect(void *rec_p)
{
   long                      status=OK;      /* status return            */
   unsigned short            cur_stat   = READ_ALARM;  /* alarm status    */
   unsigned short            cur_sevr   = INVALID_ALARM;/* alarm severity */
   DPVT_ID                   devPvt_ps  = NULL;
   IPADC_ID                  card_ps    = NULL;
   HY8413_IO                 io_ps      = NULL;
   struct mbbiDirectRecord  *rec_ps     = (struct mbbiDirectRecord *)rec_p;
   char                     *taskName_c = "devMbbiDirectHy8413( read )";


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
       {
	   status = ERROR;
           recGblRecordError(status,(void *)rec_ps,taskName_c ); 
           return(status);
       }
   }
  
   devPvt_ps = (DPVT_ID)rec_ps->dpvt;
   card_ps   = devPvt_ps->card_ps;
   io_ps     = (HY8413_IO)card_ps->io_p;
   switch( devPvt_ps->func ) 
   {
        case ReadACR:
          rec_ps->rval = io_ps->acr & HY8413_ACR_MASK;
          break;

        case ReadCSR:
          rec_ps->rval = io_ps->acr & HY8413_CSR_MASK;
          break;

      default:
          if ( debugDevHy8413==0x4)
            printf("%s:  devSup has not been implimented for %s\n",taskName_c,rec_ps->name);
          status = ERROR;
          break;
   }/* End of switch statement */

   if (status!=OK)
      recGblSetSevr((dbCommon *)rec_p,cur_stat,cur_sevr);

   return(status);
}

