#define USE_AND_OR /* To enable AND_OR mask setting */
#define USE_AND_OR /* To enable AND_OR mask setting */
#include "xc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "list.h"
#include "adc_1.h"
#include <math.h> //pour sqrt
#include <timer.h> //pour utiliser la librairie Microchip pour configurer Timer2
#include "semphr.h"
#include "queue.h"

//********************** fonctions utilitaires*************************
void vTaskDelay(portTickType xTicktoDelay);
void vLCD(void *p);
int vI2CEEprom(void *pr);

xSemaphoreHandle xSemaphore;
QueueHandle_t xLCDQueue; 
QueueHandle_t xEEPROMQueue;

//configuration pour scanner les deux cannaux AN0 (courant) et AN1 (tension) alternativement
//8conversions successsives pour chaque canal
//autosampling --> le sampling dure 8*TAD
//conversion dure 12*TAD
//TAD=2*Tcy , sachant que Tcy=62,5ns  (1/16MHz puisque l'oscillateur utilisé est FRCPLL)
//NB: pour avoir TAD=2Tcy il faut utiliser ADC_CONV_CLK_3Tcy (qui correspond à 0x0002) et selon le datasheet 
//ceci correspond à TAD=2*Tcy, il y a une erreur dans la librairie adc.h
void configAdc(void) {
    UINT channel, config1, config2, config3, configport, configscan; //registres de config de l'ADC voir datasheet
    CloseADC10(); //turns off the ADC module and disables the ADC interrupts
    //configuration ADC pour:
    // conversions multiples du canal AN0 et AN1 de manière alternée (16 conversions successives)
    // on obtient 8 valeurs pour AN0 et 8 pour AN1
    //VinH et VinL de Simpler Holder S/H seront reliées à AN0/AN1 et Vref-
    //seul multiplexeur A sera utilisé
    channel = ADC_CH0_POS_SAMPLEA_AN0 | ADC_CH0_NEG_SAMPLEA_VREFN;
    SetChanADC10(channel);
    TRISB = 0x03;
    //on utilisera l'autosampling mais pour le moment on le désactive il sera activé par la mise à 1
    //du bit ASAM de ADC1CON1
    config1 = ADC_MODULE_OFF | ADC_IDLE_STOP | ADC_FORMAT_SIGN_INT | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_OFF | ADC_SAMP_OFF;
    config2 = ADC_VREF_AVDD_AVSS | ADC_SCAN_ON | ADC_INTR_16_CONV | ADC_ALT_BUF_OFF | ADC_ALT_INPUT_OFF;
    config3 = ADC_CONV_CLK_SYSTEM | ADC_SAMPLE_TIME_8 | ADC_CONV_CLK_3Tcy;
    //TAD=2Tcy=125ns qui est sup au min exigé par le datasheet 75ns, le temps global d'une conversion est 12TAD=24Tcy=1500ns
    //le temps de sampling est 8TAD=16*Tcy=16*65ns=1.04us
    //un sampling+conversion dure 2.54us, 16 conversions dure 40us < 2ms=Te période d'échantillonnage
    //le reste pour faire le calcul et passer la main aux autres tâches
    configport = ENABLE_AN0_ANA | ENABLE_AN1_ANA | ENABLE_AN2_DIG | ENABLE_AN3_DIG | ENABLE_AN4_DIG | ENABLE_AN5_DIG | ENABLE_AN6_DIG;
    // ou AD1PCFG=0x03; //configuration du RB0 et RB1 en tant qu'entrées analogiques, les autres des sorties digitales
    configscan = 3; //scanner AN0 et AN1 seulement --> registre AD1CSL
    //application de la config
    OpenADC10(config1, config2, config3, configport, configscan); //configurer ADC avec les paramètres choisis
    DisableIntADC1; //desactiver les interruptions de fin de conversion
    EnableADC1; //activer l'ADC
}


