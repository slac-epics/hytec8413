/*
=============================================================
 
  Abs:  Driver Support for a VME Hytec ip-adc module
 
  Name: hytecIpm.c
             hytec_ipmAdd        - Add ip module to a linked list
	   * hytec_ipmCreate     - Allocate memory and add IPAC module to the list. 
             hytec_ipmGetFirst   - Get ptr to the first card in the list
	     hytec_ipmGetByLoc   - Get ptr to card info by carrier and slot
	     hytec_ipmGetByName  - Get ptr to card info by name
           * hytec_ipmInit       - Initialize card configuation structure
             hytec_ipmInitDev    - Initialize device structure 
	   * hytec_analyzeINP    - Analyze input string.
             hytec_ipmIsr        - Interrupt handler
             hytec_ipmReport     - Display card linked list information (output to stdio)
	   * hytec_ipmValidate   - Validate IPAC module model at the given carrier & slot 
             hytec_ipmCalEnb     - Enable/Disable calibration

          * indicates static routines
  

  Proto: hytecIpm.h
 
  NOTE: Copied functions getByName,getByLocation from drvIP231.c
        and initDevice from devAoIP231.c written by Sheng Peng
 
  Auth: 27-Aug-2006, K. Luchini,S.Peng (LUCHINI/PENGS)
  Rev : dd-mmm-yyyy, Reviewer's Name   (USERNAME)
 
-------------------------------------------------------------
  Mod:
        05-Dec-2007, K. Luchini        (LUCHINI):
           added TYPE_MBBO to hytec_ipmInitDev
 
=============================================================
*/ 
/* Header Files */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "epicsVersion.h"
#include "epicsMutex.h"
#include "epicsString.h"
#include "errlog.h"
#include "cantProceed.h"
#include "ellLib.h"
#include "dbScan.h"
#include "drvIpac.h"
#include "hytecIpm.h"
#include "hytecIpmLib.h"     /* for hytec_ipmGetByName(),etc proto  */
#include "drvHy8413Lib.h"    /* for drvHy8413_int() prototype       */


/* Local variables */
static ELLLIST cardList_s = {{ NULL,NULL },0};

/* Local Error Messages */
static const char *cardErr_c     ="Card name was not specified!\n";
static const char *invModelErr_c ="Card %hd slot %hd does not have a Hytec model IP-%x\n";
static const char *noModErr_c    ="Card %hd slot %hd has no module\n";
static const char *badIdErr_c    ="Card %hd slot %hd has an invalid IPAC Identifier of %.4s\n";
static const char *addrErr_c     ="Bad carrier %hd or bad slot %hd\n";
static const char *dupNameErr_c  ="Hytec module name %s already exists!\n";
static const char *dupCardErr_c  ="Carrier %hd slot %hd already exists!\n";
static const char *memErr_c      ="Memory allocation error\n";
static const char *initErr_c     ="Card %hd slot %hd module initialization failed!\n";
static const char *inpErr_c      ="Record %s INP/OUT field is empty!\n";
static const char *illInpErr_c   ="Record %s INP/OUT format is illegal - %s!\n";
static const char *regErr_c      ="Record %s Hytec IP-ADC card %s is not registered!\n";
static const char *rngErr_c      ="Record %s channel number %hd is out of range!\n"; 
static const char *bitRngErr_c   ="Record %s bit number %hd is out of range!\n";
static const char *offErr_c      ="Record %s word offset %hd is out of range!\n";
static const char *InvTypeErr_c  ="Record %s record type is not supported!\n";


/* Globals */
extern int debugHy8413;

/* Local prototypes */
static long hytec_ipmInit( char const * const  name_c,
                           unsigned short      carrier,
                           unsigned short      slot, 
                           unsigned long       mask,
                           unsigned char       vector,
                           IPADC_ID  const     config_ps );
static long hytec_ipmValidate( unsigned short carrier,
                               unsigned short slot, 
                               unsigned short model );
static long hytec_ipmCreate( char const * const name_c, 
                             unsigned short     carrier,
                             unsigned short     slot,
                             unsigned short     model,
                             unsigned long      mask,
                             unsigned char      vector );
