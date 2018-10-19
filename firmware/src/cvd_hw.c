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
#include "cvd_hw.h"
#include "bsp.h"
#include "adc_0.h"

//---------------------------Common dir-----------------------------------------
//---------------------------Project dir----------------------------------------
//---------------------------C library------------------------------------------
//------------------------------------------------------------------------------


#ifdef __CVD_HW__

/**
********************************************************************************
*                                  MACROS
********************************************************************************
*/



/**
********************************************************************************
*                               TYPEDEFS
********************************************************************************
*/


/**
********************************************************************************
*                            FUNCTION DECLARATIONS
********************************************************************************
*/


/**
********************************************************************************
*                              GLOBAL VARIABLES
********************************************************************************
*/


/**
********************************************************************************
*                          FUNCTION IMPLEMENTATIONS
********************************************************************************
*/



/*==========================================================================*/
/* Name   : CvdHwInit                                                       */
/* Brief  : Module initialization                                           */
/* Param  : -                                                               */
/* Return : -                                                               */
/*==========================================================================*/
void CvdHwInit (void) {

}

void CvdSampleInit (void) {

}

void CvdSampleDeInit (void) {

}


void SetVdcChargePinHigh (void) {

}

void DischargeTouchButton (void) {
	uint8_t SFRPAGE_save = SFRPAGE;

	SFRPAGE = 0x00;
	P0MDOUT = P0MDOUT_B0__OPEN_DRAIN | P0MDOUT_B1__PUSH_PULL
			| P0MDOUT_B2__OPEN_DRAIN | P0MDOUT_B3__OPEN_DRAIN
			| P0MDOUT_B4__PUSH_PULL | P0MDOUT_B5__OPEN_DRAIN
			| P0MDOUT_B6__PUSH_PULL | P0MDOUT_B7__PUSH_PULL;
	P0MDIN = P0MDIN_B0__DIGITAL | P0MDIN_B1__DIGITAL | P0MDIN_B2__DIGITAL
			| P0MDIN_B3__DIGITAL | P0MDIN_B4__DIGITAL | P0MDIN_B5__DIGITAL
			| P0MDIN_B6__DIGITAL | P0MDIN_B7__DIGITAL;
	U2F_BUTTON = 0;
	u2f_delay(1);
	SFRPAGE = SFRPAGE_save;
}

void ConnectTouchButtonToAdc (void) {
	uint8_t SFRPAGE_save = SFRPAGE;

	SFRPAGE = 0x00;
	P0MDOUT = P0MDOUT_B0__OPEN_DRAIN | P0MDOUT_B1__OPEN_DRAIN
			| P0MDOUT_B2__OPEN_DRAIN | P0MDOUT_B3__OPEN_DRAIN
			| P0MDOUT_B4__PUSH_PULL | P0MDOUT_B5__OPEN_DRAIN
			| P0MDOUT_B6__PUSH_PULL | P0MDOUT_B7__PUSH_PULL;
	P0MDIN = P0MDIN_B0__DIGITAL | P0MDIN_B1__ANALOG | P0MDIN_B2__DIGITAL
			| P0MDIN_B3__DIGITAL | P0MDIN_B4__DIGITAL | P0MDIN_B5__DIGITAL
			| P0MDIN_B6__DIGITAL | P0MDIN_B7__DIGITAL;
	SFRPAGE = SFRPAGE_save;
}

CVD_SAMPLE_T ChargeAndMeasureAdcCap (void) {
	ADC0_setPositiveInput(ADC0_POSITIVE_INPUT_LDO_OUT); // u_ch = u_ldo = 1.8V
	ADC0_startConversion();                             // Start conversion
	while(!ADC0_isConversionComplete());                // Wait for conversion
	return (CVD_SAMPLE_T)ADC0_getResult();
}

CVD_SAMPLE_T MeasureTouchButton (void) {
	ADC0_setPositiveInput(ADC0_POSITIVE_INPUT_P1);      //
	ADC0_startConversion();                             // Start conversion
	while(!ADC0_isConversionComplete());                // Wait for conversion
	return (CVD_SAMPLE_T)ADC0_getResult();
}




#endif // __CVD__
