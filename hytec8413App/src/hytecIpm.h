/*
=============================================================

  Abs:  Include file for the generic Hytec ADC Driver Utils

  Name: hytecIpm.h

  Side: Must included the following header files
             ipac.h      - for ipac_idProm_t
             dbScan.h    - for IOSCANPVT
             epicsMux.h  - for epicsMutexId
             ellLib.h    - for ELLNODE

  Auth: 19-Sep-2006, Kristi Luchini   (LUCHINI)
  Rev : dd-mmm-yyyy, Reviewer's Name  (USERNAME)

-------------------------------------------------------------
  Mod:
        dd-mmm-yyyy, First Lastname (USERNAME):
           comments

=============================================================
*/
#ifndef HYTECIPM_H
#define HYTECIPM_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/************************************************************

                      Miscellaneous Parameters

*************************************************************/

/* Driver and device support status codes */
#ifndef OK
#define OK 0
#endif
#ifndef ERROR
#define ERROR -1
#endif
#define ANLG_NO_CONVERSION       2          /* device support not convsion */

/* Interrupt limits */
#define DEFAULT_INT_LEVEL        3          /* default interrupt level     */
#define MIN_INT_LEVEL            0          /* minimum interrupt level     */
#define MAX_INT_LEVEL            7          /* maximum interrupt level     */
#define MIN_VEC_NUM              0          /* minimum interrupt vector    */
#define MAX_VEC_NUM             255         /* maximum interrupt vector    */

#define MAX_CA_STRING_SIZE      40
#define MAX_CHAN                16          /* maximum number of channels */

/************************************************************

                   Clock Rates

*************************************************************/

/* The clock rate is defined in bits 0-3 of the clock
   rate register. The rates are listed below:
 
         0 =      1Hz
         1 =      2Hz
         2 =      5Hz
         3 =     10Hz
         4 =     20Hz
         5 =     50Hz
         6 =    100Hz
         7 =    200Hz
         8 =    500Hz
         9 =   1000Hz
         10 =  2000Hz
         11 =  5000Hz
         12 = 10000Hz
         13 = 20000Hz
         14 = 50000Hz
         15 =100000Hz
*/

/************************************************************

                   ID PROM Register Map

*************************************************************/

#define  HYTEC_PROM_MANUF    0x80
#define  HYTEC_IP8413_MODEL  0x8413
#define  HYTEC_IP8402_MODEL  0x8402

/*
 * For some reason Hytec is not using Ipac standard
 * for the ID PROM layout, so the structure below is
 * what should be used.
 * WARNING:!!!  
 * when using the ipac driver calls such as ipmValidate() 
 * you will get an invalid return because of Hytec's
 * non-standard usage of this space. So special software
 * routines must deal with this mess.
 */
#define IPAC_IO_SPACE_WCNT  64
#define IPAC_ID_SPACE_WCNT  64

typedef volatile struct hytec_ipac_idProm_s
{
  /* standards registerd */
    unsigned short asciiI;           /* BASE+0x80 "VI"     */
    unsigned short asciiP;           /* BASE+0x82 "TA"     */
    unsigned short asciiA;           /* BASE+0x84 "4 "     */
    unsigned short asciiC;           /* BASE+0x86 unused   */
    unsigned short manufacturerId;   /* BASE+0x88 unused   */
    unsigned short modelId;          /* BASE+0x8A          */
    unsigned short revision;         /* BASE+0x8C          */
    unsigned short reserved;         /* unused */
    unsigned short driverIdLow;      /* unused */
    unsigned short driverIdHigh;     /* unused */
    unsigned short bytesUsed;        /* unused */
    unsigned short CRC;              /* unused */

    /* Pack specified */
    unsigned short    calibration;    /* BASE+0x98 */
    unsigned short    serialNo;       /* BASE+0x9A */
    unsigned short    spare[50];
}hytec_ipac_idProm_ts;

typedef union {
  short                 _a[IPAC_ID_SPACE_WCNT];
  ipac_idProm_t         std_s;
  hytec_ipac_idProm_ts  hytec_s;
} hytec_ipac_idProm_tu;

/************************************************************
                   Status Codes

*************************************************************/

#define M_drvLib (1003<<16U)
#define drvError(CODE) (M_drvLib | (CODE))
 
#define S_drv_OK 0                     /* success                                          */
#define S_drv_badParam   drvError(1)   /* driver: bad parameter                            */
#define S_drv_noMemory   drvError(2)   /* driver: no memory                                */
#define S_drv_noDevice   drvError(3)   /* driver: device not configured                    */
#define S_drv_invSigMode drvError(4)   /* driver: signal mode conflicts with device config */
#define S_drv_cbackChg   drvError(5)   /* driver: specified callback differs from previous config*/
#define S_drv_alreadyQd  drvError(6)   /* driver: a read request is already queued for the chan  */

typedef enum
{       CardNotFound,
        InvChan,
        InvCard,
        InvBusType,
        NotRegistered,
        NoGate,
        TooManyInt,
        DataInvSeq,
        OldData,
        FifoEmpty,
        Timeout,
        Failure,
        Reading,
        AckTimeout,
        Reset,
        Success
} hytec_ipmStatus_te;

typedef struct  drvHytec_err_s
{
        unsigned short  cnt;             /* total count                              */
        unsigned short  tmo;             /* timeout counter                          */
        unsigned short  inv_data;        /* sequence error                           */
        unsigned short  old_data;        /* data read from FIFO was old              */
        unsigned short  too_many_int;    /* zero ticks passed since last interrupt   */
} hytec_ipmErr_ts;

/************************************************************

                   Module Configuration
                (see: EPICS driver support)

*************************************************************/

typedef struct hytec_ipmConfig_s
{
  ELLNODE                 node;          /* Link List Node                  */
  unsigned short          carrier;       /* Industry Pack Carrier Index     */
  unsigned short          slot;          /* Slot number on carrier          */
  unsigned short          model;         /* Hytec ID PROM model number      */
  char                   *name_c;        /* Card name, NULL terminated      */

  unsigned short          nchan;         /* Number of channels              */
  unsigned long           init;          /* initialize flag                 */
  IOSCANPVT               ioscanpvt;     /* array of io ioscan lists        */

  epicsMutexId            lock;
  unsigned char           intVec;        /* interrupt vector                */
  int                     intLevel;      /* interrupt level                 */

  volatile void          *io_p;          /* io register map (A16)           */
  ipac_idProm_t          *id_ps;         /* id prom         (A16)           */ 
  volatile void          *mem_p;         /* memory address  (A24)           */

  unsigned short         fifo_state;     /* fifo state                      */
  hytec_ipmErr_ts        err_cnt_s;

  unsigned short         opMode;         /* operating mode                  */

  unsigned short         calMode;        /* 0=No calibration, 2=calibration */
  unsigned short         gain_a[MAX_CHAN];

}hytec_ipmConfig_ts;

typedef struct hytec_ipmConfig_s * IPADC_ID;

/************************************************************

                    Device Configuration
                 (see: EPICS device support)

*************************************************************/
typedef enum 
{
  ReadAdc  = 0,
  ReadFifo =1
} hytec_func_te;

typedef struct hytec_devicePvt_s
{
  IPADC_ID             card_ps;
  unsigned short       chan;
  unsigned short       nelm;
  hytec_func_te        func;
  hytec_ipmStatus_te      status;
}hytec_devicePvt_ts;

typedef struct hytec_devicePvt_s * DPVT_ID;

#ifdef __cplusplus
}
#endif /* __cplusplus  */

#endif /* HYTECIPM_H  */

