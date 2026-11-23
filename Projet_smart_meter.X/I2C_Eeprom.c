#define USE_AND_OR /* To enable AND_OR mask setting */
#include"freertos.h"
#include "task.h"
#include "adc_1.h"
#include<i2c.h>
extern QueueHandle_t xEEPROMQueue;
extern QueueHandle_t xLCDQueue;
char *strcmp(char *dest, char*src);
//Bibliothèque pour la journalisation et la récupération de données de différents types
//sur une EEprom 24XX512:
//plus d'infos sur le fonctionnement de cette EEprom
// http://ww1.microchip.com/downloads/en/devicedoc/21754m.pdf
//Capacité de la mémoire 64Koctets --> les adresses sont alors sur 16bits (2octets)
//addH et addL
//Cette EEPROM permet d'écrire de manière séquentielle sur une page de 128 octets
//les pages commence à partir des addresses dans le poid faible est multiple de 128
//soit donc addL=00, 128 (0x80))
//arrivé à la fin d'une page il faut l'enregistrer (Stop) et ouvrir la page suivante

#define pagesize 128 //taille d'une page pour cette mémoire

// fonction d'écriture de N octets pointé par pdata, dans la EEPROM d'adresse deviceadd, à partir de l'adresse
// mémoire (addH)(addL)
//adddeviceadd EEPROM_I2C_Write(int N, unsigned char *pdata, unsigned char deviceadd, unsigned char addH, char addL);
unsigned int EEPROM_I2C_Write(int N, unsigned char *pdata, unsigned char deviceadd, unsigned char addH, char addL);
// fonction de lecture de N octets vers l'espace pointé par pdata, à partir de  l'adresse
// mémoire (addH)(addL) l' EEPROM d'adresse deviceadd,

void EEPROM_I2C_Read(int N, unsigned char *pdata, unsigned char deviceadd, unsigned char addH, char addL);
void delay(void);

//Une fonction tâche  pour la journalisation et la récupération des données

int vI2CEEprom(void *pr) {
     xMessage d;
     xMessage copied;
    unsigned char copie0[20];
    unsigned char copie1[20];
    unsigned char copie2[20];
    unsigned char copie3[20];
    unsigned char copie4[20];
    
    unsigned char*p0=&copie0[0];
    unsigned char*p1=&copie1[0]; 
    unsigned char*p2=&copie2[0];
    unsigned char*p3=&copie3[0];
    unsigned char*p4=&copie4[0];
    unsigned char addL, addH;
    unsigned int add;
    unsigned char deviceadd;
    
    //configuration de i2C
    UINT config1 = 0, i = 0;
    UINT config2 = 0;
    /* Turn off I2C modules */
    CloseI2C1(); //Disbale I2C1 mdolue if enabled previously
    //************ I2C interrupt configuration ******************************************************
    ConfigIntI2C1(MI2C_INT_OFF); //Disable I2C interrupt
    //***************** I2C1 configuration **********************************************************
    /**********************************************************************************************
     * voir datasheet pour plus d'info sur chaque option de configuration
     *        I2C1 enabled
     *        stop I2C module in Idle mode
     *        IPMI mode not enabled
     *        I2CADD is 7-bit address
     *        Disable Slew Rate Control 
     *        Enable SM bus specification
     *        Disable General call address
     
     **********************************************************************************************/
      config1 = (I2C_ON | I2C_IDLE_STOP |I2C_7BIT_ADD| I2C_IPMI_DIS| I2C_SLW_DIS |I2C_SM_DIS|I2C_GCALL_DIS );
    /**         Fcy=16MHz and FSCL=400KHz --> Baud = 37 into I2CxBRG voir datasheet page 139*/
    config2 = 37; //datasheet page 139
    OpenI2C1(config1, config2); //configure I2C1
     
 

    addH = 0x00;
    addL = 0x00; 
    deviceadd=0xA0;//deviceaddr=0xA0=b1010000 --> A2=A1=A0=0 par cablage voir design et R_\w=0 --> ecriture
    _TRISD1 = 0;
    while(1){
    xQueueReceive(xEEPROMQueue,&d,portMAX_DELAY);//recevoir la structure qui regroupe les variables Ieff,Veff,P,W et FP
    
   // *************************************journalisation et recuperation des variables******************************************************************************
    
    add=EEPROM_I2C_Write(20, (unsigned char*)(d.s0), deviceadd, addH, addL);//ecrire le premier item dans l'eeprom selectionnee par deviceadd, a l'aide des adresses addH addL et l'obtention du nouveau add.
    EEPROM_I2C_Read(20, p0, deviceadd, addH, addL);//récupération du variable qu'on vient d'ecrire dans copie0.
    
    addH= (add>>8);//obtenir l'octet du poids le plus fort
    addL = (add&0xFF);//obtenir l'octet du poids le plus faible
    add=EEPROM_I2C_Write(20, (unsigned char*)(d.s1), deviceadd, addH, addL);
    EEPROM_I2C_Read(20, p1, deviceadd, addH, addL);
    
    addH= (add>>8);
    addL = (add&0xFF);
    add=EEPROM_I2C_Write(20, (unsigned char*)(d.s2), deviceadd, addH, addL);
    EEPROM_I2C_Read(20, p2, deviceadd, addH, addL);
    
    addH= (add>>8);
    addL = (add&0xFF);
   add=EEPROM_I2C_Write(20, (unsigned char*)(d.s3), deviceadd, addH, addL);
   EEPROM_I2C_Read(20, p3, deviceadd, addH, addL);
    
  addH= (add>>8);
    addL = (add&0xFF);
 add=EEPROM_I2C_Write(20, (unsigned char*)(d.s4), deviceadd, addH, addL);
    EEPROM_I2C_Read(20, p4, deviceadd, addH, addL);
    
 addH= (add>>8);
    addL = (add&0xFF);
    
    
     
     if ((strcmp((char*)(d.s0),(char*)p0) == 0) &&(strcmp((char*)(d.s1),(char*)p1) == 0)&&(strcmp((char*)(d.s2),(char*)p2) == 0)&&(strcmp((char*)(d.s3),(char*)p3) == 0)&&(strcmp((char*)(d.s4),(char*)p4) == 0)) {
         
//             sprintf((char*)p0,"add: %3d  deviceadd: %3d ",add,deviceadd);
//            copied.s0=p0;
//            copied.s1=p1;
//            copied.s2=p2;
//            copied.s3=p3;
//            copied.s4=p4;
//           xQueueSendToBack(xLCDQueue,&copied,portMAX_DELAY);//l'envoi de la sructure copie dans la file qui regroupe les variable recuperees par la fonction read.
         //ci dessus test de vérification par LCD.
        _RD1=~_RD1;//test de vérification réussi.
         }
     
    }}
