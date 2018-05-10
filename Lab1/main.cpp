#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "HAL_I2C.h"
#include "HAL_OPT3001.h"
#include <stdio.h>


bool g_bLampState = false;                  //Controls the state of the lamps
volatile int g_iStartCounter=0;             //Counter for the starting blink
float g_fInitialLight=0;                    //Initial light level
bool g_bInitialState=false;                 //Initial state of the lamp
int g_iLampAmount=1;                        //Amount of lamps to be controlled
volatile bool g_bPoweringCondition=false;   //Power On condition. Determines wheter the lamp should turn on or not.
volatile bool g_bStartFlag=true;            //This flag shows wether the starting subroutine has been executed or not
volatile int g_iPoweringCounter=0;          //Counter for the power on time
volatile float g_fLightLevel=0;             //Surrounding´s light level
volatile int g_iButtonState=0;              //State of the button: active or unactive

//Variables Globales para el ADC
volatile uint16_t g_u16AdcResult=0U;        //Result of the ADC
volatile uint32_t g_u32Index = 0U;          //Index of the vector that stores the results of the ADC.
volatile uint32_t g_u32Average = 0U;        //Average of the sound in the last 5s.
volatile uint32_t g_u32MinSound = 0U;       //Limit of the minimum sound that turn on the light.
int g_iSamplesArray[10];


void Start(){                           //This function determines the level light once the system has been powered,
    g_bStartFlag=false;                 //the Initial State and generates the initial blink
    g_iStartCounter=0;
    g_fInitialLight=OPT3001_getLux();   //Capture of the starting light level
    if(g_fInitialLight<15){
        g_bInitialState=true;
        g_bPoweringCondition=true;
    }
    else{
        g_bInitialState=false;
    }
    while(g_iStartCounter<6){
    }
    g_bStartFlag=true;
}

void TurnOnTime();                      //Forward definition of two functions.
void TimerConfigs2();

void TurnOffTime(){                     //This function acts like a main loop,if there is no power on condition, the lamp will remain turned off
    g_bPoweringCondition=false;
    TimerConfigs2();                    //Enables the Timer32_2, to be used by the ADC
    g_bLampState = false;
    switch(g_iLampAmount){              //Turn off the lamp(s)
            case 1:
                P2-> OUT &= ~(BIT0);
                break;

            case 2:
                P2-> OUT &= ~(BIT0|BIT1);
                break;

            case 3:
                P2-> OUT &= ~(BIT0|BIT1|BIT2);
                break;
        }
    while(g_bPoweringCondition==false){     //If there is a Power on Condition the lamp should turn on, or if the lamp is off, and the
        if(g_iButtonState==1){              //lamp is off, it should turn on. g_bPoweringCondition is modified on the interrupts
            g_iButtonState = 0;
            TurnOnTime();
        }
    }
    TurnOnTime();
}

void TurnOnTime(){                          //This function controls the time the lamp must remain turned on. If the lamp is on an the button
    g_bLampState = true;                    //is pressed, the lamp should turn off.
    //g_iPoweringCounter=0;
    switch(g_iLampAmount){
                case 1:
                    P2-> OUT |= BIT0;
                    break;

                case 2:
                    P2-> OUT |= BIT0|BIT1;
                    break;

                case 3:
                    P2-> OUT |= BIT0|BIT1|BIT2;
                    break;
            }
    while(g_iPoweringCounter<16){
        if(g_iButtonState==2){
            g_iButtonState=0;
            TurnOffTime();
        }
    }
    g_bPoweringCondition=false;
    if(g_bPoweringCondition==true){
        TurnOnTime();
    }
    else{
        TurnOffTime();
    }
}



void AuthomaticMode(){          //This function initiates the authomatic mode of the lamp. Its one time run function
    if(g_bInitialState==true){
        TurnOnTime();
    }
    else{
        TurnOffTime();
    }
}

void Setup(){                   //Initialization of the I2C,light sensor and configuration of the button
    Init_I2C_GPIO();
    I2C_init();
    OPT3001_init();
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD; // stop watchdog timer

    //Initialization of the Ports P3 (Button), P2 (Lights)

    P3->DIR = ~BIT5;
    P3->OUT = BIT5;
    P3->REN = BIT5;
    P3->DS = ~BIT5;
    P3->SEL0= ~BIT5;
    P3->SEL1= ~BIT5;
    P3->IES= ~BIT5;
    P3->IFG= ~BIT5;
    P3->IE= BIT5;

    switch(g_iLampAmount){
            case 1:
                P2-> DIR = BIT0;
                P2-> OUT &= ~(BIT0);
                break;

            case 2:
                P2-> DIR = BIT0|BIT1;
                P2-> OUT &= ~(BIT0|BIT1);
                break;

            case 3:
                P2-> DIR = BIT0|BIT1|BIT2;
                P2-> OUT &= ~(BIT0|BIT1|BIT2);
                break;
        }
}

void InterruptSetup(){                  //NVIC configuration for button, ADC and T32_1 interrupts

    NVIC_SetPriority(PORT3_IRQn,1);
    NVIC_EnableIRQ(PORT3_IRQn);

    NVIC_SetPriority(T32_INT1_IRQn,1);
    NVIC_EnableIRQ(T32_INT1_IRQn);

    NVIC_SetPriority(ADC14_IRQn,1);
    NVIC_EnableIRQ(ADC14_IRQn);

}

