#define USE_AND_OR
#include <xc.h>
#include "freeRTOS.h"
#include "task.h"
#include "queue.h"
#include <adc.h>  //pour pouvoir utiliser les fonctions de lalib de periph du PIC24 relatives a  ADC
#include <stdio.h>
#include "semphr.h"
typedef struct{
   unsigned char *s0;// courant
   unsigned char *s1;//tension
   unsigned char *s2;//puissance
   unsigned char *s3;//energie
   unsigned char *s4;//facteur de puissance 
    
}xMessage; 