static long hytec_analyzeINP( char const      * const name_c,
                              char const      * const string_c,
                              IPADC_ID        * const card_pps,
                              short           * const chan_p,
                              short           * const reg_type );

   
/*===================================================
 
  Abs:  Add the ipac module a card configuration
 
  Name: hytec_ipmAdd
 
  Args: name_c                          Card description 
          Type: ascii-string            Note: must be NULL 
          Use:  char *                  terminated.
          Acc:  read-only               
          Mech: By reference            
 
        carrier                         IP Carrier Card Number 
          Type: integer                 Note: 0...196
          Use:  int
          Acc:  read-only
          Mech: By value

        slot                            IP Carrier Port Number  
          Type: integer                 Note: 0=port A
          Use:  int                           1=port B
          Acc:  read-only                     2=port C
          Mech: By value                      4=port D  
           
        model                          Hytec model Id found in ID PROM
          Type: integer                
          Use:  unsigned short                          
          Acc:  read-only                   
          Mech: By value 

        mask                            Bit mask specific to ip model
          Type: integer                 Note: see card specific driver.
          Use:  int                          
          Acc:  read-only                   
          Mech: By value           
 
  Rem: This function determines if this card has already been
       added to the card list. If it has not, then memory is
       allocated to store the card information in a structure.
       This structure is then added to the card linked list.
 
  Side: This is function is called prior to iocInit() and
        after the ip carrier initialization function.
 
  Ret:  long
             OK    - Successful operation
             ERROR - Failure, due to invalid card/chan
 
=======================================================*/
long hytec_ipmAdd(char const * const name_c, 
                  unsigned short     carrier,
                  unsigned short     slot,
                  unsigned short     model,
                  unsigned long      mask,
                  unsigned char      vector )
{
    long  status;

   /*
    * Validate that the module exists 
    * and it's the correct model expected.
    */
    status = hytec_ipmValidate(carrier,slot,model);
    if ( status!=OK ) return(status);

    /* Is the card name valid? */
    status = ERROR;
    if( (!name_c) || (strlen(name_c)==0) )
    {
      errlogPrintf ( cardErr_c );
    } 
    /* Verify that module is not already registered in the linked list, by name. */
    else if( hytec_ipmGetByName(name_c) )
    {
        errlogPrintf ( dupNameErr_c, name_c );
    }
    /* Verify that module is not already registered in the linked list, by carrier and slot */
    else if ( hytec_ipmGetByLoc(carrier,slot) )
    {
        errlogPrintf ( dupCardErr_c,carrier, slot );  
    }
    else
    {
        status = hytec_ipmCreate( name_c,carrier,slot,model,mask,vector );
    }
    return( status );
}

