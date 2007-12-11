/*
=============================================================

  Abs:  Include file for the Hytec IP-ADC-8413 16-bit ADC

  Name: drvHy8413.h

  Side: Must included the following header files
             callback.h - for CALLBACK
             dbScan.h   - for IOSCANPVT
             devLib.h   - for epicsAddressType

  Auth: 19-Sep-2006, Kristi Luchini   (LUCHINI)
  Rev : dd-mmm-yyyy, Reviewer's Name  (USERNAME)

-------------------------------------------------------------
  Mod:
        dd-mmm-yyyy, First Lastname (USERNAME):
           comments

=============================================================
*/
#ifndef DRVHY8413_H
#define DRVHY8413_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/************************************************************

                      Macros

*************************************************************/

#define FULL(csr)     (0)                      
#define DATA_RDY(csr) (csr&0x0040 ? 1 : NULL )
#define ICNT(err)     ( err==0xffff ? err=1 : err++ )
#define MIN(a,b)      ( a<b ? a : b )

/********************************************

              Task information

*********************************************/

/* IO scan task */
#define HY8413_SCAN_NAME      "Hy8413Scan"
#define HY8413_SCAN_PRI       70
#define HY8413_SCAN_OPT       FP_TASK
#define HY8413_SCAN_STACK     (4096 * ARCH_STACK_FACTOR)

/* Fifo full task.  */
#define HY8413_DONE_NAME      "Hy8413Done"
#define HY8413_DONE_PRI       69
#define HY8413_DONE_OPT       FP_TASK
#define HY8413_DONE_STACK     (4096 * ARCH_STACK_FACTOR)

/************************************************************

                      IO Registers                 
                     
*************************************************************/

/* Word offset from base io address */
#define HY8413_CSR          0x0   /* control status register               */
#define HY8413_SAMP_LS      0x2   /* trigger sample num reg (lower word)   */
#define HY8413_SAMP_MS      0x4   /* trigger sample num reg (uper word)    */
#define HY8413_CLK          0x6   /* internal clock rate register          */
#define HY8413_VEC          0x8   /* interrupt vector register             */
#define HY8413_INT_FIFO     0xa   /* adc internal fifo,  16 conversions    */
                                  /* one conversion per channel            */

/* Clock Rate register */
#define HY8413_CLK_RATE_MASK 0xf    
#define HY8413_MIN_CLK_RATE  0
#define HY8413_MAX_CLK_RATE  16

/*
 * The external FIFO, when triggered will store 16384 16-bit words samples for each channel.
 * Once the FIFO is triggered it will continue to store all 16384 samples for each 
 * channel until the FIFO is full. 
 *
 * The FIFO may be readout as it is filling. The data is stored in groups of 16 samples,
 * such that each sample includes all 16-samples as follows:
 *
 *  sample     #1:  1 of 16, 2 of 16, ...16 of 16
 *  sample     #2:  1 of 16, 2 of 16, ...16 of 16
 *   ....
 *  sample #16384: 1 of 16, 2 of 16, ... 16 of 16
 * 
 * Therefore, even though the data can be read out of the FIFO at any time, 
 * groups of 16 word should be read at a time to keep the sample format intact.  
 */
#define HY8413_EXT_FIFO     0xc   /* adc external fifo, 256K conversion    */
                                  /* 16384 conversion per channel          */
/*
 * The external fifo fullness counter register is a 16 bit. This register count 
 * conversions as they are read from the external FIFO. At the end of each trigger and 
 * readout sequence to empty the FIFO, the value in the registers should be zero. 
 * When the FIFO is full (TF) the counter will display 16384(dec) (ie 0x4000) for 256K.
 */
#define HY8413_EXT_FIFO_FULL 0xe  /* external fifo full counter            */

/*
 * There are sixteen ADC buffer registers (addresses 10hex - 2Ehex) which store 
 * the last sampled conversions and may be read at any time.  The channel order
 * is channel 1 at address 10hex to channel 16 at address 2E.  Additionally there
 * are two additional ADC registers to monitor the 0V Reference  (address 30Hex) 
 * and 2.5V Reference  (address 32Hex). All ADC registers are updated simultaneously
 *
 * The ADC data can be configured to straight binary or 2's complement format 
 * dependent upon 2C  bit 3 of the Auxiliary Control Register. Set this bit to 1 
 * for straight binary and 0 for 2's complement operation.
 *
 * The ADC range can also be set to +/-10V or +/-5V dependent upon bit 0 of the 
 * Auxiliary Control Register RGE. Set this bit to 1 for +/-5V and set to
 * 0 for +/-10V operation. At switch on the default will be 2's complement
 * and +/-10V operation
 *
 * The first sixteen ADC buffer registers store the last sample conversions 
 * and may be read at any time.  The seventeenth is used to monitor the 0V 
 * reference and the eighteenth is used to monitor the 2.5V reference.
 */
