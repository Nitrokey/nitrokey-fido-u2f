/**=============================================================================
* Module name : cvd.c
* Description : Touch button driver on a Capacitive Voltage Divider basis
* Author      : GA
* Version     : v1.0
*-------------------------------------------------------------------------------
* History    :
*    2018.09.20. v1.0     Module created
*
*=============================================================================*/

//---------------------------Standard includes----------------------------------
//---------------------------Common dir-----------------------------------------
//---------------------------Project dir----------------------------------------
#include "cvd_hw.h"
#include "cvd.h"
//---------------------------C library------------------------------------------
//------------------------------------------------------------------------------


#ifdef __CVD__

/**
********************************************************************************
*                                  MACROS
********************************************************************************
*/


#ifndef CVD_DAC_BIT_NUM
#error  CVD_DAC_BIT_NUM  is not set!
#endif

#define CVD_DAC_MAX_VALUE        ((1 << ((DAC_BIT_NUM) + 1)) - 1)

#ifdef __GNUC__
#define STRUCT_PACK              __attribute__((packed))
#else
#define STRUCT_PACK
#endif

/**
********************************************************************************
*                               TYPEDEFS
********************************************************************************
*/

typedef struct STRUCT_PACK {
   CVD_SAMPLE_T charge;
   CVD_SAMPLE_T touch;
} ADC_DATA;


/**
********************************************************************************
*                            FUNCTION DECLARATIONS
********************************************************************************
*/
CVD_SAMPLE_T CvdSample (void);


/**
********************************************************************************
*                              GLOBAL VARIABLES
********************************************************************************
*/

#ifdef __CVD_TEST__
#define ADC_TEST_BUF_SIZE        32
ADC_DATA AdcData[ADC_TEST_BUF_SIZE] = {0};
static uint8_t AdcDataIdx = 0;
#endif

/**
********************************************************************************
*                          FUNCTION IMPLEMENTATIONS
********************************************************************************
*/



/*==========================================================================*/
/* Name   : CvdInit                                                         */
/* Brief  : Module initialization                                           */
/* Param  : -                                                               */
/* Return : -                                                               */
/*==========================================================================*/
void CvdInit (void) {

}


/*==========================================================================*/
/* Name   : CvdSample                                                       */
/* Brief  :                                            */
/* Param  : -                                                               */
/* Return :                                                                */
/*==========================================================================*/
CVD_SAMPLE_T CvdSample (void) {
   CVD_SAMPLE_T before;
   CVD_SAMPLE_T after;

   CvdSampleInit();

   DischargeTouchButton();
   ConnectTouchButtonToAdc();
   before = ChargeAndMeasureAdcCap();
   #ifdef __CVD_TEST__
   AdcDataIdx %= ADC_TEST_BUF_SIZE;
   AdcData[AdcDataIdx].charge = before;
   #endif
   after = MeasureTouchButton();
   #ifdef __CVD_TEST__
   AdcData[AdcDataIdx].touch = after;
   AdcDataIdx++;
   #endif

   CvdSampleDeInit();

   return before - after;
}



#endif // __CVD__
