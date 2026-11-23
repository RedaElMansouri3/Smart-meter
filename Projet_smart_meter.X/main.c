// PIC24FJ128GA010 Configuration Bit Settings

// 'C' source line config statements

// CONFIG2
#pragma config POSCMOD = NONE           // Primary Oscillator Select (Primary oscillator disabled)
#pragma config OSCIOFNC = OFF           // Primary Oscillator Output Function (OSC2/CLKO/RC15 functions as CLKO (FOSC/2))
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor (Clock switching and Fail-Safe Clock Monitor are disabled)
#pragma config FNOSC = FRCPLL           // Oscillator Select (Fast RC Oscillator with PLL module (FRCPLL))
#pragma config IESO = ON                // Internal External Switch Over Mode (IESO mode (Two-Speed Start-up) enabled)

// CONFIG1
#pragma config WDTPS = PS32768          // Watchdog Timer Postscaler (1:32,768)
#pragma config FWPSA = PR128            // WDT Prescaler (Prescaler ratio of 1:128)
#pragma config WINDIS = ON              // Watchdog Timer Window (Standard Watchdog Timer enabled,(Windowed-mode is disabled))
#pragma config FWDTEN = ON              // Watchdog Timer Enable (Watchdog Timer is enabled)
#pragma config ICS = PGx2               // Comm Channel Select (Emulator/debugger uses EMUC2/EMUD2)
#pragma config GWRP = OFF               // General Code Segment Write Protect (Writes to program memory are allowed)
#pragma config GCP = OFF                // General Code Segment Code Protect (Code protection is disabled)
#pragma config JTAGEN = ON              // JTAG Port Enable (JTAG port is enabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include "xc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "list.h"
#include "adc_1.h"
#include <math.h> //pour sqrt
#include <timer.h> //pour utiliser la librairie Microchip pour configurer Timer2
#include "semphr.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>



void configAdc(void);
void configTMR2(void);
void vCapt(void *p);
int xStartLCDTask(void);
void vLCD(void *p);
void pmp_Init(void); //initialise le Parallel Master PORT pour controler le LCD
void LCDInit(void);  //initialise le LCD
void LCDHome(void);  //ramène le curseur au début
void LCDLHome(int line); //ramène le curseur au debut de la ligne Line
void LCDLC(int line, int col); //ramène le curseur à la colonne col de la ligne Line
void LCDClear(void); //efface le LCD
void LCDPut(char A); //affiche un caractère à la position courante
void DisplayMSG(unsigned char *array); //affiche une chaine à partir de la position courante
void Wait(unsigned  int B); //temporisation pour l'attente d'execution des instructions en interne du LCD
unsigned int EEPROM_I2C_Write(int N, unsigned char *pdata, unsigned char deviceadd, unsigned char addH, char addL);
void EEPROM_I2C_Read(int N, unsigned char *pdata, unsigned char deviceadd, unsigned char addH, char addL);
int vI2CEEprom(void *pr);
void vApplicationIdleHook(void); //déclaration avant main( )
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName ); 

int main(int argc, char** argv) {
    
if(xStartLCDTask()==1){//si fille et vLCDTask sont crees demarrer le scheduler 
xTaskCreate(vCapt, "Task1", 800, NULL, 1, NULL );
vTaskStartScheduler();
}else {_TRISA0=0;
while(1) _RA0=1;}

}


void vApplicationIdleHook(void) {
// si on souhaite que la tâche idle execute un code on le met ici
}
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName ){
 //Que faire en cas de débordement de pile ?
}