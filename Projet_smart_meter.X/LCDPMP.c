#include "FreeRTOS.h"
#include "LCDPMP.h"
#include "queue.h"
#include "adc_1.h"

extern QueueHandle_t xLCDQueue;


void vLCD(void *p)
{
 xMessage d;
    while(1){
     xQueueReceive(xLCDQueue,&d,portMAX_DELAY);//recevoir la structure qui regroupe les variables Ieff,Veff,P,W,FP.
    LCDInit();//Initialize LCD module
    LCDLHome(1); //aller au debut de la ligne 1
    DisplayMSG(d.s0); //afficher le variable Ieff.
    LCDLHome(2); //aller au debut de la ligne 2
    DisplayMSG(d.s1); //afficher le variable Veff.
    LCDLHome(3); //aller au debut de la ligne 3
    DisplayMSG(d.s2); //afficher le variable P.
    LCDLHome(4); //aller au debut de la ligne 4
    DisplayMSG(d.s3); //afficher le variable W.
    LCDLC(4,12); //aller a la colonne 12 de la ligne 4
    DisplayMSG(d.s4);//afficher le variable FP.
   

}
}


/*************************************************************************************************
*Function : pmp_Init
*Function initializes PMP module for data transfer to LCD mdule
* Parameter passed: None
* Return value: None
* Affects registers and port pins associated with PMP module
**************************************************************************************************/

void pmp_Init(void)
{   //config et initialisation du Parallel Master Port
    UINT mode, controle,port,addrs,interrup;
    //reinitialiser la config précédente éventuelle
    PMPClose();        //disable PMP if enabled previously

//********************* PMP configuration ***********************************************************
/********************* configuration settings ********************************************************
*
*        PMP module enabled
*        Configure RD, RD/WR strobe = ON; WR, WR/ENB strobe = ON
*        Configure Write/ENB and Read/RW polarity = active high
*        Configure MASTER mode 1
*        Configure 4 Tcy WAIT for Data Setup to Read/Write Wait State
*        Configure 15 Tcy WAIT for Read to Byte Enable Strobe Wait State
*        Configure 4 Tcy WAIT for Data Hold After Strobe Wait State
**************************************************************************************************/
    controle = BIT_PMP_ON | BIT_RD_WR_ON | BIT_RD_WR_HI ;
    mode = BIT_MODE_MASTER_1 | BIT_WAITB_4_TCY  | BIT_WAITM_15_TCY | BIT_WAITE_4_TCY ;
    port = BIT_P0;
    addrs = 0x0000;
    interrup = 0x0000;
    PMPOpen(controle,mode,port,addrs,interrup); //Configure PMP and enable it

}

//********************** LCD initialization ************************************************************
void LCDInit(void)
{

    pmp_Init();              //Initialize PMP

//wait for typically > 30ms
    _uLCDloops = LCD_STARTUP;
    Wait(_uLCDloops);

//waitt for typically > 43us
    _uLCDloops = LCD_F_INSTR;
    PMDIN1 = 0b00111000;     // Set the default function
    Wait(_uLCDloops);

//wait for typically > 30ms
    _uLCDloops = LCD_STARTUP;
    Wait(_uLCDloops);
//waitfor typically > 43us
    _uLCDloops = LCD_F_INSTR;
    PMDIN1 = 0b00001100;
    Wait(_uLCDloops);
//wait for typically > 30ms
    _uLCDloops = LCD_STARTUP;
    Wait(_uLCDloops);
//wait for typically > 1.35ms
    _uLCDloops = LCD_S_INSTR;
    PMDIN1 = 0b00000001;        // Clear the display
    Wait(_uLCDloops);
//wait for typically > 30ms
    _uLCDloops = LCD_STARTUP;
    Wait(_uLCDloops);

    _uLCDloops = LCD_S_INSTR;
    PMDIN1 = 0b00000110;        // Set the entry mode

    Wait(_uLCDloops);

    LCDClear();
    LCDHome();
}

//pour les codes des instructions ainsi que les temps d'execution voir datasheet du LCD
//sur https://www.sparkfun.com/datasheets/LCD/HD44780.pdf

//********** Implémentation de quelques fonctions de base du LCD commandé via le PMP

void LCDHome(void)
{//Remettre le curseur au debut le pointeur d'adresse interne à la première ligne première colonne
    _uLCDloops = LCD_S_INSTR; //temps d'attente d'exécution d'instruction
//    PMADDR = 0x0000;
    PMPSetAddress(0x0000); //Instruction --> ligne RS=0 Registre selection : Instruction registre
    PMDIN1 = 0b00000010; //Code de l'instruction
    Wait(_uLCDloops) ;  //attente d'execution d'instruction interne
    
}


void LCDLHome(int Line)
{   //Aller au debut de la ligne Line : 1, 2 3 ou 4 pour un LCD 4 lignes
    _uLCDloops = LCD_S_INSTR;
  
//    PMADDR = 0x0000;
    PMPSetAddress(0x0000); //selection du registre d'instruction
    /*Selon les propriétés du LCD dans Isis les adresses de debut des lignes sont
    {ROW1=80-93}
    {ROW2=C0-D3}
    {ROW3=94-A7}
    {ROW4=D4-E7}*/
    switch(Line){
        case 1: PMDIN1 =0x80; break; //envoyer l'adresse 0x80 au LCD
        case 2: PMDIN1 =0xC0; break;
        case 3: PMDIN1 =0x94; break;
        case 4: PMDIN1 =0xD4; break;
    }
    Wait(_uLCDloops) ; 
}
void LCDLC(int Line, int col)
{   //Aller au debut de la ligne Line : 1, 2 3 ou 4 pour un LCD 4 lignes
    _uLCDloops = LCD_S_INSTR;
  
//    PMADDR = 0x0000;
    PMPSetAddress(0x0000); //selection du registre d'instruction
    /*Selon les propriétés du LCD dans Isis les adresses de debut des lignes sont
    {ROW1=80-93}
    {ROW2=C0-D3}
    {ROW3=94-A7}
    {ROW4=D4-E7}*/
    if((col<20)&&(col>=0)){
    switch(Line){
        case 1: PMDIN1 =0x80+col; break; //envoyer l'adresse 0x80 au LCD
        case 2: PMDIN1 =0xC0+col; break;
        case 3: PMDIN1 =0x94+col; break;
        case 4: PMDIN1 =0xD4+col; break;
    }
    }
    Wait(_uLCDloops) ; 
}




void LCDClear(void)
{  //Effacer tout le LCD
    _uLCDloops = LCD_S_INSTR;
    //PMADDR = 0x0000;
    PMPSetAddress(0x0000);
    PMDIN1 = 0b00000001;
    Wait(_uLCDloops);
}

void LCDPut(char A)
{   //afficher le caractère A a la position courante
    _uLCDloops = LCD_F_INSTR;
//    PMADDR = 0x0001;
    PMPSetAddress(0x0001);
    PMDIN1 = A;
    Wait(_uLCDloops);
    Nop();
    Nop();
    Nop();
    Nop();
}



void Wait(unsigned  int B)
{
    while(B)
    B--;
}


void DisplayMSG(unsigned char *array)
{ //afficher une chaine à partir de lposition courante
 
     while (*array)  // Continue display characters from STRING untill NULL character appears.
     {
      LCDPut(*array++); // Display selected character from the STRING.
     }
}

