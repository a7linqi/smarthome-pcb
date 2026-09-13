#ifndef __LED_H
#define __LED_H
#include "sys.h"

#define  ON        1  	//¿ª
#define  OFF       0  	//¹Ø

#define BEEP         PCout(13)  	//·äÃùÆ÷
#define Water_pump   PBout(14)   //Ë®±Ã

void LED_Init(void);


#endif

