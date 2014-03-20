#define PWM_FREQ 15e3  //Hz
#define CTRL_SAMP_TIME 3e-3 //Sec
#define PWM_MAX 0.99

#define TIC   GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x40);
#define TOC   GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);

#define LEFT  GPIO_PIN_2
#define RIGHT GPIO_PIN_3
#define UP    GPIO_PIN_0
#define DOWN  GPIO_PIN_1


#include <string.h>
#include <math.h>
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


enum Mode {
  Open = 0,
  Stab = 1,
} mode;

volatile int timerFlag;

int ulPeriod;
char str[40];
const int encStates[4] = {0,1,3,2};

double pwm;
double pendSpd, pendPos, pos, spd;

  
int motEncPrevState;
long motEncAngle;
long motEncPeriod;
int motPrevDiff;

int pendEncPrevState;
long pendEncAngle;
long pendEncPeriod;
int pendPrevDiff;


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


void Timer0IntHandler(void)
{
  TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
  timerFlag = 1;
}


void GPIOBIntHandler(void)
{
  int state;
  int diff;
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
  IntMasterEnable();

}

int main(void)
{
  timerFlag = 0;

  pwm = 0;
  
  pendPos = 0;
  pendSpd = 0;
  pos = 0;
  spd = 0;


  motEncPrevState = 0;
  motEncAngle = 0;
  motEncPeriod = 0xFFFFFFFF;
  motPrevDiff = 0;
  pendEncPrevState = 0;
  pendEncAngle = -0.287*4000*4;
  pendEncPeriod = 0xFFFFFFFF;
  pendPrevDiff = 0;

  mode = Open;

  init();

  usprintf(str, "Started !!!");
  RIT128x96x4StringDraw(str , 10, 24, 15);

  while(1){

    // wait for new control sampling time
    while (!timerFlag) {
    }
    timerFlag = 0;
    TIC;

    // push button actions
    if (GPIOPinRead(GPIO_PORTE_BASE, LEFT) == 0) {
      pwm = 0.4;
      writePwm(pwm);
      motEncAngle = 0;
    } else if (GPIOPinRead(GPIO_PORTE_BASE, RIGHT) == 0) {
      pwm = -0.4;
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
    
    // safety conditions
    if ((mode==Stab) && (pos > 3.2 || pos < -3.2 || pendPos < -0.25 || pendPos > 0.25)) {
      RIT128x96x4StringDraw(" !!!! HALTED !!!!" , 10, 64, 15);
      IntMasterDisable();
      pwm = 0;
      writePwm(pwm);
      while (1) {
      }
    }

    if (mode==Open) {
      //updating pendulum and motor position and speed vars
      if (pendEncPeriod != 0) {
	pendSpd = 50000000.0/((double)pendEncPeriod)/4000/4;
      } else {
	pendSpd = 1e-10;
      }
      pendPos = pendEncAngle/4000.0/4;

      if (motEncPeriod != 0) {
	spd = 50000000.0/((double)motEncPeriod)/720;
      } else {
	spd = 1e-10;
      }
      pos = motEncAngle/720.0;

      // display position and speed vars on LCD 
      usprintf(str, "pend = %6d",   (int)(pendPos*1000));
      RIT128x96x4StringDraw(str , 10, 14, 15);
      usprintf(str, "pendspd=%6d",   (int)(pendSpd*1000));
      RIT128x96x4StringDraw(str , 10, 24, 15);
      usprintf(str, "mot = %6d",  (int)(pos*1000));
      RIT128x96x4StringDraw(str , 10, 34, 15);
      usprintf(str, "motspd=%6d",  (int)(spd*1000));
      RIT128x96x4StringDraw(str , 10, 44, 15);
    }

    if (mode==Stab) {

      //updating pendulum and motor position and speed vars
      if (pendEncPeriod != 0) {
	pendSpd = 50000000.0/((double)pendEncPeriod)/4000/4;
      } else {
	pendSpd = 1e-10;
      }
      pendPos = pendEncAngle/4000.0/4;


      // control law
      pwm = 100*pendPos-1.5*pendSpd;
      if (pwm>PWM_MAX) {
      	pwm=PWM_MAX;
      } else if (pwm <-PWM_MAX) {
      	pwm=-PWM_MAX;
      }
      writePwm(pwm);
    }

    TOC;
  }

}
