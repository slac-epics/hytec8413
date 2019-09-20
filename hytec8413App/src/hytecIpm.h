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

------------------------------------------------------------
  Mod:
        dd-mmm-yyyy, First Lastname (USERNAME):
           comments

=============================================================
*/
#ifndef HYTECIPM_H
#define HYTECIPM_H

#if (EPICS_REVISION == 14 && EPICS_MODIFICATION >= 11) || (EPICS_REVISION == 15) || (EPICS_VERSION == 7)
#include "ellLib.h"
#endif

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

#define TYPE_BI                  0         /* bi record type               */
#define TYPE_MBBI                1         /* mbbi record type             */
#define TYPE_MBBI_DIRECT         2         /* mbbiDirect record type       */
#define TYPE_AI                  3         /* analog input record          */
#define TYPE_WF                  4         /* waveform input record        */
#define TYPE_BO                  5         /* bo record type               */
#define TYPE_MBBO                6         /* mbbo record type             */
#define TYPE_MBBO_DIRECT         7         /* mbboDirect record type       */
#define TYPE_LI                  8         /* Longin record type           */

#define MAX_BITS                 16        /* max num of bits in acr,csr   */

/* Interrupt limits */
#define DEFAULT_INT_LEVEL        3          /* default interrupt level     */
#define MIN_INT_LEVEL            0          /* minimum interrupt level     */
#define MAX_INT_LEVEL            7          /* maximum interrupt level     */
#define MIN_VEC_NUM              0          /* minimum interrupt vector    */
#define MAX_VEC_NUM             255         /* maximum interrupt vector    */

#define MAX_CA_STRING_SIZE      40
#define MAX_CHAN                16          /* maximum number of channels      */
#define MAX_ID_PAGES            3           /* maximum number of ID PROM pages */
#define MIN_CAL_PTS             3           /* minimum num of cal pts per chan */
#define MAX_CAL_PTS             5           /* maximum num of cal pts per chan */
#define NUM_CAL_TYPES           3           /* number of calibration  types    */
#define CAL_MASK               0x3          /* mask for calibration type       */   

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
#define  IPAC_ID_SPACE_WCNT  64

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
    unsigned short    calType;      /* BASE+0x98 */
    unsigned short    serialNo;     /* BASE+0x9A */
    unsigned short    spare;        /* BASE+0x9C */
    unsigned short    WLO;          /* BASE+0x9E */
    unsigned short    packSpecific_a[48];
}hytec_ipac_idProm_ts;

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
typedef union hytec_ipac_idProm_u { 
  unsigned short        _a[IPAC_ID_SPACE_WCNT];
  ipac_idProm_t         std_s;
  hytec_ipac_idProm_ts  hytec_s;
} hytec_ipac_idProm_tu;

typedef union hytec_ipac_idProm_u  * HYTEC_ID;

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

                   Module Calibration Information

*************************************************************/
#define NUM_FORMATS  2
typedef struct hytec_ipmCalChan_s
{
   unsigned short    init;      /* successfully read cal data      */
   unsigned short    enb;       /* use calibration flag           */
  /*
   * The data calibration data is stored in the id prom
   * memory as offset binary format. However, since the
   * data can be read in either offset binary or two's 
   * compliment (see auxilliary control register), the
   * calibration data is stored locally in both data
   * formats.
   */
   unsigned short    gain_a[NUM_FORMATS][MAX_CAL_PTS];
} hytec_ipmCalChan_ts;

typedef struct  hytec_ipmCal_s
{
    unsigned short       type;          /* calibration type                */
    unsigned short       enb;           /* calibrate enable                */
    short                npts;          /* number of calibration poits     */
    hytec_ipmCalChan_ts  chan_as[MAX_CHAN];
}hytec_ipmCal_ts;

/************************************************************

                   Module Configuration
                (see: EPICS driver support)

*************************************************************/

typedef void (*VOIDFUNPTR)(void);

typedef struct hytec_ipmConfig_s {

  ELLNODE                 node;          /* Link List Node                  */
  unsigned short          carrier;       /* Industry Pack Carrier Index     */
  unsigned short          slot;          /* Slot number on carrier          */
  unsigned short          model;         /* Hytec ID PROM model number      */
  unsigned short          serialNo;      /* Serial Number                   */
  unsigned short          rev;           /* revision                        */
  char                   *name_c;        /* Card name, NULL terminated      */

  unsigned short          range;         /* voltage range of module         */
  unsigned short          mode;          /* operating mode                  */
  unsigned short          format;        /* adc data format                 */

  hytec_ipmErr_ts         err_cnt_s;
  unsigned short          nchan;         /* Number of channels              */
  unsigned long           init;          /* initialize flag                 */
  unsigned char           intHandler;    /* interrupt handler flag          */
  int                     arm;
  epicsMutexId            lock;
  unsigned char           intVec;        /* interrupt vector                */
  int                     intLevel;      /* interrupt level                 */

  volatile void          *io_p;          /* io register map (A16)           */
  hytec_ipac_idProm_tu   *id_pu;         /* id prom         (A16)           */ 
  volatile void          *mem_p;         /* memory address  (A24)           */

  hytec_ipmCal_ts         cal_s;          /* calibration information             */

  IOSCANPVT              biScan_a[2][MAX_BITS];
  IOSCANPVT              mbbiScan_a[MAX_BITS];
  IOSCANPVT              calEnbScan;
  struct 
  {
    unsigned short state;
    IOSCANPVT      ioscanpvt;

  } fifo_s;

  /* Module specific functions */
  struct 
  {
    VOIDFUNPTR           isr_pf;         /* Address of Interrupt Service Routine */
    VOIDFUNPTR           usr_pf;         /* Address of user function             */
    VOIDFUNPTR           rdWf_pf;        /* Address of read wavefor routine      */
    VOIDFUNPTR           rdCal_pf;       /* Address of read calibration routine  */
  }rtn_s;

}hytec_ipmConfig_ts;

typedef struct hytec_ipmConfig_s * IPADC_ID;

/************************************************************

                    Device Configuration
                 (see: EPICS device support)

*************************************************************/
typedef enum 
{
  ReadCSR       = 0,
  ReadACR       = 1,
  ReadIO        = 2,
  ReadID        = 3,
  ReadCAL       = 4,
  ReadDATA      = 5,
  SetCSR        = 6,
  SetACR        = 7,
  SetIO         = 8,
  SetID         = 9,
  SetCAL        = 10
} hytec_func_te;

#define REG_IO_CSR  "CSR"
#define REG_IO_ACR  "ACR"
#define REG_IO      "IO"
#define REG_ID      "ID"
#define REG_SW_CAL  "CAL"  /* This is software related items */
#define REG_IO_DATA "DATA"
#define REG_TYPE_NUM 6


typedef struct hytec_devicePvt_s
{
  IPADC_ID             card_ps; /* IPAC card information        */
  short                i;       /* channel num or word offset   */
  unsigned short       nelm;    /* number of elements to read   */
  unsigned short       recType; /* type of record               */
  hytec_func_te        func;    /* type of operation            */
  hytec_ipmStatus_te   status;  /* status of operation          */
} hytec_devicePvt_ts;

typedef struct hytec_devicePvt_s          * DPVT_ID;

#ifdef __cplusplus
}
#endif /* __cplusplus  */

#endif /* HYTECIPM_H  */

