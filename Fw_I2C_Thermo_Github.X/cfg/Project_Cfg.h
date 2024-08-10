#ifndef PROJECT_CFG_H
#define PROJECT_CFG_H
/* ******************************************************* C STANDARD LIBRARY */
#ifndef __LANGUAGE_ASSEMBLY__
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#endif

/* ************************************************************** MCC LIBRARY */
#include <xc.h>

#ifndef __LANGUAGE_ASSEMBLY__
#include "clock.h"
#include "coretimer.h"
#include "exceptions.h"
#include "interrupt_manager.h"
#include "pin_manager.h"
#include "system.h"
#include "uart3.h"
#include "watchdog.h"
#endif
/* ************************************************************** CLIB CONFIG */

#ifndef __LANGUAGE_ASSEMBLY__
#include "TickTimer_Cfg.h"
#endif

/* ************************************************************ COMMON MACROS */
// Global interrupt
#define Disable_Global_Interrupt()      __builtin_disable_interrupts()
#define Enable_Global_Interrupt()       __builtin_enable_interrupts()
// Peripheral interrupt
#define Disable_Peripheral_Interrupt()  // Not implemented in PIC32
#define Enable_Peripheral_Interrupt()   // Not implemented in PIC32
// Core timer interrupt
#define Disable_Coretimer_Interrupt()   (IEC0CLR=_IEC0_CTIE_MASK)
#define Enable_Coretimer_Interrupt()    (IEC0SET=_IEC0_CTIE_MASK)
#define Clear_Coretimer_Interrupt()     (IFS0CLR=_IFS0_CTIF_MASK)

#define ClrWdt()                        WATCHDOG_TimerClear()

#define DummyInterruptHandler()
#define LibComp_Initialize()

#endif