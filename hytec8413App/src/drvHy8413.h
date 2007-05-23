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
 * When set to 1 the internal clock generates a 10MHz clock. 
 */
#define HY8413_CSR_IC       0x0020   /* Internal clock (R/W)  */

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

#define HY8413_MIN_CLK_RATE 0
#define HY8413_MAX_CLK_RATE 15

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
 *  PG1  PG0  Page  Notes
 *   0   0    0     Normal VITA4 format for ID PROM (default)
 *   0   1    1     Calibration values for ADC Channel 0-5
 *   1   0    2     Calibration values for ADC Channel 6-11
 *   1   1    3     Calibration values for ADC Channel 12-16
 */
#define HY8413_ACR_MASK  0x003F  /* Register Mask */
#define ID_PROM_MIN_PAGE  0
#define ID_PROM_MAX_PAGE  3

/* 
 * Sets the range of the ADCs such that
 * 0 =  +/-10V
 * 1 =  +-5
 */
#define HY8413_ASCR_RGE  0x0001

/* 
 * Operation state
 * 0 = Normal operation
 * 1 = Reset ADC
 */
#define HY8413_ACR_RST   0x0002

/*
 * Operation Mode
 * 0 = Standby Mode
 * 1 = Normal Operation
 */
#define HY8413_ACR_NS    0x0004

/* 
 * ADC data format
 * 0 = two's complement (default).
 * 1 = ADC values 0 (Neg FS)-FFFF(Pos FS) 
 */   
#define HY8413_ACR_2C    0x0008 
		    
/* 
 * Bit 0 of ID PROM Paging
 */ 
#define HY8413_ACR_PG0   0x0010

/*
 * Bit 1 of ID PROM Paging
 */ 
#define HY8413_ACR_PG1   0x0020

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
typedef struct hy8413_data_s 
{
   short adc_a[HY8413_NUM_CHAN];
   short ref_0_volt;
   short ref_2_5_volt;
}hy8413_data_ts;

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
     * Read the pre-trigger FIFO memory (16 conversions).
     * Reset Full and set FE when the FIFO is emptied.
     * address: base +0xa
     */
    unsigned short pre_trig;
    /* 
     * Read the post-trigger FIFO memory (256K conversions).
     * Reset TF and TFE when FIFO is emptied.
     * address: base +0xc
     */
    unsigned short post_trig;  
    
     /*
      * Thisis the FIFO fullness counter. Two up/down counters which
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
    * ADC channel data and reference voltage (R)
    * address: base +0x10 to base+0x32 
    */
   hy8413_data_ts data_s; 

   /*
    * Auxilary Control Register (ACR).
    * This register is used to page the IP PROM.
    * address: base +0x10 to base+0x34
    */ 
   unsigned short acr;
   
} hy8413_io_ts;

typedef union 
{
  short           _a[HY8413_NUM_CHAN+2];
  hy8413_data_ts  _s;
}hy8413_io_tu;

typedef struct hy8413_io_s  * HY8413_IO;

/************************************************************

                    Module Configuration Information
                    (see: EPICS driver support)

*************************************************************/
/*
 *  There are trhe types of operating modes:
 *  1. DC sampling mode - when the unit is armed or receiving an external 10MHz
 *     signal, the inputs are sampled at the programmed clock rate and conversions
 *     placed in a 16 conversion pre-trigger FIFO.
 *  2. Register mode - the last ADC reading may be read at random from each addressed
 *     ADC register.
 *  3. Triggered sampling - when the board is triggered conversion are stored in a 
 *     large 156K conversion post-trigger FIFO. Interrupt requests is generated when 
 *     the FIFO is full. The FIFO may be readout as it is filling. The pre-trigger
 *     FIFO remains unchanged. A record of the sample number when the trigger occured
 *     is stored and can be read.
 */
typedef enum 
{
  dcMode=0,
  regMode=1,
  TrigMode=2
} hy8413_mode_te;

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
typedef enum 
{
  noCalb=0,
  Calb=2
} hy8413_calb_te;

#ifdef __cplusplus
}
#endif /* __cplusplus  */

#endif /* DRVHY8413_H  */