#define HY8413_ADC          0x10   /* chan 1-16 last conversions */
#define HY8413_ZERO_REF     0x1e   /* 0V reference               */
#define HY8413_2HALF_REF    0x20   /* 2.5V reference             */

/*
 * Auxillary control register. This register is reset to all
 * zeros at switch on and must be set for correct operation
 */
#define HY8413_AUX          0x34


/************************************************************

                      Calibration Type 
                  defined in id prom at BASE+0x98 
                     
*************************************************************/
/* 
 * The type of calibration factores held in the ID PROM are specified at 
 * base+0x98 in the ID PROM, such that:
 *
 *   0 = no calibration factors held in ID PROM
 *   2 = calibration factors stored in ID PROM
 *
 * The calibration factors are held in the ID PROM starting at baxe +0xa0.
 * These values are derived from reading the ADC values at specified voltages.
 * 
 * For the nFS vaule, -10Volts, pHS at +5Volts and PFS at +10Volts. These values 
 * can then be used in the user program equations to correct the offset and
 * gain errors of the individual channels of the ADC.
 */
#define HY8413_NO_CAL_PTS   0   /* no calibration       */
#define HY8413_3PT_CAL_PTS  1   /* calibration 3 pts    */
#define HY8413_5PT_CAL_PTS  2   /* calibration 5 pts    */
#define ID_PG_WCNT          30  /* base+0x80 to base+ba */

/* Word index for each channels calibration data  */
#define HY8413_CAL_NEG_FS      0   /* negative full scale */
#define HY8413_CAL NEG_HS      1   /* negative half scale */
#define HY8413_CAL_ZERO        2   /* zero                */
#define HY8413_CAL_POS_HS      3   /* positive half scale */
#define HY8413_CAL_POS_FS      4   /* positive full scale */

#define POS_FULL_SCALE         4    /* 10V index to calibration voltage */
#define POS_HALF_SCALE         3    /* 5V  index to calibration voltage */
#define ZERO                   2    /* 0V   */
#define NEG_HALF_SCALE         1    /* -5V  */
#define NEG_FULL_SCALE         0    /* -10V */

typedef enum {
  nocal      = 0,
  factor_3pt = 1,
  factor_5pt = 2
} hy8413_calType_te; 
  
/* pg1 chan 0-5, pg2 chan 6-11, pg3 chan 12-16 */
#define HY8413_MAX_CHANS_PER_PAGE 6

/************************************************************

             Control Status Register bit masks (CSR) 

*************************************************************/

#define HY8413_CSR_MASK     0xfff0   /* bits 0-4 not used by control */
/* 
 * The pre-trigger FIFO is full. The FIFO contains the last ADC 
 * conversions. Cleared when the FIFO is read or when a new
 * conversion is read.
 */
#define HY8413_CSR_F        0x0001   /* The pre-trigger FIFO is full.  (R/W) */
#define HY8413_CSR_TF       0x0002   /* The post-trigger FIFO is FULL  (R/W) */
#define HY8413_CSR_FE       0x0004   /* The pre-trigger FIFO is empty  (R)   */
#define HY8413_CSR_THF      0x0008   /* post-trigger FIFO is half full (R/W) */
#define HY8413_CSR_RST      0x0010   /* reset the FIFO when set to 1   (R/W) */

/*
 * DMA Request. When set to 1 allow DMA request between the IP carrier
 * card. DMAREQ0 set when external FIFO is full and DMAREQ1 set when internal
 * FIFO full.
 * Note: At present the External FIFO operation is not available as of Dec 2006.
 *
 * OLD: When set to 1 the internal clock generates a 10MHz clock.
 *      HY8413_CSR_IC       0x0020      Internal clock (R/W) 
 */
#define HY8413_CSR_DRE      0x0020   /* Internal clock (R/W)  */

/* 
 * The post-trigger FIFO is a quatter full. 
 * this bit indicates that the fullness counter has overflowed. (R/W)
 */
#define HY8413_CSR_QF       0x0040  