// fonction d'écriture de N octets pointés par pdata, dans la EEPROM d'adresse deviceadd, à partir de l'adresse
// (addH)(addL)

 unsigned int EEPROM_I2C_Write(int N, unsigned char *pdata, unsigned char deviceadd, unsigned char addH, char addL) {
    
    unsigned int add;
    int j=1;
    IdleI2C1(); //attendre que le bus soit libre
    StartI2C1(); //debuter un échange
    while (I2C1CONbits.SEN); //attendre la condition de début soit terminée
    MI2C1_Clear_Intr_Status_Bit; //effacer le flag d'interruption
    //adresser une mémoire parmi d'autres sur le bus i2C
    MasterWriteI2C1(deviceadd); //envoyer sur le bus i2c l'adresse de l'eeprom
    //Noter qu'on peut avoir plusieurs eeprom sur le même bus
    //l'adresse i2C pour les eeprom 24XX512 possède le format suivant:
    //b1010A2A1A0(R_\W)   A2A1A0 permettent de sélectionner une mémoire parmi d'autre
    //ont donct avoir jusqu'à 8 EEprom sur un bus
    //le bit (R_\W) permet de choisir une lecture ou ecriture
    //pour cette fonction il faut qu'il soit à 0 soit donc une ecriture
    while (I2C1STATbits.TBF); // attendre que les 8bits de l'octet est envoyé et le buffer est vide
    while (!IFS1bits.MI2C1IF); // Wait for 9th clock cycle
    MI2C1_Clear_Intr_Status_Bit; // Clear interrupt flag 
    while (I2C1STATbits.ACKSTAT); //attendre l'accusé de Reception ACK
    //choisir une case mémoire dans l'eeprom déjà ciblé
    MasterWriteI2C1(addH); //envoyer l'octet poid fort de l'adresse eeprom ou écrire/lire
    while (I2C1STATbits.TBF); // 8 clock cycles
    while (!IFS1bits.MI2C1IF); // Wait for 9th clock cycle
    MI2C1_Clear_Intr_Status_Bit; // Clear interrupt flag 
    while (I2C1STATbits.ACKSTAT);
    MasterWriteI2C1(addL); //envoyer l'octet poid fort de l'adresse eeprom ou écrire/lire
    while (I2C1STATbits.TBF); // 8 clock cycles
    while (!IFS1bits.MI2C1IF); // Wait for 9th clock cycle
    MI2C1_Clear_Intr_Status_Bit; // Clear interrupt flag 
    while (I2C1STATbits.ACKSTAT);
    // ecrire les données de manière séquentielle
    //si on arrive à la fin d'une page, il faut envoyer un stop pour l'enregistrer
    //et la page suivante
    while (N--) { //tant qu'il reste des données à écrire décrémenter N
        MasterWriteI2C1(*pdata++); //acrire l'octet pointé par pdata et passer au suivant

        // attendre qu'il soit transmis 
        while (I2C1STATbits.TBF); // 8 clock cycles
        while (!IFS1bits.MI2C1IF); // Wait for 9th clock cycle
        MI2C1_Clear_Intr_Status_Bit; // Clear interrupt flag 
        while (I2C1STATbits.ACKSTAT); //attendre accusé de l'eeprom
        //incrémenter l'octet poids faible d'adresse
        addL++;
        if ((addL % pagesize) == 0) //si on passe la frontière d'une page
        {
            if (addL == 0){addH++;} //si addL a débordé 255-->0 incrémenté addH
            if (addH ==0xFF )//si addH a débordé 255-->0 incrémenté A2A1A0 (passer a l'eeprom suivante)
            {
            if(j<8){
            deviceadd=deviceadd+2;
            j++;
            }
            }
            StopI2C1(); //forcer l'enregistrement de la page qu'on vient de dépasser
            vTaskDelay(6 / portTICK_RATE_MS); //attendre 6ms (typique))
            if (N) { //tant qu'il rese encore des données à écrire
                IdleI2C1(); //démarrer un nouveau échange pour ouvrir une nouvelle page
                StartI2C1();
                while (I2C1CONbits.SEN);
                MI2C1_Clear_Intr_Status_Bit;
                MasterWriteI2C1(deviceadd);
                while (I2C1STATbits.TBF); // 8 clock cycles
                while (!IFS1bits.MI2C1IF); // Wait for 9th clock cycle
                MI2C1_Clear_Intr_Status_Bit; // Clear interrupt flag 
                while (I2C1STATbits.ACKSTAT);
                MasterWriteI2C1(addH);
                while (I2C1STATbits.TBF); // 8 clock cycles
                while (!IFS1bits.MI2C1IF); // Wait for 9th clock cycle
                MI2C1_Clear_Intr_Status_Bit; // Clear interrupt flag 
                while (I2C1STATbits.ACKSTAT);
                MasterWriteI2C1(addL);
                while (I2C1STATbits.TBF); // 8 clock cycles
                while (!IFS1bits.MI2C1IF); // Wait for 9th clock cycle
                MI2C1_Clear_Intr_Status_Bit; // Clear interrupt flag 
                while (I2C1STATbits.ACKSTAT);
                
            }
        }
    }
    //si la page n'est pas enregistrée il faut le faire
    if ((addL % pagesize) != 0) {
        StopI2C1();
        vTaskDelay(6 / portTICK_RATE_MS);
    }
    add=addL+(addH<<8);//regrouper les adresses addH et addL dans add.
    
   return add;
}

