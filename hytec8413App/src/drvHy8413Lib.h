/*
=============================================================

  Abs:  Prototype include file for the Hytec 
        IP-ADC-8413 16-bit Module

  Name: drvHy8413Lib.h

  Side: None

  Auth: 16-Aug-2001, Kristi Luchini   (LUCHINI)
  Rev : dd-mmm-yyyy, Reviewer's Name  (USERNAME)

-------------------------------------------------------------
  Mod:
        05-Dec-2007, K. Luchini       (LUCHINI):
         add hy8413_wt_clk_rate() prototype
         add hy8413_rd_clk_rate() prototype
         add hy8413_init_sam_mode() prototype

=============================================================
*/
#ifndef DRVHY8413LIB_H
#define DRVHY8413LIB_H

/* Interrupt Service Routine */ 
void drvHy8413_isr( int param );                    /* module configuration infor     */

/*
 * Write to the control register of specified card 
 */
long drvHy8413_wt_csr(
          volatile unsigned short * const   io_p,   /* io base address                */
          unsigned short                    mask,   /* bit mask for csr               */
          unsigned short                    val     /* state of mask bits to set      */
                    );

/*
 * Write to the auxilary control register of specified card
 */
long drvHy8413_wt_acr(
          volatile unsigned short  * const  io_p,   /* io base address                */
          unsigned short                    mask,   /* bit mask for csr               */
          unsigned short                    val     /* state of mask bits to set      */
                    ); 

/*
 * Set id prom page in auxilliary control register
 */
long drvHy8413_wt_page(
          volatile void            * const  io_p,   /* io base address                */
          unsigned short                    val     /* page number                    */
                      ); 

/*
 * Enable or disable samping adc data at sample clock rate/
 */
long drvHy8413_ARM(
          volatile unsigned short * const   io_p,   /* io base address                */
          unsigned short                    val     /* clock rate (0-15)              */
                      ); 


/*
 * Set the clock rate
 */
long drvHy8413_wt_clk_rate(
          volatile unsigned short  * const  io_p,   /* io base                        */
          unsigned short                    val     /* clock rate (0-15)              */
                      ); 


/*
 * Read the clock rate register. The value in the clock rate register is
 * the return value (0-15).
 */
short drvHy8413_rd_clk_rate(
          volatile unsigned short  * const  io_p    /* io base                        */
                      ); 


/*
 * Initilize the modules (v2 only) to SAM Readout Mode
 */
long drvHy8413_init_sam_mode(
          volatile unsigned short  * const  io_p    /* io base                       */
                      );


/*
 * Read control status register
 */
long drvHy8413_rd_status(
          volatile unsigned short  * const  io_p ,  /* io base address                */
          unsigned short           * const  val_p   /* data read from status reg      */
                    );
/*
 * Read adc buffer of specified card and channel.
 * If the config_p arguement is not supplied then the card and
 * chan arguments are used. Otherwise, the configuration information
 * is used.
 */
long drvHy8413_rd(
          volatile unsigned short  * const  io_p,   /* io base             */
          unsigned short                    chan,   /* channel number      */
          short                    * const  val_p   /* adc data            */
                       );

/*
 * Display adc data to standard output.
 */
void drvHy8413_dump_adc_data( volatile unsigned short const * const io_p );

/*
 * Initialize adc according to bitmask supplied as an 
 * input argument.
 */
long drvHy8413_init(
          void           * const      card_p,    /* card info           */
          unsigned long               mask 
          );

/*
 * Add module to card linked list. This function
 * must be called before iocInit(). Please note
 * that the generic function hytec_ipAddAdc()
 * can be called instead of this function.
 */
long ip8413Create(
          char const * const name_c,             /* name to register device with EPICS */
          unsigned short     carrier,            /* carrier card number (0-max)        */
          unsigned short     slot,               /* port number  (0-3)                 */
          unsigned long      mask,               /* mask, bits specific to model       */
          unsigned char      vector              /* Interrupt vector                   */
	  );
 
/*
 * This function scales the raw adc value using
 * the calibration data.
 */
long  drvHy8413_cal_adc(
         unsigned short const * const gain_a,
         unsigned short               calType,
         unsigned short               format,
         long                         val
          );

#endif /* DRVHY8413LIB_H */