/* 
 * Enables interrupts when the pre-trigger FIFO is half full. (R/W)
 */
#define HY8413_CSR_EHF      0x0080

/* 
 * Enables interrupts when the pre-trigger FIFO is full. (R/W)   
 */
#define HY8413_CSR_EF       0x0100

/*
 * When set this bit is set to 1 it enables interrupt when the post-trigger
 * FIFO is full.  Optional, this bit is normally set to 0. (R/W)
 */
#define HY8413_CSR_ETF      0x0200

/*
 * If set to 0, the internal 10MHz clock is used to derive the
 * sample rate. If set to 1 the external clock (Strobe*) isused. (R/W)
 */
#define HY8413_CSR_EXT      0x0400   /* External trigger */

/* 
 * Enable go. When strobe is released sampling can begin
 * according to the state of the bit ET. (R/W)
 */
#define HY8413_CSR_EG       0x0800    

#define HY8413_CSR_XC       0x1000    /* external clock (R/W) */

/* 
 * Software trigger. Allows trigger actionto be 
 * initiated by software. (R/W)
 */
#define HY8413_CSR_ST       0x2000

/* 
 * Enable trigger. If this bit is then external triggers are enabled
 * or software trigger will route sampled conversion to the post-trigger
 * FIFO. The pre-trigger FIFO will retain the previous 
 * sampled conversions. The sample number will be stored in the trigger
 * sample number register when a trigger occurs. The action is synchronized
 * to the first sample clock after the rising edge of the trigger. (R/W)
 */
#define HY8413_CSR_ET       0x4000  

/*
 * Arm adc if set. Allows sampling if EG bit is set to zero. (R/W)
 */
#define HY8413_CSR_ARM      0x8000  

/************************************************************

             Auxilary Control Register bit masks (ACR)

*************************************************************/

/*
 * Auxilary Control Register (ACR). This register is used
 * to page the IP PROM.
 * Note: Some IP Carriers only support 64 locations in the ID PROM
 * and this module has 80 16-bit calibration values. Therefore,
 * in such cases were the io memory map is limited to 64 locations
 * the bits PG0 and PG1 in the auxilary control register are used 
 * to switch between pages of the ID PROM.
 * 
 *  PG1  PG0  Page# Notes
 *   0   0    0     Normal VITA4 format for ID PROM (default)
 *   0   1    1     Calibration values for ADC Channel 0-5
 *   1   0    2     Calibration values for ADC Channel 6-11
 *   1   1    3     Calibration values for ADC Channel 12-16
 */
#define HY8413_ACR_MASK         0x1FFF  /* Register Mask (SLAC Version) */

/* 
 * Sets the range of the ADCs such that
 * 0 =  +/-10V
 * 1 =  +-5
 */
#define HY8413_ACR_RGE      0x0001
#define HY8413_ACR_RGE_SHFT 0
typedef enum
{
    ten_plus_minus =0, /* +10V to -10V */
    five_plus_minus=1  /*  +5V to  -5V */
}hy8413_rng_te;


/*
 *  There are two types of operating modes:
 *  1. DC sampling mode - when the unit is armed or receiving an external 10MHz
 *     signal, the inputs are sampled at the programmed clock rate and conversions
 *     placed in a 16 conversion pre-trigger FIFO.
 *  2. Standby mode.
 */
#define HY8413_ACR_NS        0x0004
#define HY8413_ACR_NS_SHFT   2 
typedef enum 
{
  standby=0,
  normal=1,
} hy8413_mode_te;


/* 
 * ADC data format
 * 0 = two's complement (default).
 * 1 = ADC values 0 (Neg FS)-FFFF(Pos FS) 
 */   
#define HY8413_ACR_2C       0x0008 
#define HY8413_ACR_2C_SHFT  3
typedef enum 
{
   twos_compliment=0,
   offset_binary=1
}hy8413_2c_te;
#define OFFSET_BINARY  1
#define TWOS_COMPIMENT 0

#define OFFSET_BINARY_MIN       0x0
#define OFFSET_BINARY_ZERO      0x8000
#define OFFSET_BINARY_MAX       0xffff

#define TWOS_COMPLIMENT_MIN     0x8000
#define TWOS_COMPLIMENT_ZERO    0x0
#define TWOS_COMPLIMENT_MAX     0x7fff		    
/*  
 * Paging:
 * Bit 0 of ID PROM Paging
 */ 