// fonction de lecture de N octets vers l'espace pointé par pdata, à partir de  l'adresse
// (addH)(addL) l' EEPROM d'adresse deviceadd,

void EEPROM_I2C_Read(int N, unsigned char *pdata, unsigned char deviceadd, unsigned char addH, char addL) {
    IdleI2C1(); //attendre que le bus soit libre
    StartI2C1(); //dévuter un échange
    while (I2C1CONbits.SEN); //Wait till Start sequence is completed
    MI2C1_Clear_Intr_Status_Bit;
    //sélectionner une mémoire parmis d'autre, pour y ecrire
    MasterWriteI2C1(deviceadd);
    while (I2C1STATbits.TBF); // 8 clock cycles
    while (!IFS1bits.MI2C1IF); // Wait for 9th clock cycle
    MI2C1_Clear_Intr_Status_Bit; // Clear interrupt flag 
    while (I2C1STATbits.ACKSTAT);
    //sélectionner une adresse dans cette eeprom
    MasterWriteI2C1(addH);
    while (I2C1STATbits.TBF); // 8 clock cycles
    while (!IFS1bits.MI2C1IF); // Wait for 9th clock cycle
    MI2C1_Clear_Intr_Status_Bit; // Clear interrupt flag 
    while (I2C1STATbits.ACKSTAT);
    MasterWriteI2C1(addL);
    while (I2C1STATbits.TBF); // 8 clock cycles
    while (!IFS1bits.MI2C1IF); // Wait for 9th clock cycle
    MI2C1_Clear_Intr_Status_Bit; // Clear interrupt flag 
    while (I2C1STATbits.ACKSTAT);
    // lire les données de manière séquentielle
    RestartI2C1(); //relancer l'échange
    while (I2C1CONbits.RSEN); //Wait till Start sequence is completed
    MI2C1_Clear_Intr_Status_Bit;
    //sélectionner une mémoire pour y lire (bit R_\w=1 --> lecture)
    MasterWriteI2C1(deviceadd + 1);
    while (I2C1STATbits.TBF); // 8 clock cycles
    while (!IFS1bits.MI2C1IF); // Wait for 9th clock cycle
    MI2C1_Clear_Intr_Status_Bit; // Clear interrupt flag 
    while (I2C1STATbits.ACKSTAT);
    MastergetsI2C1(N, pdata, 1000);

    StopI2C1();
    //vTaskDelay(6/ portTICK_RATE_MS); 
}
