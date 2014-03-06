#define PWM_FREQ  20e3  //Hz
#define CTRL_SAMP_TIME 0.5e-3 //Sec

#define TIC   GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x40);
#define TOC   GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);

#define LEFT  GPIO_PIN_2
#define RIGHT GPIO_PIN_3
#define UP    GPIO_PIN_0
#define DOWN  GPIO_PIN_1


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


volatile int timerFlag;

int ulPeriod;
char str[40];
const int encStates[4] = {0,1,3,2};

double pwm;
double curr, currErr, currErrInt, currSetPnt;
double spd, spdErr, spdErrInt, spdSetPnt;
double pos, posErr, posSetPnt;
double pendSpd;
double pendPos;
enum Mode {
  Open = 0,
  Stab = 1,
} mode;
  
int counter;
int flag;

int motEncPrevState;
long motEncAngle;
long motEncPeriod;
int motPrevDiff;

int pendEncPrevState;
long pendEncAngle;
long pendEncPeriod;
int pendPrevDiff;


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
double prevPwmInc = 0;
void Timer0IntHandler(void)
{
  TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
  timerFlag = 1;
}


void GPIOBIntHandler(void)
{
  int state;
  int diff;
  //
  // Clear the GPIO interrupt.
  //
  GPIOPinIntClear(GPIO_PORTB_BASE, GPIO_PIN_2|GPIO_PIN_3);    

  state = GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_2|GPIO_PIN_3) >> 2;
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
void GPIODIntHandler(void)
{
  int state;
  int diff;
  //
  // Clear the GPIO interrupt.
  //
  GPIOPinIntClear(GPIO_PORTD_BASE, GPIO_PIN_6|GPIO_PIN_5);    

  state = GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_6|GPIO_PIN_5) >> 5;
  diff = encStates[state]-encStates[pendEncPrevState];
  if (diff == 3) {
    diff = -1;
  } else if (diff == -3) {
    diff = 1;
  }
  pendEncAngle += diff;

  if (pendPrevDiff+diff==2) {
    pendEncPeriod = 0XFFFFFFFF-TimerValueGet64(TIMER2_BASE);
    TimerLoadSet64(TIMER2_BASE, 0xFFFFFFFF);
    TimerEnable(TIMER2_BASE, TIMER_A);
  } else if (pendPrevDiff+diff==-2) {
    pendEncPeriod = -(0XFFFFFFFF-TimerValueGet64(TIMER2_BASE));
    TimerLoadSet64(TIMER2_BASE, 0xFFFFFFFF);
    TimerEnable(TIMER2_BASE, TIMER_A);
  }
  
  pendEncPrevState = state;
  pendPrevDiff = diff;
}


void init(void) {
  SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
  SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

  // GPIO
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
  GPIOPadConfigSet(GPIO_PORTE_BASE, LEFT|RIGHT|UP|DOWN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
  GPIODirModeSet(GPIO_PORTE_BASE, LEFT|RIGHT|UP|DOWN, GPIO_DIR_MODE_IN);
  GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6);
  GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_2|GPIO_PIN_3);
  GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_6|GPIO_PIN_5);
  //GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, LEFT|RIGHT|UP|DOWN);
  GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_2|GPIO_PIN_3, GPIO_BOTH_EDGES);
  GPIOIntTypeSet(GPIO_PORTD_BASE, GPIO_PIN_6|GPIO_PIN_5, GPIO_BOTH_EDGES);
  GPIOPinIntEnable(GPIO_PORTB_BASE, GPIO_PIN_2|GPIO_PIN_3);
  GPIOPinIntEnable(GPIO_PORTD_BASE, GPIO_PIN_6|GPIO_PIN_5);
  IntEnable(INT_GPIOB);
  IntEnable(INT_GPIOD);

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
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
  TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
  TimerConfigure(TIMER1_BASE, TIMER_CFG_ONE_SHOT);
  TimerConfigure(TIMER2_BASE, TIMER_CFG_ONE_SHOT);
  TimerLoadSet(TIMER0_BASE, TIMER_A, (unsigned long)(SysCtlClockGet() * CTRL_SAMP_TIME));
  TimerLoadSet64(TIMER1_BASE, 0xFFFFFFFF);
  TimerLoadSet64(TIMER2_BASE, 0xFFFFFFFF);
  IntEnable(INT_TIMER0A);
  TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
  TimerEnable(TIMER0_BASE, TIMER_A);
  TimerEnable(TIMER1_BASE, TIMER_A);
  TimerEnable(TIMER2_BASE, TIMER_A);


  //Int
  /* IntPriorityGroupingSet(3);  */
  /* IntPrioritySet(INT_TIMER0A, 1<<5); */
  /* IntPrioritySet(INT_GPIOB, 1); */
  IntMasterEnable();

}