/*====================================================
 
  Abs:  Validate a particular IPAC module type at 
        the given carrier & slot number.
 
  Name:  hytec_ipmValidate
 
  Args: carrier                         IP Carrier Card Number 
          Type: integer                 Note: 0...196
          Use:  int
          Acc:  read-only
          Mech: By value

        slot                            IP Carrier Port Number  
          Type: integer                 Note: 0=port A
          Use:  int                           1=port B
          Acc:  read-only                     2=port C
          Mech: By value                      4=port D  

  Rem:
    Uses ipmCheck to ensure the carrier and slot numbers are legal, probe the
    IDprom and check that the IDprom looks like an IPAC module.  Calculates
    the CRC for the ID Prom, and compares the manufacturer and model ID values
    in the Prom to the ones given.
  
  Side: This function is a mirror of the drvIpac.c function
        ipac_ipmValidate() with the exception that 
           1) CRC is not checked becasue this register
              is not setup on the module.
           2) The IPAC ID is "VITA" and not "IPAC" 
           3) The full model ID is check rather than only the 
              lower byte
           4) The manufacturer's ID is not checked because
              this register is setup on the module.

  Ret: long 
            OK                - Successful Operation 
             S_IPAC_badAddress - Failure, bad carrier or slot number
             S_IPAC_noModule   - Failure, no module installed
             S_IPAC_noIpacId   - Failure, invalid IPAC Indentifier
             S_IPAC_badModule  - Failure, unexpected model ID

=======================================================*/
static long hytec_ipmValidate( unsigned short carrier,
                               unsigned short slot, 
                               unsigned short model )
{    
    long           status=OK;
    ipac_idProm_t *id_ps = NULL;

    /* 
     * Verify that the module on this carrier is the right model.
     * Please note that this function checks that the "IPAC" identifier
     * exists. If it does not then you get the return error code
     * S_IPAC_noIpacId. The Hytec ip-adc-8413 and the ip-adc-8402
     * modules does NOT have this particular identifier. These modules
     * instead use "VITA" as the identifier. So wethis validation
     * test should pass acceptance at thispoint if we get this particular
     * error code because the "VITA" identifier is verified in the 
     * module specific init later.
     */
    status = ipmCheck(carrier,slot);
    if ( status && (status!=S_IPAC_noIpacId) ) 
    {
       if (status==S_IPAC_noModule)
       {
         errlogPrintf ( noModErr_c,carrier,slot );
       }
       else
       {
         errlogPrintf ( addrErr_c,carrier,slot );
       }
    }
    else 
    {
       status = OK;
       id_ps = (ipac_idProm_t *) ipmBaseAddr(carrier, slot, ipac_addrID);
       if ( strncmp((char *)id_ps,"VITA",sizeof(long)) )
       {
          errlogPrintf ( badIdErr_c,carrier,slot, id_ps );  
          status = S_IPAC_noIpacId;
       }
       else if ( id_ps->modelId != model )
       {
          errlogPrintf ( invModelErr_c,carrier, slot, model );
          status = S_IPAC_badModule;
       }
    }
    return( status );
}

/*====================================================
 
  Abs:  Allocate memory and add the ipac module 
        to the card list.
 
  Name: hytec_ipmCreate
 
  Args: name_c                          Card description 
          Type: ascii-string            Note: must be NULL 
          Use:  char *                  terminated.
          Acc:  read-only               
          Mech: By reference            
 
        carrier                         IP Carrier Card Number 
          Type: integer                 Note: 0...196
          Use:  int
          Acc:  read-only
          Mech: By value

        slot                            IP Carrier Port Number  
          Type: integer                 Note: 0=port A
          Use:  int                           1=port B
          Acc:  read-only                     2=port C
          Mech: By value                      4=port D  
           
        model                          Hytec model Id found in ID PROM
          Type: integer                
          Use:  unsigned short                          
          Acc:  read-only                   
          Mech: By value 
     
        mask                           Bit mask specific to ip model
          Type: integer                Note: see card specific driver.
          Use:  int                          
          Acc:  read-only                   
          Mech: By value  

        vector                         Vector (0-255)
          Type: integer  
          Use:  unsigned char                          
          Acc:  read-only                   
          Mech: By value    
 
  Rem: This function determines if this card has already been
       added to the card list. If it has not, then memory is
       allocated to store the card information in a structure.
       This structure is then added to the card linked list.
 
  Side: This is function is after the IPAC module has 
        been found to exists and the model is correct. 
 
  Ret:  long
          OK    - Successful operation
          ERROR - Failure, due to invalid card/chan
 
=======================================================*/
static long hytec_ipmCreate( char const * const name_c, 
                             unsigned short     carrier,
                             unsigned short     slot,
                             unsigned short     model,
                             unsigned long      mask,
                             unsigned char      vector )
{
    long           status=ERROR;
    size_t         bcnt = sizeof(hytec_ipmConfig_ts);
    IPADC_ID       card_ps=NULL;
 
    /* Allocate memory for card information */
    card_ps = callocMustSucceed(1,bcnt,"hytec_addIpAdc()");
    if ( !card_ps ) 
    {
       errlogPrintf ( memErr_c );  
    }
    else
    {  
       status = hytec_ipmInit( name_c, carrier, slot, mask, vector, card_ps );  
       if (status==OK)
       {
	  /* add card to linked list */
          ellAdd( &cardList_s,(ELLNODE *)card_ps); 
       }
       else
       { 
         /* release memory before exiting */
         errlogPrintf ( initErr_c,carrier,slot  ); 
	 if ( card_ps->name_c ) free(card_ps->name_c);
            free( card_ps );
       }
    }
    return( status );
}

