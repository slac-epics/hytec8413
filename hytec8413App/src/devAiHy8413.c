/*
=============================================================

  Abs:  EPICS Device Support for the Hytec IP-ADC-8413 Module

  Name: devAiHy8413.c

         *   ai_init_record     - initialization
         *   ai_rd_record       - read analog input
         *   ai_get_ioint_info  - Get I/O event list info
         *   ai_special_linconv - linear conversion routine

   Proto: 

  Auth: 19-Sep-2006, K. Luchini       (LUCHINI)
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
#include "aiRecord.h"       /* struct aiRecord          */
#include "alarm.h"          /* READ_ALARM,INVALID_ALARM */    
#include "recGbl.h"         /* recGblRecordError proto  */
#include "dbCommon.h"       /* for dbCommon             */
#include "dbScan.h"         /* IOSCANPVT                */
#include "devSup.h"         /* DEVSUPFUN, S_dev_badBus  */
#include "drvIpac.h"        /* ipac_idProm_t            */
#include "hytecIpm.h"       /* for IPADC_ID,DPVT_ID     */
#include "hytecIpmLib.h"    /* for hytec_ipmInitDev()   */
#include "drvHy8413Lib.h"   /* for drvHy8413_rd() proto */
#include "epicsExport.h"


/* Local prototypes */
static long ai_init_record( void *rec_p );
static long ai_rd_record( void *rec_p );
static long ai_get_ioint_info( int cmd, void *rec_p,IOSCANPVT *evt_pp );
static long ai_special_linconv(void *rec_p,int after);


/* Local variables */
static double slope = 65535.0;      

/* 
 * Global variables - device support entry table 
 * for conventional AI record  
 */
struct {
     long       number;       /* number of functions in list        */
     DEVSUPFUN  report;       /* print report - currently not used  */
     DEVSUPFUN  init;         /* called once during ioc init        */
     DEVSUPFUN  init_record;  /* init support for particular record */
  /*
   * This routine is required for any device
   * type that can use the ioEvent scanner.
   * It is called by the ioEventScan system each
   * time the record is added or deleted from
   * an I/O event scan list.  The argument "cmd"
   * of this function, has the value (0,1) if the
   * record is being added to or deleted from,
   * an I/O scan list.  It must be provided for
   * any device type that can use the ioEvent scanner.
   */
     DEVSUPFUN  get_ioint_info;

     DEVSUPFUN  read_ai;            /* ptr to read function        */
     DEVSUPFUN  special_linconv;    /* special  lineary conversion */
} devAiHy8413 = {
        6,
        NULL,
        NULL,
        ai_init_record,
        ai_get_ioint_info,
        ai_rd_record,
        ai_special_linconv };

epicsExportAddress(dset,devAiHy8413);


/*=============================================================

  Abs:  Device Support initialization

  Name: ai_init_record

  Args: rec_p                          Record information
          Use:  struct
          Type: biRecord *
          Acc:  read-write access
          Mech: By reference

  Rem:  This device support routine is called by the record
        support function init_record(). Its purpose it to
        initializes analog input records.

  Side: INST_IO is the only bus type supported

  Ret: long
         OK               - Successful operation
         S_dev_badBus     - Failure, due to bad BUS type
         Otherwise see failed return from routines called:
            hy8413_getConfig()

=============================================================*/
static long ai_init_record(void *rec_p)
{
    long                          status = OK;
    struct instio                *instio_ps = NULL;
    struct aiRecord              *ai_ps = NULL;
    static const hytec_func_te    func = ReadAdc;
    static const unsigned short   nelm = 1;

   /*
    * Is this bus type supported for this module?
    */
    ai_ps = (struct aiRecord *)rec_p;
    switch (ai_ps->inp.type) {

        case INST_IO:  /* Instrumentation */
          instio_ps = (struct instio *)&(ai_ps->inp.value);
          ai_ps->dpvt = hytec_ipmInitDev( ai_ps->name,
                                          (unsigned short)func,
                                          nelm,
                                          instio_ps->string );
          if ( ai_ps->dpvt )
          {
            ai_ps->eslo = (ai_ps->eguf - ai_ps->egul)/slope;
            ai_ps->roff = ai_ps->eguf;
          }
          break;

        default:       /* Bus type is not supported */
          status = S_dev_badBus;
          break;
    }/* End of switch statement */

    if ( status==OK )
       ai_ps->udf = 0;      /* Init completed successfully */
    else
       recGblRecordError(status,rec_p,"devAiHy8413(init): Illegal INP field");

    return(status);
}