int main(void)
{
  double pwmInc;

  timerFlag = 0;

  pwm = 0;
  
  pendSpd = 0;
  currErrInt = 0;
  currSetPnt = 0; 
  spd = 0;
  spdErrInt = 0;
  spdSetPnt = 0; 
  posSetPnt = 0; 
  counter = 0;
  flag = 0;
  motEncPrevState = 0;
  motEncAngle = 0;
  motEncPeriod = 0xFFFFFFFF;
  motPrevDiff = 0;
  pendEncPrevState = 0;
  pendEncAngle = -0.328*4000*4;
  pendEncPeriod = 0xFFFFFFFF;
  pendPrevDiff = 0;
  mode = Open;
  init();

  usprintf(str, "Started !!!");
  RIT128x96x4StringDraw(str , 10, 24, 15);

  while(1){
    while (!timerFlag) {
    }
    timerFlag = 0;
    TIC;
    /* usprintf(str, "pwm = %6d",  (int)(pwm*1000)); */
    /* RIT128x96x4StringDraw(str , 10, 24, 15); */
    /* usprintf(str, "mot = %6d",  (int)(pos*1000)); */
    /* RIT128x96x4StringDraw(str , 10, 34, 15); */
    /* usprintf(str, "pend = %6d",   (int)(pendPos*1000)); */
    /* RIT128x96x4StringDraw(str , 10, 44, 15); */
    /* usprintf(str, "curr = %6d",   (int)(curr*1000)); */
    /* RIT128x96x4StringDraw(str , 10, 54, 15); */


    if (GPIOPinRead(GPIO_PORTE_BASE, LEFT) == 0) {
      pwm = 0.37;
      writePwm(pwm);
      motEncAngle = 0;
    } else if (GPIOPinRead(GPIO_PORTE_BASE, RIGHT) == 0) {
      pwm = -0.37;
      writePwm(pwm);
      motEncAngle = 0;
    } else if (GPIOPinRead(GPIO_PORTE_BASE, UP) == 0) {
      mode = Stab;
    } else {
      if (mode != Stab) {
	pwm = 0;
	writePwm(pwm);
      }
    }
    
    if ((mode==Stab) && (pos > 1.2 || pos < -1.2 || pendPos < -0.125 || pendPos > 0.125)) {
      RIT128x96x4StringDraw(" !!!! HALTED !!!!" , 10, 64, 15);
      IntMasterDisable();
      pwm = 0;
      writePwm(pwm);
      while (1) {
      }
    }

    if (mode==Stab) {

      if (pendEncPeriod != 0) {
	pendSpd = 50000000.0/((double)pendEncPeriod)/4000/4;
      } else {
	pendSpd = 1e-10;
      }
      pendPos = pendEncAngle/4000.0/4;

      if (motEncPeriod != 0) {
	spd = 50000000.0/((double)motEncPeriod)/1633*2;
      } else {
	spd = 1e-10;
      }
      pos = motEncAngle/1633.0*2;

#define k1 -2
#define k2 -0.7
#define a1 4
#define a2 5

#define K1 (k1*a2/(1-k1)*a1)
#define K2 (k2*a1/(1-k1)*a1)
#define K3 (k2*a2/(1-k1)*a1)

      posSetPnt = K1 * pendPos + K2 * spd + K3 * pendSpd;
      
#define DEAD_ZONE 0.01
#define START_PWM 0.34
#define PWM_MAX 0.7

#define ACC_MAX 1e-3


      posErr = posSetPnt-pos;

      if (posErr < -DEAD_ZONE && posErr > DEAD_ZONE) {
	posErr = 0;
      }

      if (posErr > 0) {
	pwm = START_PWM;
      } else {
	pwm = -START_PWM;
      }

      pwmInc = .3*posErr;
      if (pwmInc-prevPwmInc > ACC_MAX) {
	pwmInc = prevPwmInc + ACC_MAX;
      } else if (pwmInc-prevPwmInc < -ACC_MAX) {

	pwmInc = prevPwmInc-ACC_MAX;
      } 
      pwm += pwmInc;
      prevPwmInc = pwmInc;

      if (pwm>PWM_MAX) {
	pwm=PWM_MAX;
      } else if (pwm <-PWM_MAX) {
	pwm=-PWM_MAX;
      }
      writePwm(pwm);

      /* counter = counter + 1; */
      /* if (counter == 10000) { */
      /* 	if (!flag) { */
      /* 	  posSetPnt = 0.2; */
      /* 	  flag = 1; */
      /* 	} else { */
      /* 	  posSetPnt = -.2; */
      /* 	  flag = 0; */
      /* 	} */
      /* 	counter = 0; */
      /* } */

    }
    TOC;
  }

}
