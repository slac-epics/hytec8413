/*
=============================================================

  Abs:  Prototype include file for the Hytec Utilities

  Name: hytecIpmLib.h

  Side: None

  Auth: 21-Sep-2006, Kristi Luchini   (LUCHINI)
  Rev : dd-mmm-yyyy, Reviewer's Name  (USERNAME)

-------------------------------------------------------------
  Mod:
        06-Dec-2007, K. Luchini       (LUCHINI):
          add prototype for hytec_ipmGetByLoc()
          add prototype for hytec_ipmGetByName()

=============================================================
*/
#ifndef HYTECIPMLIB_H
#define HYTECIPMLIB_H

/*
 * Add ipac module to linked list of cards. This function
 * must be called before iocInit() and after the ip carrier
 * board initialization routine.
 */
long hytec_ipmAdd(
         char const * const name_c,        /* name to register device with EPICS */
         unsigned short     carrier,       /* carrier card number (0-max)        */
         unsigned short     slot,          /* port number  (0-3)                 */
	 unsigned short     model,         /* hytec model found in ID PROM       */
         unsigned long      mask,          /* mask, bits specific to model       */
         unsigned char      vector         /* vector (0-255)                     */
                 );
/*
 * Initialize the device informational structure. This
 * function is called in device support. If successful a
 * pointer to the new device support structure will be returned.
 * Otherwise, a NULL will be returned.
 */
void * hytec_ipmInitDev( 
	  char const * const           rec_name_c,    /* record name                      */  
          unsigned short               rec_type,       /* type of info, chan or register  */
          unsigned short               nelm,          /* Number of elements               */
          char const * const           string_c       /* INP or OUT field of record       */
                         );

/*
 * Return a pointer to the first node in the card linked list.
 * If the list is empty a NULL will be returned.
 */
void * hytec_ipmGetFirst(void);

/*
 * The purpose of this function is to get the 
 * ptr to the card info by card name.
 */
void * hytec_ipmGetByName( char const * const  name_c );

/*
 * The purpose of this function is to get the 
 * ptr to the card info by carrier and slot
 */
void * hytec_ipmGetByLoc( unsigned short  carrier,  
                          unsigned short  slot );
/*
 * The purpose of this function is to display
 * information regarding the card list list. The
 * detail displayed is dependend on the level
 * supplied. Level 0, produces a display with
 * limited information with increasing levels
 * producing more.
 */
void hytec_ipmReport( int level );

/*
 * The purpose of this function is to 
 * enable or disable the calibration
 * usage flag for the specified module
 * and channel. Note that this flag can
 * only be enabled if calibration data is
 * available for this module.
 */
long hytec_ipmCalEnb( 
	  char const * const name_c, /* card name */
          short              chan,   /* chan num 0-15, -1 for all channels */
          unsigned short     enb     /* enable flag. 0=disable,1=enable    */
                  );

#endif /* HYTECIPMLIB_H */
