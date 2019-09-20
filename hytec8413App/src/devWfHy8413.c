/*
=============================================================

  Abs:  EPICS Device Support for the Hytec IP-ADC-8413 Module
        for Waveform Input 

  Name: devWfHy8413.c
         *   init_wf     - initialization
         *   read__wf    - read analog input

   Proto: None

  Auth: 21-Jan-2007, K. Luchini       (LUCHINI)
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
#if (EPICS_REVISION == 14 && EPICS_MODIFICATION >= 11) || (EPICS_REVISION == 15) || (EPICS_VERSION == 7)
#include  "errlog.h"
#endif
#include "cantProceed.h"
#include "menuFtype.h"      /* For menuFtypeSHORT           */
#include "waveformRecord.h" /* struct wfRecord              */
#include "alarm.h"          /* READ_ALARM,INVALID_ALARM     */    
#include "recGbl.h"         /* recGblRecordError proto      */
#include "dbCommon.h"       /* for dbCommon                 */
#include "devLib.h"         /* for S_dev_badSignalCount     */
#include "dbScan.h"         /* IOSCANPVT                    */
#include "devSup.h"         /* DEVSUPFUN, S_dev_badBus      */
#include "drvIpac.h"        /* ipac_idProm_t                */
#include "hytecIpm.h"       /* for IPADC_ID,DPVT_ID         */
#include "hytecIpmLib.h"    /* for hytec_ipmInitDev()       */
#include "drvHy8413.h"      /* for factor_3pt               */
#include "drvHy8413Lib.h"   /* for drvHy8413_get_cal() proto*/
#include "epicsExport.h"


/* Local prototypes */
static long init_wf( void *rec_p );
static long read_wf( void *rec_p );

/* 
 * Global variables - device support entry table 
 * for conventional Analog Input Array record  
 */
struct {
     long       number;       /* number of functions in list        */
     DEVSUPFUN  report;       /* print report - currently not used  */
     DEVSUPFUN  init;         /* called once during ioc init        */
     DEVSUPFUN  init_record;  /* init support for particular record */
     DEVSUPFUN  get_ioint_info;
     DEVSUPFUN  read_write;           /* ptr to read function        */
} devWfHy8413 = {
        5,
        NULL,
        NULL,
        init_wf,
        NULL,
        read_wf };

epicsExportAddress(dset,devWfHy8413);


/*=============================================================

  Abs:  Device Support initialization

  Name: init_wf

  Args: rec_p                          Record information
          Use:  struct
          Type: aaiRecord *
          Acc:  read-write access
          Mech: By reference

  Rem:  This device support routine is called by the record
        support function init_record(). Its purpose it to
        initializes analog input array records.

  Side: INST_IO is the only bus type supported

  Ret: long
         OK               - Successful operation
         S_dev_badBus     - Failure, due to bad BUS type
         Otherwise see failed return from routines called:
            hy8413_getConfig()

=============================================================*/
static long init_wf(void *rec_p)
{
    long                          status = OK;
    unsigned short                i      = 0;         /* channel number                  */
    unsigned short                type   = TYPE_WF;   /* channel data                    */
    static const unsigned short   nelm   = 5;         /* number elmements in func_s list */
    struct instio                *instio_ps = NULL;
    struct waveformRecord        *rec_ps  = NULL;
    DPVT_ID                       devPvt_ps = NULL;
    IPADC_ID                      card_ps   = NULL;

   /*
    * Is this bus type supported for this module?
    */
    rec_ps = (struct waveformRecord *)rec_p;
    switch (rec_ps->inp.type) 
    {
        case INST_IO:  /* Instrumentation */
          instio_ps = (struct instio *)&(rec_ps->inp.value);
          rec_ps->dpvt = hytec_ipmInitDev( rec_ps->name,type,nelm,instio_ps->string );
          if ( rec_ps->dpvt ) 
          {
            devPvt_ps = (DPVT_ID)rec_ps->dpvt;
            card_ps   = devPvt_ps->card_ps;
            i         = devPvt_ps->i;
            if ( (rec_ps->ftvl != menuFtypeUSHORT) || (rec_ps->nelm < MAX_CAL_PTS) )
              status = S_dev_badInpType;
	  }  
          else
            status = S_dev_badRequest;                  
	  break;

        default:       /* Bus type is not supported */
          status = S_dev_badBus;
          break;
    }/* End of switch statement */

    if ( status==OK )
       rec_ps->udf = 0;      /* Init completed successfully */
    else
       recGblRecordError(status,rec_p,"devWfHy8413( init): Illegal INP field");

    return(status);
}


/*=============================================================

  Abs:  Input device support read

  Name: read_wf

  Args: rec_p                      Record information
          Use:  struct
          Type: void *
          Acc:  read-write access
          Mech: By reference

  Rem: This routine processes a waveform record.
       The floating point read from the specified hardware
       memeory location, and stored into the VAL field.

  Side: None

  Ret: long
         OK                 - Successful operation (convert rval->val)
         INVALID_ALARM      - Failure occured
         READ_ALARM         - Read error occured 
         Otherwise, see return from
             hy8413_read()
             alarmStatusChk()

=============================================================*/
static long read_wf(void *rec_p)
{
   long                   status=OK;       /* status return            */
   short                  i          = 0;  /* index                    */
   short                  j          = 1;  /* increment counter        */
   unsigned short        *data_a     = NULL;    
   unsigned short         cur_stat   = READ_ALARM;   /* alarm status   */
   unsigned short         cur_sevr   = INVALID_ALARM;/* alarm severity */
   DPVT_ID                devPvt_ps  = NULL;
   IPADC_ID               card_ps    = NULL;
   struct waveformRecord *rec_ps     = NULL;
   hytec_ipmCalChan_ts   *cal_ps     = NULL;
   char                  *taskName_c = "devWfHy8413( read )";
   

   /*
    * If the private device infor has not been
    * filled in then we have a problem and so
    * just exit successfully.  Otherwise, continue.
    */
   rec_ps = (struct waveformRecord *)rec_ps;
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

  /*
   * Read data
   */
   devPvt_ps = (DPVT_ID)rec_ps->dpvt;
   card_ps   = devPvt_ps->card_ps;
   switch( devPvt_ps->func ) 
   {
      case ReadID:
        if ( card_ps->cal_s.type  && card_ps->cal_s.npts )
	{
          i      = devPvt_ps->i;
          cal_ps = &card_ps->cal_s.chan_as[i];
          data_a = (unsigned short *)rec_ps->bptr;

          /* Read calibration data */
          if ( card_ps->cal_s.type==factor_3pt ) j=2; 
	  for (i=0,j=0; i<MAX_CAL_PTS; i+=j) 
	    data_a[i] = cal_ps->gain_a[card_ps->format][i];
	} 
        break;
          
      default:
        printf("%s:  devSup has not been implimented for %s\n",taskName_c,rec_ps->name);
	status = ERROR;
	  break;    
   }/* End of switch statement */

   rec_ps->nord = i-1;
   if (status!=OK)
      recGblSetSevr((dbCommon *)rec_p,cur_stat,cur_sevr);
   return(status);
}
