#ifndef TELITATCMD_H
#define TELITATCMD_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef USE_PROJECT_CFG
#include "Project_Cfg.h"
#else
#include "TelitAtCmd_Cfg.h"
#endif

typedef struct {
    bool (*IsRxReady)(void);
    bool (*IsTxReady)(void);
    bool (*IsTxDone)(void);
    void (*Write)(uint8_t);
    uint8_t(*Read)(void);
} atcmdport_cxt_t;

typedef struct {
    uint8_t *pData;
    int Len;
    int Size;
} buff_t;

#ifndef ATCMD_BUFFER_SIZE
#define ATCMD_BUFFER_SIZE   128
#ifndef DISABLE_LIBRARY_WARNING
#warning "Default AtCmdData size is 128"
#endif
#endif

/* ******************************************************* External variables */
extern uint8_t AtCmdData[ATCMD_BUFFER_SIZE];
extern buff_t AtCmdRxBuff;
// Response constants
extern const char RES_OK[];
extern const char RES_ERROR[];

#ifdef ATCMD_USE_DYNAMIC_PORT
extern atcmdport_cxt_t AtPort;

#define ATCMD_Port_IsRxReady()      AtPort.IsRxReady()
#define ATCMD_Port_Read()           AtPort.Read()
#define ATCMD_Port_IsTxReady()      AtPort.IsTxReady()
#define ATCMD_Port_Write(x)         AtPort.Write(x)
#define ATCMD_Port_IsTxDone()       AtPort.IsTxDone()

#define ATCMD_Port_SetRxReadyFnc(x) AtPort.IsRxReady=x
#define ATCMD_Port_SetReadFnc(x)    AtPort.Read=x
#define ATCMD_Port_SetTxReadyFnc(x) AtPort.IsTxReady=x
#define ATCMD_Port_SetWriteFnc(x)   AtPort.Write=x
#define ATCMD_Port_SetTxDoneFnc(x)  AtPort.IsTxDone=x
#endif

void ATCMD_Init(void);
void ATCMD_Delay(uint16_t delayMs);
int8_t ATCMD_SendRaw(const uint8_t *pD, int sz, uint16_t Wait);
int8_t ATCMD_GetRaw(uint8_t *pD, int *pSz, uint16_t firstWait, uint16_t lastWait);
int8_t ATCMD_SendGetDat(const char *pTx, char *pRx, uint16_t firstWait, uint16_t lastWait);
int8_t ATCMD_SendGetAck(const char *pTx, const char *pAck, const char *pNAck, uint16_t firstWait, uint16_t lastWait, uint8_t tryCount);
int8_t ATCMD_GetAck(const char *pAck, const char *pNAck, uint16_t firstWait, uint16_t lastWait, uint8_t tryCount);

int8_t ATCMD_Test(uint8_t tryCount); // Max 63 time
// tryCount OR (|) options
#define ALL_STATE_ON    0x00
#define ALL_STATE_OFF   0x40
#define AT_LEAST_1ON    0x80
#define AT_LEAST_1OFF   0xC0

#define ATCMD_GetRxBuffer(idx)  AtCmdRxBuff.pData[idx]
#define ATCMD_GetRxSize()       AtCmdRxBuff.Size
#define ATCMD_GetRxLen()        AtCmdRxBuff.Len

#endif