/*====================================================
 
  Abs:  Initialize a card configuration structure
 
  Name: hytec_ipmInit
 
  Args: name_c                          Card description 
          Type: ascii-string            Note: must be NULL 
          Use:  char *                  terminated.
          Acc:  read-only               
          Mech: By reference            
 
        carrier                         IP Carrier Card Number 
          Type: integer                 Note: 0...196
          Use:  int
          Acc:  read-only
          Mech: By value

        slot                            IP Carrier Port Number  
          Type: integer                 Note: 0=port A
          Use:  int                           1=port B
          Acc:  read-only                     2=port C
          Mech: By value                      4=port D           

        mask                            Bit mask specific to ip model
          Type: integer                 Note: see card specific driver.
          Use:  int                          
          Acc:  read-only                   
          Mech: By value 

        vector                         Vector (0-255)
          Type: integer  
          Use:  unsigned char                          
          Acc:  read-only                   
          Mech: By value    
  
        card_ps                         Card informational structure
          Type: struct               .
          Use:  hytec_adc_config_ts *                       
          Acc:  read-write access                   
          Mech: By reference 

  Rem: This function initialized the card configuration
       structure. The card information has already been verified
       prior to calling this function, to determine that this module
       is not already in the linked list.
 
  Side: None
 
  Ret:  long
             OK - Successful operation 
 
=======================================================*/  
static long hytec_ipmInit( char const * const  name_c,
                           unsigned short      carrier,
                           unsigned short      slot, 
                           unsigned long       mask,
                           unsigned char       vector,
                           IPADC_ID const      card_ps )
{
  long     status = OK;
  unsigned short i = 0;


  card_ps->carrier = carrier;
  card_ps->slot    = slot;
  card_ps->name_c  = epicsStrDup(name_c);
  card_ps->intVec  = vector;

 /* 
  * Get the base address of the id, 
  * io registers and memory address if available
  */
  card_ps->id_pu = (hytec_ipac_idProm_tu *)ipmBaseAddr(carrier, slot, ipac_addrID);
  card_ps->io_p  = ipmBaseAddr(carrier, slot, ipac_addrIO); 
  card_ps->mem_p = (unsigned short *)ipmBaseAddr(carrier, slot, ipac_addrMem);

  /* Get model number of module */
  card_ps->model = card_ps->id_pu->hytec_s.modelId;

  /* Get serial number of module */
  card_ps->serialNo = card_ps->id_pu->hytec_s.serialNo;

  /* Get firmware revision */
  card_ps->rev = card_ps->id_pu->hytec_s.revision;

  /* Perform special card initialization based on model */
  switch ( card_ps->model ) 
  {
      case HYTEC_IP8413_MODEL:
       status = drvHy8413_init( (void *)card_ps,mask );
       break;

      default: /* Not supported yet */
       status = ERROR;
       break;
  }/* End of switch statement */

  /* Setup some device support initialization for binary inputs */
  if (status==OK ) 
  {
    card_ps->lock  = epicsMutexMustCreate();
    scanIoInit(&card_ps->fifo_s.ioscanpvt);
    for (i=0; i<MAX_BITS; i++)
    {
      scanIoInit( &card_ps->mbbiScan_a[i] );
      scanIoInit( &card_ps->biScan_a[ReadACR][i] );
      scanIoInit( &card_ps->biScan_a[ReadCSR][i] );
    }
  }
 
  return( status );
} 
         

/*====================================================
 
  Abs:  Get the first item in the linked list
 
  Name: hytec_ipmGetFirst
 
  Args: None

  Rem: This function retrieves the pointer to the first card 
       in the linked list. If the list ie empty a NULL will
       be returned.
 
  Side: None
 
  Ret: void *
            NULL     - list empty
            Otherwise, pointer to the first card in list
 
=======================================================*/
void * hytec_ipmGetFirst( void )
{
  IPADC_ID card_ps = NULL;
  card_ps = (IPADC_ID)ellFirst((ELLLIST *)&cardList_s);
  return((void *)card_ps);
}


