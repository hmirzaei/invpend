#define PWM_MAX  0.99   // [0 to 1] 
#define PWM_FREQ  20e3  //Hz
#define CTRL_SAMP_TIME 0.2e-3 //Sec
#define TIC   GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x40);
#define TOC   GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);

#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "utils/ustdlib.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/qei.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "drivers/rit128x96x4.h"
#include "driverlib/adc.h"


int ulPeriod;
char str[40];
const int encStates[4] = {0,1,3,2};
double pwm;
double curr, currErr, currErrInt, currSetPnt;
double spd, spdErr, spdErrInt, spdSetPnt;
double pos, posErr, posSetPnt;

int counter;
int flag;
int motEncPrevState;
long motEncAngle;
long motEncPeriod;
int motPrevDiff;


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

//*****************************************************************************
//
// The interrupt handler for the first timer interrupt.
//
//*****************************************************************************
void Timer0IntHandler(void)
{
  TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
  TIC;

  curr = readCurrent();
  currErr = currSetPnt-curr;
  currErrInt = currErrInt + currErr;
  pwm = 0.3*currErr+ 0.06 * currErrInt;
  if (pwm>PWM_MAX) {
    pwm=PWM_MAX;
  } else if (pwm <-PWM_MAX) {
    pwm=-PWM_MAX;
  }
  writePwm(pwm);

  if (motEncPeriod != 0) {
    spd = 50000000.0/((double)motEncPeriod)/1633;
  } else {
    spd = 1e-10;
  }
  spdErr = spdSetPnt-spd;
  spdErrInt = spdErrInt + spdErr;
  if (spdErrInt > 1000) {
    spdErrInt = 1000;
  } else if (spdErrInt < -1000) {
    spdErrInt = -1000;
  }
  currSetPnt = 0.7*spdErr;// +  0.003 * spdErrInt;
  if (currSetPnt > 3) {
    currSetPnt = 3;
  } else if (currSetPnt < -3) {
    currSetPnt = -3;
  }

  pos = motEncAngle/1633.0*2;
  posErr = posSetPnt-pos;
  spdSetPnt = 50*posErr;
  if (spdSetPnt > 2) {
    spdSetPnt = 2;
  } else if (spdSetPnt < -2) {
    spdSetPnt = -2;
  }

  counter = counter + 1;
  if (counter == 20000) {
    if (!flag) {
      posSetPnt =.1;
      flag = 1;
    } else {
      posSetPnt = -.1;
      flag = 0;
    }
    counter = 0;
  }

  /* IntMasterDisable(); */
  /* IntMasterEnable(); */
  TOC;
}


void GPIOEIntHandler(void)
{
  int state;
  int diff;
  //
  // Clear the GPIO interrupt.
  //
  GPIOPinIntClear(GPIO_PORTE_BASE, GPIO_PIN_2|GPIO_PIN_3);    

  state = GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_2|GPIO_PIN_3) >> 2;
  diff = encStates[state]-encStates[motEncPrevState];
  if (diff == 3) {
    diff = -1;
  } else if (diff == -3) {
    diff = 1;
  }
  motEncAngle += diff;

  if (motPrevDiff+diff==2) {
    motEncPeriod = 0XFFFFFFFF-TimerValueGet64(TIMER1_BASE);
    TimerLoadSet64(TIMER1_BASE, 0xFFFFFFFF);
    TimerEnable(TIMER1_BASE, TIMER_A);
  } else if (motPrevDiff+diff==-2) {
    motEncPeriod = -(0XFFFFFFFF-TimerValueGet64(TIMER1_BASE));
    TimerLoadSet64(TIMER1_BASE, 0xFFFFFFFF);
    TimerEnable(TIMER1_BASE, TIMER_A);
  }
  
  motEncPrevState = state;
  motPrevDiff = diff;
}


void init(void) {
  SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
  SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

  // GPIO
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
  GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6);
  GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_2|GPIO_PIN_3);
  GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_2|GPIO_PIN_3, GPIO_BOTH_EDGES);
  GPIOPinIntEnable(GPIO_PORTE_BASE, GPIO_PIN_2|GPIO_PIN_3);
  IntEnable(INT_GPIOE);

  // PWM0
  SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
  GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_0);
  ulPeriod = SysCtlClockGet() / PWM_FREQ;
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
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
  TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
  TimerConfigure(TIMER1_BASE, TIMER_CFG_ONE_SHOT);
  TimerLoadSet(TIMER0_BASE, TIMER_A, (unsigned long)(SysCtlClockGet() * CTRL_SAMP_TIME));
  TimerLoadSet64(TIMER1_BASE, 0xFFFFFFFF);
  IntEnable(INT_TIMER0A);
  TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
  TimerEnable(TIMER0_BASE, TIMER_A);
  TimerEnable(TIMER1_BASE, TIMER_A);

  /* //QEI */
  /* SysCtlPeripheralEnable(SYSCTL_PERIPH_QEI1); */
  /* GPIOPinTypeQEI(GPIO_PORTE_BASE, GPIO_PIN_2 |  GPIO_PIN_3); */
  /* QEIConfigure(QEI1_BASE, (QEI_CONFIG_CAPTURE_A_B  | QEI_CONFIG_NO_RESET | QEI_CONFIG_QUADRATURE | QEI_CONFIG_NO_SWAP), 100000000); */
  /* QEIVelocityConfigure(QEI1_BASE, QEI_VELDIV_1, SysCtlClockGet()/10); */
  /* QEIPositionSet(QEI1_BASE, 0); */
  /* QEIEnable(QEI1_BASE); */
  /* QEIVelocityEnable(QEI1_BASE); */

  //Int
  IntMasterEnable();

}

int main(void)
{
  currErrInt = 0;
  currSetPnt = 0; 
  spdErrInt = 0;
  spdSetPnt = 0; 
  posSetPnt = 0; 
  counter = 0;
  flag = 0;
  motEncPrevState = 0;
  motEncAngle = 0;
  motEncPeriod = 0xFFFF;
  motPrevDiff = 0;

  init();

  usprintf(str, "Started !!!");
  RIT128x96x4StringDraw(str , 10, 24, 15);

  while(1){
    usprintf(str, "spd = %6d",  (int)(spd*1000));
    RIT128x96x4StringDraw(str , 10, 24, 15);
    usprintf(str, "pos = %6d",  (int)(pos*1000));
    RIT128x96x4StringDraw(str , 10, 34, 15);
    usprintf(str, "curr = %6d",   (int)(currSetPnt*1000));
    RIT128x96x4StringDraw(str , 10, 44, 15);
    SysCtlDelay(SysCtlClockGet() / 50);
  }

}
