#define MON_DATA_LEN 100

#include "utils/locator.h"
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "utils/ustdlib.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "drivers/rit128x96x4.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "enet.h"




//*****************************************************************************
//
// Display an lwIP type IP Address.
//
//*****************************************************************************
void
DisplayIPAddress(unsigned long ipaddr, unsigned long ulCol,
		   unsigned long ulRow) {
  char pucBuf[16];
  unsigned char *pucTemp = (unsigned char *)&ipaddr;
  //
  // Convert the IP Address into a string.
  //
  usprintf(pucBuf, "%d.%d.%d.%d", pucTemp[0], pucTemp[1], pucTemp[2],
	   pucTemp[3]);

  //
  // Display the string.
  //
  RIT128x96x4StringDraw(pucBuf, ulCol, ulRow, 15);
}


//*****************************************************************************
//
// Required by lwIP library to support any host-related timer functions.
//
//*****************************************************************************
void
lwIPHostTimerHandler(void)
{
  /* static unsigned long ulLastIPAddress = 0; */
  /* unsigned long ulIPAddress; */

  /* ulIPAddress = lwIPLocalIPAddrGet(); */

  /* // */
  /* // If IP Address has not yet been assigned, update the display accordingly */
  /* // */
  /* if(ulIPAddress == 0) */
  /*   { */
  /*     static int iColumn = 6; */

  /*     // */
  /*     // Update status bar on the display. */
  /*     // */
  /*     /\* RIT128x96x4Enable(1000000); *\/ */
  /*     if(iColumn < 12) */
  /*       { */
  /* 	  RIT128x96x4StringDraw(" >", 114, 24, 15); */
  /* 	  RIT128x96x4StringDraw("< ", 0, 24, 15); */
  /* 	  RIT128x96x4StringDraw("*",iColumn, 24, 7); */
  /*       } */
  /*     else */
  /*       { */
  /* 	  RIT128x96x4StringDraw(" *",iColumn - 6, 24, 7); */
  /*       } */

  /*     iColumn += 4; */
  /*     if(iColumn > 114) */
  /*       { */
  /* 	  iColumn = 6; */
  /* 	  RIT128x96x4StringDraw(" >", 114, 24, 15); */
  /*       } */
  /*     /\* RIT128x96x4Disable(); *\/ */
  /*   } */

  /* // */
  /* // Check if IP address has changed, and display if it has. */
  /* // */
  /* else if(ulLastIPAddress != ulIPAddress) */
  /*   { */
  /*     ulLastIPAddress = ulIPAddress; */
  /*     /\* RIT128x96x4Enable(1000000); *\/ */
  /*     RIT128x96x4StringDraw("                       ", 0, 16, 15); */
  /*     RIT128x96x4StringDraw("                       ", 0, 24, 15); */
  /*     RIT128x96x4StringDraw("IP:   ", 0, 16, 15); */
  /*     RIT128x96x4StringDraw("MASK: ", 0, 24, 15); */
  /*     RIT128x96x4StringDraw("GW:   ", 0, 32, 15); */
  /*     DisplayIPAddress(ulIPAddress, 36, 16); */
  /*     ulIPAddress = lwIPLocalNetMaskGet(); */
  /*     DisplayIPAddress(ulIPAddress, 36, 24); */
  /*     ulIPAddress = lwIPLocalGWAddrGet(); */
  /*     DisplayIPAddress(ulIPAddress, 36, 32); */
  /*     /\* RIT128x96x4Disable(); *\/ */
  /*   } */
}


void initEnet() {
  unsigned long ulUser0, ulUser1;
  unsigned char pucMACArray[8];

 
  FlashUserGet(&ulUser0, &ulUser1);
  if((ulUser0 == 0xffffffff) || (ulUser1 == 0xffffffff))
    {
      RIT128x96x4StringDraw("MAC Address", 0, 16, 15);
      RIT128x96x4StringDraw("Not Programmed!", 0, 24, 15);
      while(1);
    }

  pucMACArray[0] = ((ulUser0 >>  0) & 0xff);
  pucMACArray[1] = ((ulUser0 >>  8) & 0xff);
  pucMACArray[2] = ((ulUser0 >> 16) & 0xff);
  pucMACArray[3] = ((ulUser1 >>  0) & 0xff);
  pucMACArray[4] = ((ulUser1 >>  8) & 0xff);
  pucMACArray[5] = ((ulUser1 >> 16) & 0xff);

  lwIPInit(pucMACArray, 0, 0, 0, IPADDR_USE_DHCP);

  LocatorInit();
  LocatorMACAddrSet(pucMACArray);
  LocatorAppTitleSet("EK-LM3S6965 enet_io");

  tcp_init2();
}


unsigned char tcp_buffer1[MON_DATA_LEN*sizeof(long)];
unsigned char tcp_buffer2[MON_DATA_LEN*sizeof(long)];
long * monWriteBuf;
long monDataCount;

inline void writeMonData(long data) {
  if (monDataCount < MON_DATA_LEN - 1) {
    monWriteBuf[monDataCount++] = data;
  }
}

static void close_conn (struct tcp_pcb *pcb )
{
  tcp_arg(pcb, NULL);
  tcp_sent(pcb, NULL);
  tcp_recv(pcb, NULL);
  tcp_close(pcb);
}
static err_t echo_recv( void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err )
{
  int i;
  int len;
  char *pc;
 
  if ( err == ERR_OK && p != NULL )
    {
      tcp_recved( pcb, p->tot_len );
      pc = (char *)p->payload;
      len =p->tot_len;
 

      if( pc[0] == 'X' )
	close_conn( pcb );
      pbuf_free( p );

      if (monWriteBuf == (long *)tcp_buffer1) {
	monWriteBuf[MON_DATA_LEN-2] = monDataCount;
	tcp_write( pcb, tcp_buffer1, MON_DATA_LEN*sizeof(long), 0 );
	monWriteBuf = (long *)tcp_buffer2;
	
      } else {
	monWriteBuf[MON_DATA_LEN-2] = monDataCount;
	tcp_write( pcb, tcp_buffer2, MON_DATA_LEN*sizeof(long), 0 );
	monWriteBuf = (long *)tcp_buffer1;
      }
      monDataCount = 0;
      tcp_sent( pcb, NULL );
    }
  else
    {
      pbuf_free( p );
    }
 
  if( err == ERR_OK && p == NULL )
    {
      close_conn( pcb );
    }
 
  return ERR_OK;
}
static err_t echo_accept(void *arg, struct tcp_pcb *pcb, err_t err )
{
  LWIP_UNUSED_ARG( arg );
  LWIP_UNUSED_ARG( err );
  tcp_setprio( pcb, TCP_PRIO_MIN );
  tcp_recv( pcb, echo_recv );
  tcp_err( pcb, NULL );
  tcp_poll( pcb, NULL, 4 );
  return ERR_OK;
}

void tcp_init2( void )
{
  
  /* struct tcp_pcb *tcp_pcb; */
  /* monWriteBuf = (long *)tcp_buffer1; */
  /* monDataCount = 0; */

  /* tcp_pcb = tcp_new(); */
  /* tcp_bind(tcp_pcb, IP_ADDR_ANY, 23); */
 
  /* tcp_pcb = tcp_listen( tcp_pcb ); */
  /* tcp_accept( tcp_pcb, echo_accept ); */
}