/*====================================================
 
  Abs:  Find a card configuration by name
 
  Name: hytec_ipmGetByName
 
  Args: name_c                          Card description
          Type: ascii-string            Note: must be NULL
          Use:  char const * const      terminated.
          Acc:  read-only             
          Mech: By reference           
 
 
  Rem: This function retrieves the pointer to the card information
       based on the name that was used by hytec_addIpAdc() which 
       is called prior to iocInit().
 
  Side: None
 
  Ret: void *
            NULL     - Failure, card not found
            Otherwise, pointer of card information

=======================================================*/
void * hytec_ipmGetByName(char const   * const  name_c)
{
    IPADC_ID card_ps  = NULL;
    IPADC_ID found_ps = NULL;
    
    for( card_ps = hytec_ipmGetFirst();
         card_ps && !found_ps; 
         card_ps = (IPADC_ID)ellNext((ELLNODE *)card_ps) )
    {
        if ( strcmp(name_c,card_ps->name_c)==0 ) 
	  found_ps = card_ps;
    }/* End of FOR loop */
    return( (void *)found_ps );
}

/*====================================================
 
  Abs:  Find a card configuration by carrier and slot
 
  Name: hytec_ipmGetByLoc
 
  Args: carrier                         IP Carrier Card Number 
          Type: integer                Note: 0...196
          Use:  int
          Acc:  read-only
          Mech: By value

        slot                            IP Carrier Port Number  
          Type: integer                 Note: 0=port A
          Use:  int                           1=port B
          Acc:  read-only                     2=port C
          Mech: By value                      4=port D   
 
  Rem: This function retrieves the pointer to the card information
       based on the carrier index and the slot or port number.
 
  Side: None
 
  Ret:  void *
            NULL     - Failure, card not found
            Otherwise, pointer of card information
 
=======================================================*/
void * hytec_ipmGetByLoc(unsigned short   carrier,
                         unsigned short   slot )
{
   IPADC_ID  card_ps = NULL;
   int       found   = 0;


   for(card_ps = (IPADC_ID)hytec_ipmGetFirst();
       card_ps && !found; 
       card_ps = (IPADC_ID)ellNext((ELLNODE *)card_ps) )
   {
       if ((carrier == card_ps->carrier) && (slot == card_ps->slot))
	 found = 1;
   }
   return( (void *)card_ps );
}


/*=============================================================

  Abs:  Initialize device support informational structure

  Name: hytec_ipmInitDev

  Args: rec_name_c                 Record name
          Use:  ascii-string       Note: NULL terminated
          Type: char * 
          Acc:  read-only access
          Mech: By reference

         rec_type                   Type of record 
          Use:  integer
          Type: unsigned short
          Acc:  read-only access
          Mech: By value

         nelm                       Number of items in list 
          Use:  integer
          Type: unsigned short
          Acc:  read-only access
          Mech: By value

        string_c                    INP/OUT field
          Use:  ascii-string
          Type: char * 
          Acc:  read-only access
          Mech: By reference

  Rem: This routine verifies that the device information supplied
       for this record is valid, and then allocates memory for the
       private device information, and initializes this structure.

  Side: This function is called by EPICS device support

  Ret: void *
         NULL - Failed operation
         Otherwise - Address of private device info

=============================================================*/
void * hytec_ipmInitDev( char const *  const          rec_name_c,
                         unsigned short               rec_type,
                         unsigned short               nelm,
                         char const *  const          string_c )
{
   long                 status     = OK;
   short                reg_type   = 0;
   short                chan       = 0;
   short                offset,bitNo;
   size_t               bcnt = sizeof(hytec_devicePvt_ts);
   DPVT_ID              devPvt_ps  = NULL;
   IPADC_ID             card_ps    = NULL;


    /* Analyze the record INP/OUP field */
    status = hytec_analyzeINP( rec_name_c, string_c, &card_ps, &chan, &reg_type );
    if ( status!=OK )  return( (void *)devPvt_ps );
    
    switch( rec_type ) 
    { 
         case TYPE_WF:
         case TYPE_AI:
	   if ((card_ps->nchan<= chan ) || (chan<0))
           {
 	      errlogPrintf(rngErr_c,rec_name_c,chan);
              status = ERROR;
	   } 
           break;

         case TYPE_MBBO:
         case TYPE_MBBI:
           offset = chan;
           if ((reg_type==ReadCAL) && (offset>=IPAC_ID_SPACE_WCNT))
	    {
              errlogPrintf(offErr_c,rec_name_c,offset);
              status = ERROR;
	      break;
	    }
	  
         case TYPE_BO:
         case TYPE_BI:
           bitNo = chan;
	   if ( (bitNo >= MAX_BITS) || (bitNo < 0) )
	   {
	     errlogPrintf(bitRngErr_c,rec_name_c,bitNo);
             status = ERROR;
	   }
           break;

         case TYPE_LI:
           offset = chan;
	   if ( offset>=IPAC_ID_SPACE_WCNT )
	   {
              status = ERROR;
              errlogPrintf(offErr_c,rec_name_c,offset);
	   }
           break;

        default:
	    errlogPrintf(InvTypeErr_c,rec_name_c);
            status = ERROR;
            break;
    } /* End of switch statement */      

   /* 
    * If the record type is valid with respect to the INP/OUP field
    * then allocate memory for the private device information that
    * is returned to the device support.
    */
    if (status==OK)
    {
      /*  devPvt_ps = (DPVT_ID)callocMustSucceed(1,bcnt,rec_name_c); */
       devPvt_ps = (DPVT_ID)calloc(1,bcnt);
       if ( devPvt_ps )
       {
	 devPvt_ps->card_ps = card_ps;
         devPvt_ps->i       = chan;
         devPvt_ps->func    = reg_type;
         devPvt_ps->recType = rec_type;
         devPvt_ps->nelm    = nelm;
       }
       else {
         errlogPrintf(memErr_c);
	 status = ERROR;
       }
    }
    return( (void *)devPvt_ps );
}

