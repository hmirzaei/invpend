#define PWM_MAX  0.8
#define CTRL_SAMP_TIME 0.2e-3
#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "utils/ustdlib.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "drivers/rit128x96x4.h"
#include "driverlib/adc.h"


//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

int ulPeriod;
char str[40];
double current, pwm, e, currErrInt, currSetPnt;
int counter;
    

inline void writePwm(double pwm) {
  if (pwm < 0) {
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, ulPeriod * -pwm);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4|GPIO_PIN_5, 0x10);
  } else {
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, ulPeriod * pwm);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4|GPIO_PIN_5, 0x20);
  }
}

inline double readCurrent() {
  unsigned long ulADC0_Value[8];
  int pinVal;

  pinVal = GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_4|GPIO_PIN_5);

  ADCProcessorTrigger(ADC0_BASE, 0);
  while(!ADCIntStatus(ADC0_BASE, 0, false)){
  }
  ADCIntClear(ADC0_BASE, 0);
  ADCSequenceDataGet(ADC0_BASE, 0, ulADC0_Value);

  if (pinVal == 0x10) {
    return ulADC0_Value[0] * -0.002643849203542  +0.328681546195706;	  
  } else {
    return ulADC0_Value[0] * 0.002664294671601   +0.061590507306012;
  }
} 

//*****nn************************************************************************
//
// The interrupt handler for the first timer interrupt.
//nnnnnnnnn
//*****************************************************************************
void Timer0IntHandler(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x40);

    current = readCurrent();
    e = currSetPnt-current;
    currErrInt = currErrInt + e;
    pwm = .2*e+ .06 * currErrInt; 
    if (pwm>PWM_MAX) {
      pwm=PWM_MAX;
    } else if (pwm <-PWM_MAX) {
      pwm=-PWM_MAX;
    }
    writePwm(pwm);
    counter = counter + 1;
    if (counter == 1000) {
      currSetPnt = -currSetPnt;
      counter = 0;
    }

    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
    /* IntMasterDisable(); */
    /* IntMasterEnable(); */
}


void init(void) {
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
    SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

    // GPIO
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6);

    // PWM0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_0);
    ulPeriod = SysCtlClockGet() / 20000;
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1,  PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, ulPeriod);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, ulPeriod * 0 /4);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4|GPIO_PIN_5, 0x10);
    PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT , true);
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    RIT128x96x4Init(1000000);

    // ADC0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlADCSpeedSet(SYSCTL_ADCSPEED_1MSPS);
    ADCSequenceDisable(ADC0_BASE, 0);
    ADCHardwareOversampleConfigure(ADC0_BASE, 8);
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH0 | ADC_CTL_END| ADC_CTL_IE);
    ADCSequenceEnable(ADC0_BASE, 0);
    ADCIntClear(ADC0_BASE, 0);


    //TIMER
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    IntMasterEnable();
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, (unsigned long)(SysCtlClockGet() * CTRL_SAMP_TIME));
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER0_BASE, TIMER_A);
}

int main(void)
{
    currErrInt = 0;
    currSetPnt = 0.3; 
    counter = 0;

    init();

    usprintf(str, "Started !!!");
    RIT128x96x4StringDraw(str , 10, 24, 15);

    while(1){
      usprintf(str, "adc = %10d", counter);
      RIT128x96x4StringDraw(str , 10, 24, 15);
    }

}
