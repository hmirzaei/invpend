#include "utils/lwiplib.h"

//*****************************************************************************
//
// Timeout for DHCP address request (in seconds).
//
//*****************************************************************************
#ifndef DHCP_EXPIRE_TIMER_SECS
#define DHCP_EXPIRE_TIMER_SECS  45
#endif


void DisplayIPAddress(unsigned long ipaddr, unsigned long ulCol, unsigned long ulRow);
void Timer0IntHandler(void);
void lwIPHostTimerHandler(void);
void initEnet();
static void close_conn (struct tcp_pcb *pcb );
static err_t echo_recv( void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err );
static err_t echo_accept(void *arg, struct tcp_pcb *pcb, err_t err );
void writeMonData(long data);
void tcp_init2( void );