/*=============================================================

  Abs:  Analyze the INP/OUP field of the Hytec IP-ADC record

  Name: hytec_analyzeINP

  Args: name_c                     Record name
          Use:  ascii-string        Note: NULL terminated
          Type: char const * const
          Acc:  read-only access
          Mech: By reference

        string_c                    INP/OUT field opf record
          Use:  ascii-string        Note: NULL terminated
          Type: char const * const 
          Acc:  read-only access
          Mech: By reference

        card_pps                    Card information information 
          Use:  pointer             
          Type: IPADC_ID *
          Acc:  read-write access
          Mech: By reference

        chan_p                      Channel or bit number 
          Use:  pointer
          Type: short *
          Acc:  read-write access
          Mech: By reference

        reg_type_p                   Register type
          Use:  pointer
          Type: short *
          Acc:  read-write access
          Mech: By reference



  Rem: This routine analyzed the EPICS pv INP/OUP field and
       initializes the private device information structure.

  Side: None

  Ret: long 
         OK    - Successful operation
         ERROR - Failure

=============================================================*/
static long hytec_analyzeINP( char const      * const name_c,
                              char const      * const string_c,
                              IPADC_ID        * const card_pps,
                              short           * const chan_p,
                              short           * const reg_type_p )
{
    long                status  = ERROR;
    unsigned short      i = 0;
    unsigned short      found  = 0;
    char                parm_c[MAX_CA_STRING_SIZE];
    char                card_c[MAX_CA_STRING_SIZE];
    static char        *reg_c[6] = { REG_IO_CSR, 
                                     REG_IO_ACR, 
                                     REG_IO, 
                                     REG_ID, 
                                     REG_SW_CAL, 
                                     REG_IO_DATA };

    /* Initialize return values */
    *card_pps   = NULL;
    *chan_p     = 0;
    *reg_type_p = 0;

    if ( !string_c || !strlen(string_c) ) 
    {
      errlogPrintf( inpErr_c, name_c );
      return(status);
    }

    /* Analyze the record INP/OUP field */
    if ( sscanf(string_c,"%[^:]:%hd:%[^:]",card_c, chan_p, parm_c ) != 3 )
    {
       errlogPrintf( illInpErr_c, name_c, string_c);
       return( status);
    }

    /*
     * Determine if the type of memory is valid for this device.
     * Search the mode list provided as an input argument.
     */
    for (i=0,found=0; (i<REG_TYPE_NUM) && !found; i++)
    {  
       if (strcmp(parm_c,reg_c[i])==0 )
       {
	  found       = 1;
          *reg_type_p = i;
       }
    } /* End of FOR loop */

    if ( !found ) 
       errlogPrintf( illInpErr_c, name_c, string_c);
    else 
    {

      /* Is the card specified online? */
      *card_pps = hytec_ipmGetByName( card_c );
      if( *card_pps == NULL )
      {
        errlogPrintf( regErr_c, name_c, card_c );
        return( status );
      }
      else      
        status = OK;
    }
    return( status );
}


