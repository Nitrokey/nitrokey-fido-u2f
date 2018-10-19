#ifndef __CVD_H__
#define __CVD_H__



#ifdef __CVD__
extern void         CvdInit    (void);
extern CVD_SAMPLE_T CvdSample  (void);
#else
#define  CvdInit ()    ;
#define  CvdSample()   -1
#endif

#endif
