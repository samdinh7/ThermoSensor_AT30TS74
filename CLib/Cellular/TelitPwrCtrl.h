#ifndef TELITPWRCTRL_H
#define	TELITPWRCTRL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef USE_PROJECT_CFG
#include "Project_Cfg.h"
#else
#include "TelitPwrCtrl_Cfg.h"
#endif

typedef union {
    uint8_t val;

    struct {
        unsigned dischrErr : 1; // VCEL discharge error
        unsigned vcelRdy : 1; // VCEL ready
        unsigned vauxRdy : 1; // VAUX ready
        unsigned swRdy : 1; // Software ready
        unsigned reboot : 1; // Reboot state
        unsigned rfu : 2; // Unused
        unsigned busy : 1; // Internal process is in progress
    };
} telit_stt_t;

extern telit_stt_t TelitState;

#define Telit_Ready()               ((TelitState.val&0b10011111)==0b10001110)
#define Telit_IsBusy()              (TelitState.busy==1)
#define Telit_PwrCtrl_GetState()    (TelitState)

void Telit_PwrCtrl_Init(void);
void Telit_PwrCtrl_Tasks(void*);
void Telit_TurnOn(bool dischrFirst, uint8_t Retry);
void Telit_TurnOff(void);
bool Telit_PwrCtrl_IsError(void);

#endif