#define HY8413_ACR_PG        0x0070
#define HY8413_ACR_PG_SHFT    4
#define HY8413_MIN_PG         0  /* minimum id prom page number           */
#define HY8413_MAX_PG         6  /* maximum id prom page number           */
#define HY8413_NUM_PG         3  /* number of calibration pages per range */
#define HY8413_MIN_PG_CHAN    4  /* minimum number of channels per page   */
#define HY8413_MAX_PG_CHAN    6  /* maximum number of channels per page   */
typedef enum 
{
    pg0=0,
    pg1=1,
    pg2=2,
    pg3=3,
    pg4=4,
    pg5=5,
    pg6=6
}hy8413_pg_te;

/*
 * Averager ENable bit enables the data averager subsystem. When enabled, 64 successive
 * samples will be summed and divided by 64. The results are then stored in the ADC
 * Data Register. Note that when enabled, the averaging started as soon as the next 
 * ADC sample clock occurs, as lon as the ADC has been set to the normal operation,
 * is ARMed and receives the appropriate triggers.
 * NOTE: Available only in the SLAC version with averaging.
 */
#define HY8413_ACR_AEN       0x0080 
#define HY8413_ACR_AEN_SHFT   7

/*
 * ADN/BUF (A/B): Bit 8
 * This is a dual-mode type, set by the state of the SAM mode bit. 
 *
 * Polling Mode (SAM=0)
 * ADN Read/Write - Special!
 * Averager DoNe: This bit indicates when the average operation has been 
 * completed and valud averaged data is present in the ADC Readout 
 * Registers (Regs 0x10-0x30). Note that this is a dual-use bit. In 
 * read mode, it indicates a done state, in write mode,setting a 1"
 * tells the averager subsystem that its last results have been readout
 * and that it can now resume averaging the next set of 64 samples. 
 * When SAM Readout Mode is enabled, writing to this bit has no effect.
 *   Read:  
 *      0= averaging not done
 *      1= averaging done
 * 
 *   Write: 
 *      0 = no action
 *      1 = Readout complete, start averaging the next sset of 
 *          samples. Note that this bit MUST be set to "0".
 * SAM Mode: (SAM=1) 
 * Indicates which of the two ping-pong buffers is ready for
 * NOTE: Available only in the SLAC version with averaging.
 */
#define HY8413_ACR_ADN       0x0100 
#define HY8413_ACR_ADN_SHFT  8

/*
 * ASR: Bit 9
 * ADC Readout Select, this bit selects whether the ADC Readout
 * Registers (rev 0x10-0x30) contain the latest digitized values
 * or the averaged data values. This bit can be changed at any time.
 * NOTE: Available only in the SLAC version with averaging.
 */
#define HY8413_ACR_ARS       0x0200 
#define HY8413_ACR_ARS_SHFT  9

#define HY8413_ARS_NORMAL    0
#define HY8413_ARS_AVE       1

/*
 * AINI: Bit 10
 *  Averager INItialize initializes the average subsystem,
 * which 
 *     1. resets all state machine
 *     2.  zeroes out all counters
 *     3. sets all averager logic to the idle state 
 *   0=
 * NOTE: Available only in the SLAC version with averaging.
 */
#define HY8413_ACR_AINI       0x0400 
#define HY8413_ACR_AINI_SHFT  10

#define HY8413_AINI_NORMAL    0
#define HY8413_AINI_INIT      1

/*
 * SAM: Bit 11
 * SAM Mode Enable: Enables SAM readout mode, where
 * the averaged data is automatically written into an
 * output buffer while another buffer accumulates sampled
 * data. When a new set of averages is available, the buffers
 * are automaticallly swapped.
 * NOTE: Available only in the SLAC version with averaging.
 */
#define HY8413_ACR_SAM       0x0800 
#define HY8413_ACR_SAM_SHFT  11

#define HY8413_SAM_DIS       0  /* disabled */
#define HY8413_SAM_ENB       1  /* enabled  */

/*
 * BUF: Bit 12
 * Readout BUFfer Status: In SAM Readout Mode, this bit
 * indicates which of the two ping-pong buffers is ready for
 * readout. As a convenience, changes in the state of this bit
 * can be monitored by software to indicate when new data is
 * available.
 *
 *  Read Only (while in SAM Mode)
 *     0 = Buffer A is available for external readout
 *     1 = Buffer B is available for externaml readout
 *
 * NOTE: Available only in the SLAC version with averaging.
 */

#define HY8413_ACR_BUF       0x1000 
#define HY8413_ACR_BUF_SHFT  12

