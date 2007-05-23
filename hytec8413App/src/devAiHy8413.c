/*
=============================================================

  Abs:  EPICS Device Support for the Hytec IP-ADC-8413 Module

  Name: devAiHy8413.c
         *   init_ai            - initialization
         *   read_ai            - read analog input
         *   get_ioint_info_ai  - Get I/O event list info
         *   special_linconv_ai - linear conversion routine

  Proto: None

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
#include "drvHy8413.h"      /* for factor_5pt, etc      */
#include "drvHy8413Lib.h"   /* for drvHy8413_rd() proto */
#include "epicsExport.h"


/* Local prototypes */
static long init_ai( void *rec_p );
static long read_ai( void *rec_p );
static long get_ioint_info_ai( int cmd, void *rec_p,IOSCANPVT *evt_pp );
static long special_linconv_ai(void *rec_p,int after);



/* Local variables */
static double slope = 65535.0;      

/* Global variables */ 
int debugDevHy8413=0;

/*Device support entry table */
struct {
     long       number;       /* number of functions in list        */
     DEVSUPFUN  report;       /* print report - currently not used  */
     DEVSUPFUN  init;         /* called once during ioc init        */
     DEVSUPFUN  init_record;  /* init support for particular record */
     DEVSUPFUN  get_ioint_info;
     DEVSUPFUN  read_write;            /* ptr to read function        */
     DEVSUPFUN  special_linconv;    /* special  lineary conversion */
} devAiHy8413 = {
        6,
        NULL,
        NULL,
        init_ai,
        get_ioint_info_ai,
        read_ai,
        special_linconv_ai };

epicsExportAddress(dset,devAiHy8413);


/*=============================================================

  Abs:  Device Support initialization

  Name: init_ai

  Args: rec_p                          Record information
          Use:  struct
          Type: aiRecord *
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
static long init_ai(void *rec_p)
{
    long                          status = OK;
    unsigned short                type = TYPE_AI;
    unsigned short                nelm   = 1;
    struct instio                *instio_ps = NULL;
    struct aiRecord              *rec_ps  = NULL;

   /*
    * Is this bus type supported for this module?
    */
    rec_ps = (struct aiRecord *)rec_p;
    switch (rec_ps->inp.type) {

        case INST_IO:  /* Instrumentation */
          instio_ps = (struct instio *)&(rec_ps->inp.value);
          rec_ps->dpvt = hytec_ipmInitDev( rec_ps->name,type,nelm,instio_ps->string );
          if ( !rec_ps->dpvt )
            status = S_dev_badInpType;
          else
          {
            rec_ps->eslo = (rec_ps->eguf - rec_ps->egul)/slope;
            rec_ps->roff = rec_ps->eguf;
          }
          break;

        default:       /* Bus type is not supported */
          status = S_dev_badBus;
          break;
    }/* End of switch statement */

    if ( status==OK )
       rec_ps->udf = 0;      /* Init completed successfully */
    else
       recGblRecordError(status,rec_p,"devAiHy8413(init): Illegal INP field");

    return(status);
}

/*=============================================================

  Abs:  Device Support for io scanner init

  Name: get_ioint_info_ai

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
static long get_ioint_info_ai( int cmd, void *rec_p, IOSCANPVT *evt_pp )
{
    long               status=OK;          /* status return        */
    DPVT_ID            devPvt_ps = NULL;   /* private device info  */
    struct aiRecord    *rec_ps;             /* Analog input record  */


    rec_ps  = (struct aiRecord *)rec_p;
    if (rec_ps->dpvt) 
    {
       devPvt_ps = rec_ps->dpvt;
       *evt_pp   = devPvt_ps->card_ps->fifo_s.ioscanpvt;
    }
    return( status );
}


/*=============================================================

  Abs:  Input device support read

  Name: read_ai

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
static long read_ai(void *rec_p)
{
   long                 status=OK;          /* status return            */
   unsigned short       rval=0;             /* raw data value           */
   unsigned short       rescale=0;          /* use channel calibration  */
   unsigned short       i=0;                  /* channel number           */
   unsigned short       cur_stat   = READ_ALARM;      /* alarm status   */
   unsigned short       cur_sevr   = INVALID_ALARM;   /* alarm severity */
   DPVT_ID              devPvt_ps  = NULL;
   IPADC_ID             card_ps    = NULL;
   hytec_ipmCalChan_ts *cal_ps = NULL;
   struct aiRecord     *rec_ps      = (struct aiRecord *)rec_p;
   char                *taskName_c = "devAiHy8413( read )";


   /*
    * If the private device infor has not been
    * filled in then we have a problem and so
    * just exit successfully.  Otherwise, continue.
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

  /*
   * Read data
   */
   devPvt_ps = (DPVT_ID)rec_ps->dpvt;
   card_ps   = devPvt_ps->card_ps;
   status    = drvHy8413_rd(card_ps->io_p,devPvt_ps->i,(short *)&rval);
   if (status==OK)
   {
       i       = devPvt_ps->i;
       cal_ps  = &card_ps->cal_s.chan_as[i];
       rescale = cal_ps->enb;
       if ( rescale && card_ps->cal_s.enb && cal_ps->init )
       {
          rval = drvHy8413_cal_adc( &cal_ps->gain_a[card_ps->format][0],
                                    card_ps->cal_s.type,
                                    card_ps->format,
                                    rval );
   
       }

       if ( !rec_ps->linr )
       {
         rec_ps->val  = rval;
         status = ANLG_NO_CONVERSION;
       }
       else
         rec_ps->rval = rval;
   }
   else
      recGblSetSevr((dbCommon *)rec_p,cur_stat,cur_sevr);
   return(status);
}

/*=============================================================

  Abs:  Linear conversion routine

  Name: special_linconv_ai

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
static long special_linconv_ai(void *rec_p,int after)
{
  long              status = OK;       /* return status  */
  struct aiRecord  *rec_ps = (struct aiRecord *)rec_p;

  if(after)
  {
     rec_ps->eslo = (rec_ps->eguf - rec_ps->egul)/slope;
  }
  return(status);
}

