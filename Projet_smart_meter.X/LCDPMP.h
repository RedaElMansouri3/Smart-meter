#define USE_AND_OR
/*****************************************************************************/
#include<pmp.h>

//************* Macros pour les délais LCD  ************************
// Ces valeurs sont valables pour Fcy=16MHz (oscillateur FRCPLL FRC avec PLL interne, par exemple  )
// Definit le temps d'execution d'instruction LCD Fast en termes de temps de boucle
// typiquement > 43us
#define    LCD_F_INSTR        130
// Definit le temps d'execution d'instruction LCD Fast en termes de temps de boucle
// typiquement > 1.35ms
#define    LCD_S_INSTR        5000  //150
// Definit le temps d'initialisation du LCD en terme de temps de boucle
// typiquement > 30ms
#define    LCD_STARTUP        65000 //2000
unsigned  int _uLCDloops; //valeur initiale du décompteur de temporisation
//******* Prototype declarations ************
void pmp_Init(void); //initialise le Parallel Master PORT pour controler le LCD
void LCDInit(void);  //initialise le LCD
void LCDHome(void);  //ramène le curseur au début
void LCDLHome(int line); //ramène le curseur au debut de la ligne Line
void LCDLC(int line, int col); //ramène le curseur à la colonne col de la ligne Line
void LCDClear(void); //efface le LCD
void LCDPut(char A); //affiche un caractère à la position courante
void DisplayMSG(unsigned char *array); //affiche une chaine à partir de la position courante
void Wait(unsigned  int B); //temporisation pour l'attente d'execution des instructions en interne du LCD