//config du Timer2 pour génerer une interruption une fois toute les 2ms
//priorité d'interruption du TMR2 est 1 comme celle de freertos (TMR1): voir cours
void configTMR2(void) {
    
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_1); //activer les interruptions du Timmer2 avec priorité 1 comme celle du kernel freertos
    OpenTimer2(T2_ON | T2_PS_1_1, 32000-1); //Timer est configuré pour génerer des interruptions chaque Te=2ms
    //avec un Tcy=2*Tosc=2/32Mhz=1/16MHz=62,5ns
    //presecaler 1:1 , PR2 recoit (Te/Tcy)-1=2ms/62,5ns -1=32000-1
    
}

//tâche de conversion et calcul,
void vCapt(void *p) {
    long int I = 0; //crt brut : -25/+25 A --> tension 0 à 5V (entrée de l'ADC /AN0) --> sortie d'ADC -512/511
    long int V = 0; //tension brute:  -350V/350V -->capteur--> tension 0 à 5V (entrée de l'ADC/AN1) --> sortie ADC -512/511
    long int P = 0; //Puissance en valeur numérique brute
    const float Icoeff = (float) 50 / 1023; //coeff de convers de valeur numérique brute en A
    const float Vcoeff = (float) 700 / 1023; //coeff de convers de valeur numérique brute en V
    float Imes; //en A
    float Vmes; //en V
    float Pmes; //en Watt
    float Fpmes; //facteur de puissance
    int F = 5; //fréquence du réseau on suppose que c'est 5Hz et non pas 50Hz, voir remarque sur classroom
    int N = 100; //nb d'échantillons par période
    float Te = (float) 1 / (F * N); //période d'échantillonnage --> Te=0.002s=2ms
    float W = 0; //en Wh (Watt.heure)cette variable doit etre stockée dans une mémoire non volatile
    int compt = N;//compteur du nombre d'échantillons
    int compt1=300;
    int compt2=5;
    int Nb = 0; //nb de periode
    signed int ADCValue0, ADCValue1, count; //pour récupérer les 8 valeurs issues des buffers de conversion et les moyenner
    signed int *ADC16Ptr;
    xMessage d;//variable de type structure regroupe les differents variable Ieff ,P,Veff,Weff,FP.
    char x0[20];
    char x1[20];
    char x2[20];
    char x3[20];
    char x4[20];
    
    //config et activation de l'ADC
    configAdc();
    
    //A compléter par la création d'un sémaphore et l'utilisation d'un sémaphore
    vSemaphoreCreateBinary(xSemaphore);//creation d'un semaphore binaire
    xSemaphoreTake(xSemaphore,portMAX_DELAY);
    //config du TMR2 et son activation pour commencer à génerer les interruptions
    configTMR2();
    //boucle infini chaque 2ms converion puis calcul
    
    while (1) {
        
       
        xSemaphoreTake(xSemaphore,portMAX_DELAY);//bloquage jusqu'a la reception d'un jeton
        
         ADCValue0 = 0; // remettre les accumulateurs à 0
        ADCValue1 = 0;
        ADC16Ptr = &ADC1BUF0; // pointer le premier buffer ADCBUF0
        IFS0bits.AD1IF = 0; // mettre le falg de fin de conversion à 0
        AD1CON1bits.ASAM = 1; // lancer auto-sampling suivi d'une conversion automatique après 8 TAD#2us
        while (!IFS0bits.AD1IF); //attendre la fin de 16 conversion successives
        AD1CON1bits.ASAM = 0; // arreter le sampling/conversion automatique pour récupérer les 16 valeurs
        for (count = 0; count < 8; count++) // faire la moyenne des 8 valeurs pour chaque canal
        {
            ADCValue0 = ADCValue0 + *ADC16Ptr++; //cumuler les 8 valeurs successives de AN0
            ADCValue1 = ADCValue1 + *ADC16Ptr++; //cumuler les 8 valeurs successives de AN1
        }
        //calcul de la valeur moyenne ; pour éliminer l'effet du bruit

        ADCValue0 = ADCValue0 >> 3; //division par 8 --> un echantillon Ii
        ADCValue1 = ADCValue1 >> 3; //division par 8 --> un echantillon Vi
        //a ce niveau on a un echantillon Ii et Vi de i(t) et de v(t) en valeurs numériques brutes
        I = I + (long int) ADCValue0 *ADCValue0; //cumuler les Ii^2 dans I
        V = V + (long int) ADCValue1 *ADCValue1; //cumuler les Ii^2 dans V
        P = P + (long int) ADCValue0 *ADCValue1; //cumuler les Ii*Vi dans P
        //compteur d'énergie proprement dit est la variable W qui est un intégrateur
        W = W + (float) ADCValue0 * ADCValue1 *  Te *Vcoeff*Icoeff/  3600; //en Wh
        Nb++;
        compt--; //decrementer compteur d'échantillons puis reboucler pour acequir d'autres un autre echantillon Ii et Vi
        // après le cumul de N échantillons (après N*Te soit une période T du signal i(t))
        //calculer les valeurs efficaces en valeurs num brutes et convertir en A ou en V,la puissance active en Watt
        if (compt == 0) { //une periode de i(t) et v(t) est écoulée on peut calculer les val efficaces et la puiss active
            compt = N;
            I=I / N; 
            Imes = (float) sqrt((float) I) * Icoeff; //valeur efficace du courant en Amp
            V = V / N;
            Vmes = (float) sqrt((float) V) * Vcoeff; //valeur efficace du courant en Volt
            P = P /  N; //en val num brute
            Pmes = (float) P * Icoeff*Vcoeff; //en watt
            Fpmes = Pmes / (Vmes * Imes);
           
          compt1--;
         if (compt1 == 0) { //affichage chaque min (300*100 echantillons)
            compt1 =300;
             sprintf((unsigned char *) x0, "%s%3f%s", "Ieff=",Imes, "A");
             d.s0=x0;
            sprintf((unsigned char *) x1, "%s%3f%s", "Veff=",Vmes, "V");
            d.s1=x1;
            sprintf((unsigned char *) x2, "%s%3f%s", "P=",Pmes, "W");
            d.s2=x2;
            sprintf((unsigned char *) x3, "%s%3f%s", "W=",W, "wh");
            d.s3=x3;
            sprintf((unsigned char *) x4, "%s%3f%s", "F=",Fpmes, " ");
            d.s4=x4;
            xQueueSendToBack(xLCDQueue,&d,portMAX_DELAY); //envoyer ce message a la file chaque minute
            compt2--;
         if (compt2 == 0) {//journalisation chaque 5min (5*300*100 echantillons)
             compt2=5;//envoyer dans la file tous les 5mins
               xQueueSendToBack(xEEPROMQueue,&d,portMAX_DELAY);
             }
             }
       
            I=0;V=0;P=0;
         }
      
    }
}

void __attribute__((__interrupt__, auto_psv)) _T2Interrupt(void){
   
     signed portBASE_TYPE pxHigherPriorityTaskWoken=pdFALSE;
    IFS0bits.T2IF=0; //remettre le flag a 0
    xSemaphoreGiveFromISR(xSemaphore,&pxHigherPriorityTaskWoken);
   
    _TRISD0=0;    _RD0=~_RD0; //juste pour vérifier sur oscillo la période des intérruptions
   if (pxHigherPriorityTaskWoken==pdTRUE) 
       taskYIELD();
    
}
int xStartLCDTask(void) {
 xLCDQueue = xQueueCreate(3,sizeof (xMessage));
 xEEPROMQueue= xQueueCreate(3,sizeof (xMessage));
 if ((xLCDQueue != NULL)&& (xEEPROMQueue !=NULL)&& (xTaskCreate(vLCD, "LCD", 800, NULL, 1, NULL) == pdPASS)&&(xTaskCreate(vI2CEEprom, "Task2", 500, NULL, 1, NULL )==pdPASS))
 return (1); //retourne 1 si les files et les tâches sont créées
 else return (0); //retourne 0 sinon
}