#define HY8413_BUF_A       0  /* Buffer A is available for external readout  */
#define HY8413_BUF_B       1  /* Buffer B is available for external readout */

/************************************************************

                   I/O Register Map

*************************************************************/

#define HY8413_NUM_CHAN        16           /* total number of channels     */
#define HY8413_MAX_CHAN        15          /* maximum signal (chan) number */
#define HY8413_FIFO_BCNT      (1024*256)   /* 256K fifo length             */

/* 
 * The adc register readout has 16 buffer registers (from base+0x10 to base+0x2e),
 * which store the last sampled conversions and may be read at any time. The 
 * channel order is channel1 at address base+10x to channel 16 at base+0x2e).
 * The data format is two's compliment such that 0x1000=-10V adn 0x7fff=+10V.
 */
/*
 * All device registers must be declared as
 * ANSI C volatile so we dont need to use 
 * the -fvolatile flag (and don't limit the optimizer).
 * So make sure that you use the IO_MAP structure to define this structure
 */
typedef volatile struct hy8413_io_s
{
   /* 
    * Control status register (R/W)
    * address: base +0x0 
    */
   unsigned short       csr;            

  /* 
   * Number of samples stored per trigger.
   * address: base +0x2 (LSB)
   *          base +0x4 (MSB)
   */
   unsigned short        nsamples_a[2];  
  /* 
   * The clock rate register is a four bit register which enables codes 0-16
   * to generate frequencies of 1Hz to 200kHz in multiples of 1,2,3 or 10.
   * (E.g. 0=1Hz,1=2Hz,2=5Hz,3=10Hz, etc). Each clock pulse will initiate
   * simultaneous ADC conversions and store them in memory. (R/W)
   * address: base +0x6
   */
   unsigned short       clk_rate;     

  /*
   * Interrupt vector (0-255) (R/W)
   * address: base +0x8
   */
   unsigned short       vec;    

   struct {
    /*
     * Read the internal FIFO memory (16 conversions).
     * Reset Full and set FE when the FIFO is emptied.
     * address: base +0xa
     */
    unsigned short internal;
    /*
     * Read the External FIFO memory (256K conversions).
     * Reset TF and TFE when FIFO is emptied.
     * When the external fifo is triggered it will store 16384 16-bit samples
     * of each channel. Once the FIFO is triggered it will continue to sture
     * all 16384 samples of each channel until it is full.
     * address: base +0xc
     */
    unsigned short external;  
    
     /*
      * This is the FIFO counter. Two up/down counters which
      * count conversions as they are entered/read form the pre-trigger 
      * FIFO (least significant byte) and post-trigger FIFO (most significant byte).
      * At the end of each trigger/readout sequence the value in the registers
      * should be zero. Since the capacity is 64K it should be used in conjunction
      * with post trigger FIFO quater (QF), half full (THF)  for 128K and full (TF)
      * for 256K. (R/W)
      * address: base +0xe
      */
     unsigned short full;   /* base+0xe */

  }fifo_s;

   /* 
    * The adc register readout has 16 buffer registers (from base+0x10 to base+0x2e),
    * which store the last sampled conversions and may be read at any time. The 
    * channel order is channel 0 at address base+10x to channel 16 at base+0x2e).
    * The data format is dependent on the setting of the acr register
    * Note: two's compliment such that 0x1000=-10V, 0x0=0V, 0x7fff=+10V.
    * 
    * ADC channel data and reference voltage (R)
    * address: base +0x10 to base+0x2e 
    */
    unsigned short  adc_a[HY8413_NUM_CHAN];
    unsigned short  ref_zero_volt;
    unsigned short  ref_2_5_volt;

   /*
    * Auxilary Control Register (ACR).
    * This register is used to page the IP PROM.
    * address: base +0x10 to base+0x34
    */ 
   unsigned short acr;
   
} hy8413_io_ts;

typedef struct hy8413_io_s  * HY8413_IO;

/************************************************************

                   ID PROM Register Map

*************************************************************/

typedef volatile struct hy8413_idProm_s
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
}hy8413_idProm_ts;

typedef struct hy8413_idProm_s * HY8413_ID;

typedef struct hy8413_calData_s
{
   unsigned short negFS;
   unsigned short negHS;
   unsigned short zero;
   unsigned short posHS;
   unsigned short posFS;
}hy8413_calData_ts;

#ifdef __cplusplus
}
#endif /* __cplusplus  */

#endif /* DRVHY8413_H  */
