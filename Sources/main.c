#include "MPC5604B.h"
#include "IntcInterrupts.h"
#include "tmp.h"


void initEMIOS_0ch23(void);
void hardwareSetup(void);
void initModesAndClock(void);
void initPeriClkGen(void);
void disableWatchdog(void);
void initINTC(void);
void hardwareSetup(void) ;
void systemTimerSetup(void);
void stmISR(void);

void initModesAndClock(void)
{
	ME.MER.R = 0x0000001D;          	/* Enable DRUN, RUN0, SAFE, RESET modes */
	                              		/* Initialize PLL before turning it on: */
										/* Use 1 of the next 2 lines depending on crystal frequency: */
	CGM.FMPLL_CR.R = 0x02400100;    	/* 8 MHz xtal: Set PLL0 to 64 MHz */   
	/*CGM.FMPLL_R = 0x12400100;*/     	/* 40 MHz xtal: Set PLL0 to 64 MHz */   
	ME.RUN[0].R = 0x001F0074;       	/* RUN0 cfg: 16MHzIRCON,OSC0ON,PLL0ON,syclk=PLL */
	
	//ME.RUNPC[0].R = 0x00000010; 	  	/* Peri. Cfg. 0 settings: only run in RUN0 mode */
   										/* Use the next lines as needed for MPC56xxB/S: */  	    	
	//ME.PCTL[48].R = 0x0000;         	/* MPC56xxB LINFlex0: select ME.RUNPC[0] */	
	//ME.PCTL[68].R = 0x0000;         	/* MPC56xxB/S SIUL:  select ME.RUNPC[0] */	
	
	ME.RUNPC[1].R = 0x00000010;     	/* Peri. Cfg. 1 settings: only run in RUN0 mode */
	ME.PCTL[32].R = 0x01;       		/* MPC56xxB ADC 0: select ME.RUNPC[1] */
  	ME.PCTL[57].R = 0x01;       		/* MPC56xxB CTUL: select ME.RUNPC[1] */
  	ME.PCTL[48].R = 0x01;           	/* MPC56xxB/P/S LINFlex 0: select ME.RUNPC[1] */
	ME.PCTL[68].R = 0x01;           	/* MPC56xxB/S SIUL:  select ME.RUNPC[1] */
	ME.PCTL[72].R = 0x01;           	/* MPC56xxB/S EMIOS 0:  select ME.RUNPC[1] */
	                              		/* Mode Transition to enter RUN0 mode: */
	ME.MCTL.R = 0x40005AF0;         	/* Enter RUN0 Mode & Key */
	ME.MCTL.R = 0x4000A50F;         	/* Enter RUN0 Mode & Inverted Key */  
	while (ME.GS.B.S_MTRANS) {}     	/* Wait for mode transition to complete */    
	                          			/* Note: could wait here using timer and/or I_TC IRQ */
	while(ME.GS.B.S_CURRENTMODE != 4) {}/* Verify RUN0 is the current mode */
	
	//while (ME.IS.B.I_MTC != 1) {}    /* Wait for mode transition to complete */    
	//ME.IS.R = 0x00000001;           /* Clear Transition flag */ 
}

void initPeriClkGen(void)
{	
	CGM.SC_DC[0].R = 0x80; 				/* MPC56xxB/S: Enable peri set 1 sysclk divided by 1 */
	CGM.SC_DC[2].R = 0x80;         		/* MPC56xxB: Enable peri set 3 sysclk divided by 1*/
}

void disableWatchdog(void)
{
	
}

void initINTC(void)
{
	INTC.MCR.B.HVEN = INTC_HVEN_ENABLE;
	INTC.CPR.R = 0xa;
	//INTC.PSR[15].R = 0x0d;
}

void hardwareSetup(void) 
{
	//enable clocking
	initModesAndClock();
	//enable peripheral clock gen
	initPeriClkGen();
	//disable wdt
	initINTC();
	disableWatchdog();
}

void initEMIOS_0ch23(void) {        	/* EMIOS 0 CH 23: Modulus Up Counter */
	EMIOS_0.CH[23].CADR.R = 999;      	/* Period will be 999+1 = 1000 clocks (1 msec)*/
	EMIOS_0.CH[23].CCR.B.MODE = 0x50; 	/* Modulus Counter Buffered (MCB) */
	EMIOS_0.CH[23].CCR.B.BSL = 0x3;   	/* Use internal counter */
	EMIOS_0.CH[23].CCR.B.UCPRE=0;     	/* Set channel prescaler to divide by 1 */
	EMIOS_0.CH[23].CCR.B.UCPEN = 1;   	/* Enable prescaler; uses default divide by 1*/
	EMIOS_0.CH[23].CCR.B.FREN = 1;   	/* Freeze channel counting when in debug mode*/
}

void systemTimerSetup(void)
{
	STM.CH[0].CMP.R = 0x3D090;			/* COUNT fro 1s */
	STM.CH[0].CCR.B.CEN = 0x1;			/* CH 0 enable */
	STM.CR.R = 0x0000ff03;				/* Prescaler 255, Timer enable 1 */
	//STM.CH[0].CIR = 0x1;
}

void stmISR(void)
{
	uint32_t value = 0x00;
	STM.CR.B.TEN = 0;
	STM.CNT.R = 0x0;
	
	STM.CH[0].CIR.B.CIF = 1;			/* Clear interrupt flag */
	STM.CR.B.TEN = 1;
	SIU.PGPDO[2].R ^= 0x08000000; 
	
	INTC.EOIR.R = 0;
	//SIU.PGPDO[2].R |= 0x0f000000;		/* Disable LEDs*/
	//SIU.GPDO[68] = !SIU.GPDO[68];
}

int main(void) {
  volatile int i = 0;

  
	// Hardware setup
	//INTC_InstallINTCInterruptHandler(stmISR, 30, 14);
	hardwareSetup();
	INTC.PSR[30].B.PRI = 0xd;
	
	SIU.PCR[68].R = 0x0200;				/* Program the drive enable pin of LED1 (PE4) as output*/
	SIU.PGPDO[2].R |= 0x0f000000;		/* Disable LEDs*/
	SIU.PGPDO[2].R &= 0x07000000;		/* Enable LED1*/
	
	systemTimerSetup();
	
	
	/* Loop forever */
	for (;;) {
	i++;
	}
}