/*=============================================================

  Abs:  Device Support for io scanner init

  Name: ai_get_ioint_info

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
static long ai_get_ioint_info( int cmd, void *rec_p, IOSCANPVT *evt_pp )
{
    long               status=OK;          /* status return        */
    DPVT_ID            devPvt_ps = NULL;   /* private device info  */
    struct aiRecord    *ai_ps;             /* Analog input record  */


    ai_ps  = (struct aiRecord *)rec_p;
    if (ai_ps->dpvt) 
    {
       devPvt_ps = ai_ps->dpvt;
       *evt_pp   = devPvt_ps->card_ps->ioscanpvt;
    }
    return( status );
}


/*=============================================================

  Abs:  Input device support read

  Name: ai_rd_record

  Args: rec_p                      Record information
          Use:  struct
          Type: void *
          Acc:  read-write access
          Mech: By reference

  Rem: This routine processes a analog input record.
       The floating point read from the specified hardware
       memeory location, and stored into the VAL field.

  Side: Conversion from a raw value to engineering units
        will not be performed if the field "LINR" is zero.

  Ret: long
         OK                 - Successful operation (convert rval->val)
         INVALID_ALARM      - Failure occured during write
         Otherwise, see return from
             hy8413_read()
             alarmStatusChk()

=============================================================*/
static long ai_rd_record(void *rec_p)
{
   long               status=OK;       /* status return            */
   short              value;           /* analog data from channel */
   unsigned short     cur_stat   = READ_ALARM;   /* alarm status   */
   unsigned short     cur_sevr   = INVALID_ALARM;/* alarm severity */
   DPVT_ID            devPvt_ps  = NULL;
   struct aiRecord   *ai_ps      = (struct aiRecord *)rec_p;
   char              *taskName_c = "devAiHy8413( read )";


   /*
    * If the private device infor has not been
    * filled in then we have a problem and so
    * just exit successfully.  Otherwise, continue.
    */
   if ( !ai_ps->dpvt ) 
   {
       status = recGblSetSevr(ai_ps,cur_stat,cur_sevr);
       if ( status  &&  errVerbose && 
           ((ai_ps->stat!=cur_stat) || 
            (ai_ps->sevr!=cur_sevr)) )
         recGblRecordError(ERROR,(void *)ai_ps,taskName_c ); 
      return(ERROR);
   }

  /*
   * Read data
   */
   devPvt_ps = (DPVT_ID)ai_ps->dpvt;
   status    = drvHy8413_rd((void *)devPvt_ps->card_ps->io_p,
                            devPvt_ps->chan, 
                            &value);
   if (status==OK)
   {
      if ( !ai_ps->linr )
      {
        ai_ps->val  = value;
        status = ANLG_NO_CONVERSION;
      }
      else 
      {
        ai_ps->rval = value;
      }
   }
   else
      recGblSetSevr((dbCommon *)rec_p,cur_stat,cur_sevr);
   return(status);
}

/*=============================================================

  Abs:  Linear conversion routine

  Name: ai_special_linconv

  Args: rec_p                      Record information
          Use:  struct
          Type: void *
          Acc:  read-write access
          Mech: By reference


  Rem: This routine performs a linear conversion using the
       raw adc counts found in the RVAL field from the hardware
       and placing the converted value in engineering units
       into the VAL field.

  Side: None

  Ret: long
         OK     - Successful operation (always)

=============================================================*/
static long ai_special_linconv(void *rec_p,int after)
{
  long             status = OK;
  struct aiRecord *ai_ps  = (struct aiRecord *)rec_p;

  if(after)
     ai_ps->eslo = (ai_ps->eguf - ai_ps->egul)/slope;
  return(status);
}