void hytec_ipmIsr( int param )
{
  
  return;
}

/*====================================================
 
  Abs:  Display card linked list information 
 
  Name: hytec_ipmReport
 
  Args: level                        Level of info tobe
          Type: integer              displayed.
          Use:  int
          Acc:  read-only
          Mech: By value

  Rem:  The purpose of this function is to display
        the card list information. The detail of inforamtion 
        is dependent upon the level argument specified.
        The higher the level the more detailed the information
 
        The level of detail is as follows:
          
          Level  Report Informati Displayed
          -----  ---------------------------
            0    list card count
            1    list modules in list, carrier, slot and name
 
  Side: Report is sent to the standard output device
  
  Ret:  None
            
=======================================================*/ 
void hytec_ipmReport( int level )
{
  IPADC_ID card_ps=NULL;
  int      found=0;

  printf("Card List: cnt is %d\n",cardList_s.count );
  if (level==0) return;

  for( card_ps = hytec_ipmGetFirst();
       card_ps && !found; 
       card_ps = (IPADC_ID)ellNext((ELLNODE *)card_ps) )
  {
     printf("IP-ADC-%x:  carrier %hd  slot %hd  name: %s\n",
             card_ps->model,
             card_ps->carrier,
             card_ps->slot,
             card_ps->name_c); 
  }/* End of FOR loop */
  return;
}


/*====================================================
 
  Abs:  Enable/Disable calibration for specified channel
 
  Name: hytec_ipmCalEnb
 
  Args: name_c                       Card name
          Type: char-string
          Use:  char const * const
          Acc:  read-only
          Mech: By reference

         chan                        Channel number
          Type: integer
          Use:  short *
          Acc:  read-only
          Mech: By value

         enb                         Enable flag 
          Type: integer              0=disable, 1=enable
          Use:  unsigned short *
          Acc:  read-only
          Mech: By value

  Rem:  The purpose of this function is to enable
        or disable the use of the calibration data
        by setting a flag in the module configuration
        information.

 
  Side: None
  
  Ret:  long
            OK    - Successful
            ERROR - Failure has occured.
            
=======================================================*/ 
long hytec_ipmCalEnb( char const * const name_c, short chan, unsigned short enb )
{
    long           status  = OK;      /* status return    */
    unsigned short i = 0;             /* channel index    */
    IPADC_ID       card_ps = NULL;    /* card information */

    /* Is the card specified online? */
    card_ps = hytec_ipmGetByName(name_c);
    if ( card_ps )
    {
      /* Are we setting the enable flag for all channels? */
      if (chan==-1) 
      {
        for (i=0; i<card_ps->nchan; i++)
	  card_ps->cal_s.chan_as[i].enb = enb;
      }
      /* Are we setting the enable flag for a specific channel? */
      else if ( (chan>=0) && (chan<card_ps->nchan) )
      {
	if ( enb )
	{
         /* 
	  * Is calibration data available for this module? 
	  * If yes, then eable the use of the calibration data.
	  * Otherwise, return an error.
	  */
	  if ( card_ps->cal_s.enb ) 
	    card_ps->cal_s.chan_as[chan].enb = 1; 
          else
	    status = ERROR;
         }
	else
	    card_ps->cal_s.chan_as[chan].enb = 0;
      }	
      else
      {
         errlogPrintf("IP8413: Invalid channel number %hd specified\n",chan);
         status = ERROR;
      }  
    }
    else
    {
      errlogPrintf("IP8413: Unable to find card %s\n",name_c);
      status = ERROR;
    }
    return(status);
}
