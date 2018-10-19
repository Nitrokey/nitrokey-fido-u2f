#ifndef __CVD_HW_H__
#define __CVD_HW_H__

#include "app.h"

#if (CVD_DAC_BIT_NUM > 8)
typedef uint16_t   CVD_SAMPLE_T;
#else
typedef uint8_t    CVD_SAMPLE_T;
#endif // CVD_DAC_BIT_NUM


#ifdef __CVD_HW__
extern void          CvdHwInit                  (void);
extern void          SetVdcChargePinHigh        (void);
extern void          DischargeTouchButton       (void);
extern void          ConnectTouchButtonToAdc    (void);
extern CVD_SAMPLE_T  ChargeAndMeasureAdc        (void);
extern CVD_SAMPLE_T  MeasureTouchButton         (void);
extern void          CvdSampleInit              (void);
extern void          CvdSampleDeInit            (void);

#else
#define CvdHwInit()                    ;
#define SetVdcChargePinHigh()          ;
#define DischargeTouchButton()         ;
#define ConnectTouchButtonToAdc()   ;
#define ChargeAndMeasureAdcCap()       -1
#define MeasureTouchButton()           -1
#define CvdSampleInit()                ;
#define CvdSampleDeInit()              ;
#endif


#endif