void TimerConfigs1(){                   //Configuration of Timer 32 1.
    TIMER32_1->LOAD = 0x16E360; //0.5s ---> @ 3Mhz
    TIMER32_1->CONTROL = TIMER32_CONTROL_SIZE | TIMER32_CONTROL_PRESCALE_0 | TIMER32_CONTROL_MODE | TIMER32_CONTROL_IE | TIMER32_CONTROL_ENABLE;
    }

void TimerConfigs2(){                   //Configuration of timer 32 2
    TIMER32_2->LOAD = 0x249F0; //0.5s ---> @ 3Mhz
    TIMER32_2->CONTROL = TIMER32_CONTROL_SIZE | TIMER32_CONTROL_PRESCALE_0 | TIMER32_CONTROL_MODE | TIMER32_CONTROL_IE | TIMER32_CONTROL_ENABLE;

    NVIC_SetPriority(T32_INT2_IRQn,1);
    NVIC_EnableIRQ(T32_INT2_IRQn);
}

void ADC_MIC_CONFIGURATION()
{
    //Set P4.3 for Analog Input. Switch from a general purpose (I/O) to the selected peripheral function
    P4->SEL0 |= BIT3;
    P4->SEL1 |= BIT3;
    P4->DIR &= ~BIT3; // Defines the P4.3 as input.

    ADC14->CTL0 = ADC14_CTL0_PDIV_0     | // Predivide by 1
                  ADC14_CTL0_SHS_0      | // Sample and Hold Source
                  ADC14_CTL0_DIV_7      | // Divide by 8, 3MHz / 8 = 375KHz
                  ADC14_CTL0_SSEL__MCLK | // Clock Source: MCLK = 3MHz.
                  ADC14_CTL0_SHT0_1     | // Selects sampling time = 8 cycles
                  ADC14_CTL0_ON         | //Core is ready to power up
                  ADC14_CTL0_SHP;         //Pulse Sample Mode on

    ADC14->MCTL[0] = ADC14_MCTLN_INCH_10 | //A10
                     ADC14_MCTLN_VRSEL_0; // V(R+) = AVCC, V(R-) = AVSS

    ADC14->CTL0 = ADC14->CTL0 | ADC14_CTL0_ENC; //Enables conversion
    ADC14->IER0 = ADC14_IER0_IE0; // Interrupt Enable: Enables the interrupt request for the ADC14IFG0.
}

/*Calculates the minimum limit of sound that turns on the lamp*/
void checkADC(int vector[10])
{
    g_u32Average = 0;
    for(int i = 0; i<10 ; i++)
    {
        g_u32Average += vector[i];
    }

    g_u32Average = g_u32Average / 10;

    g_u32MinSound = 1.010 * g_u32Average;
}


int main(void)
{
    Setup();
    TimerConfigs1();
    ADC_MIC_CONFIGURATION();
    InterruptSetup();
    Start();
    AuthomaticMode();
}


extern "C"
{

    void PORT3_IRQHandler(void){            //Button interrupt handler.
        __disable_irq();
        g_iPoweringCounter=0;
        P3->IFG=0;
        if(g_bLampState==false){            //
            g_iButtonState=1;
        }
        else{
            g_iButtonState=2;
            g_bPoweringCondition = false;
        }

        //Disables the Timer32_2
        TIMER32_2->CONTROL = 0U;
        NVIC_DisableIRQ(T32_INT2_IRQn);

        //g_bLampState^=true;
       __enable_irq();
        return;
    }

void T32_INT1_IRQHandler(void){             //T32 counter 1 interrupt handler. If the starting subroutine hasn´t been completely executed, it should
   __disable_irq();                         //toggle the P2->OUT state, to make the lamps blink. Also here the start counter increments.
   TIMER32_1->INTCLR = 0U;
   if(g_bStartFlag==false){
       P2->OUT = ~(P2->OUT);
       g_iStartCounter++;
   }
   else{
       g_iPoweringCounter++;                //If the starting routine has been executed, it should only increment the Power on time counter. This will always happen
   }                                        //but when there is a power on condition, the counter is cleared.
    __enable_irq();
    return;
}


//Fills the ADC samples array, updates the index,calculates the MinSound  and starts the conversion of ADC.
void T32_INT2_IRQHandler(void){
   __disable_irq();
   TIMER32_2->INTCLR = 0U;
   g_iSamplesArray[g_u32Index] = g_u16AdcResult;
   if(g_u32Index == 10)
      {
          g_u32Index = 0;
      }

      else
      {
          g_u32Index++;
      }
   checkADC(g_iSamplesArray);
   ADC14->CTL0 = ADC14->CTL0 | ADC14_CTL0_SC; //Start Conversion
    __enable_irq();
    return;
}

//Saves the result of the conversion, and checks if the lamp has to turn on.
void ADC14_IRQHandler(void)
{
    __disable_irq();
    g_u16AdcResult = ADC14->MEM[0];
    float luzLevel =OPT3001_getLux();
    if(g_u16AdcResult > g_u32MinSound && luzLevel<15 && g_u16AdcResult > 8250)
    {
         g_bPoweringCondition = true;
        g_iPoweringCounter=0; //ERICKA CAMBIO ESTO
    }
    ADC14->CLRIFGR0 = ADC14_CLRIFGR0_CLRIFG0;
    __enable_irq();
    return;
}

}







