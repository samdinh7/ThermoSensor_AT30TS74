#include "TelitAtCmd.h"
#include "Common/UserFunctions.h"
#include "System/TickTimer.h"

#ifdef USE_ATCMD_DEBUG
#include "Common/TinyPrintf.h"
#else
#define __dbs(...)
#define __dbc(...)
#define __dbh2(...)
#endif

/* ******************************************************* External variables */
#ifdef ATCMD_USE_DYNAMIC_PORT
atcmdport_cxt_t AtPort={NULL, NULL, NULL, NULL, NULL};
#endif
/* ********************************************************** Local variables */

// Response constants
const char RES_OK[]="\r\nOK\r\n";
const char RES_ERROR[]="\r\nERROR\r\n";

uint8_t AtCmdData[ATCMD_BUFFER_SIZE];
buff_t AtCmdRxBuff;

static size_t RxCount=0;
static uint8_t ReTry=0;
static uint8_t TestCount=0;
static uint8_t DoNext=0;
static tick_t Tdelay=250;
static tick_timer_t TickRaw={1, 0, 0};

void ATCMD_Init(void)
{
    ReTry=0;
    DoNext=0;
    RxCount=0;
    TestCount=0;
    Tdelay=250;
    AtCmdRxBuff.Len=0;
    AtCmdRxBuff.Size=ATCMD_BUFFER_SIZE;
    AtCmdRxBuff.pData=AtCmdData;
    Tick_Timer_Reset(TickRaw);
}

int8_t ATCMD_SendRaw(const uint8_t *pD, int sz, uint16_t Wait)
{
    // Get all response data in Rx buffer before send new data
    if(ATCMD_Port_IsRxReady())
    {
        ATCMD_Port_Read();

        if(Tick_Timer_Is_Over_Ms(TickRaw, Wait))
        {
            __dbs("TX timeout");
            return RESULT_ERR;
        }
        else
            return RESULT_BUSY;
    }

    __dbs("\nTX: ");
    Tick_Timer_Reset(TickRaw);
    RxCount=0;

    while(sz>0)
    {
        if(ATCMD_Port_IsTxReady())
        {
            if(is_printable(*pD))
                __dbc(*pD);
            else
            {
                __dbc('<');
                __dbh2(*pD);
                __dbc('>');
            }

            ATCMD_Port_Write(*pD++);
            sz--;
        }
    }

    while(!ATCMD_Port_IsTxDone());

    return RESULT_DONE;
}

int8_t ATCMD_GetRaw(uint8_t *pD, int *pSz, uint16_t firstWait, uint16_t lastWait)
{
    while(ATCMD_Port_IsRxReady())
    {
        uint8_t c=ATCMD_Port_Read();

        Tick_Timer_Reset(TickRaw);

        if(is_printable(c))
            __dbc(c);
        else
        {
            __dbc('<');
            __dbh2(c);
            __dbc('>');
        }

        if(RxCount>=ATCMD_BUFFER_SIZE)
            RxCount=0;

        pD[RxCount++]=c;
    }

    if(RxCount==0)
    {
        if(Tick_Timer_Is_Over_Ms(TickRaw, firstWait))
        {
            if(pSz!=NULL)
                *pSz=RxCount;

            pD[RxCount]=0x00;
            RxCount=0;

            __dbs("No response");
            return RESULT_ERR;
        }
    }
    else if(Tick_Timer_Is_Over_Ms(TickRaw, lastWait))
    {
        if(pSz!=NULL)
            *pSz=RxCount;

        pD[RxCount]=0x00;
        RxCount=0;

        return RESULT_DONE;
    }

    return RESULT_BUSY;
}

int8_t ATCMD_SendGetDat(const char *pTx, char *pRx, uint16_t firstWait, uint16_t lastWait)
{
    int8_t rslt=RESULT_BUSY;

    switch(DoNext)
    {
        default:
        case 3:
            if(Tick_Timer_Is_Over_Ms(TickRaw, Tdelay))
                DoNext=0;
            break;

        case 0:
            DoNext++;
            Tick_Timer_Reset(TickRaw);

        case 1:
            rslt=ATCMD_SendRaw((const uint8_t*) pTx, slen(pTx), 1000);

            if(rslt==RESULT_DONE)
            {
                DoNext++;
                rslt=RESULT_BUSY;
                __dbs("\nRX: ");
            }
            else if(rslt==RESULT_ERR)
                DoNext=0;
            break;

        case 2:
            rslt=ATCMD_GetRaw((uint8_t*) pRx, NULL, firstWait, lastWait);

            if(rslt!=RESULT_BUSY)
                DoNext=0;
            break;
    }

    return rslt;
}

int8_t ATCMD_SendGetAck(const char *pTx, const char *pAck, const char *pNAck, uint16_t firstWait, uint16_t lastWait, uint8_t tryCount)
{
    static size_t AckCount=0;
    static size_t NAckCount=0;
    int8_t rslt=RESULT_BUSY;

    switch(DoNext)
    {
        default:
        case 3:
            if(Tick_Timer_Is_Over_Ms(TickRaw, Tdelay))
                DoNext=0;
            break;

        case 0:
            DoNext++;
            AckCount=0;
            NAckCount=0;
            AtCmdRxBuff.Len=0;
            Tick_Timer_Reset(TickRaw);

        case 1:
            rslt=ATCMD_SendRaw((const uint8_t*) pTx, (int) slen(pTx), 1000);

            if(rslt==RESULT_DONE)
            {
                DoNext++;
                rslt=RESULT_BUSY;
                __dbs("\nRX: ");
            }
            else if(rslt==RESULT_ERR)
                DoNext=0;
            break;

        case 2:
            while(ATCMD_Port_IsRxReady())
            {
                char c=ATCMD_Port_Read();

                if(is_printable(c))
                    __dbc(c);
                else
                {
                    __dbc('<');
                    __dbh2(c);
                    __dbc('>');
                }

                AtCmdRxBuff.pData[AtCmdRxBuff.Len++]=c;
                AtCmdRxBuff.pData[AtCmdRxBuff.Len]=0;

                if((AtCmdRxBuff.Len+1)==AtCmdRxBuff.Size)
                    AtCmdRxBuff.Len=0;

                if(FindString(c, &AckCount, (const char *) pAck))
                    rslt=RESULT_DONE;

                if(rslt!=RESULT_DONE)
                {
                    if(FindString(c, &NAckCount, (const char *) pNAck))
                        rslt=RESULT_ERR;
                }

                if(c!=0x00)
                    Tick_Timer_Reset(TickRaw);
                else
                    break;
            }

            if(rslt!=RESULT_DONE)
            {
                if(AtCmdRxBuff.Len==0)
                {
                    if(Tick_Timer_Is_Over_Ms(TickRaw, firstWait))
                    {
                        if(++ReTry>=tryCount)
                        {
                            ReTry=0;
                            DoNext=0;
                            __dbs("RX timeout");
                            return RESULT_ERR;
                        }
                        else
                            DoNext=3;
                    }
                }
                else if(Tick_Timer_Is_Over_Ms(TickRaw, lastWait)||(rslt==RESULT_ERR))
                {
                    if(++ReTry>=tryCount)
                    {
                        ReTry=0;
                        DoNext=0;
                        __dbs("Not found");
                        return RESULT_ERR;
                    }
                    else
                    {
                        rslt=RESULT_BUSY;
                        DoNext=3;
                    }
                }
            }
            else
            {
                DoNext=0;
                ReTry=0;
            }
            break;
    }

    return rslt;
}

int8_t ATCMD_GetAck(const char *pAck, const char *pNAck, uint16_t firstWait, uint16_t lastWait, uint8_t tryCount)
{
    static size_t AckCount=0;
    static size_t NAckCount=0;
    int8_t rslt=RESULT_BUSY;

    switch(DoNext)
    {
        default:
        case 0:
            DoNext++;
            AckCount=0;
            NAckCount=0;
            AtCmdRxBuff.Len=0;
            Tick_Timer_Reset(TickRaw);

        case 2:
            while(ATCMD_Port_IsRxReady())
            {
                char c=ATCMD_Port_Read();

                __dbc(c);

                AtCmdRxBuff.pData[AtCmdRxBuff.Len++]=c;
                AtCmdRxBuff.pData[AtCmdRxBuff.Len]=0;

                if((AtCmdRxBuff.Len+1)==AtCmdRxBuff.Size)
                    AtCmdRxBuff.Len=0;

                if(FindString(c, &AckCount, (const char *) pAck))
                    rslt=RESULT_DONE;

                if(rslt!=RESULT_DONE)
                {
                    if(FindString(c, &NAckCount, (const char *) pNAck))
                        rslt=RESULT_ERR;
                }

                if(c!=0x00)
                    Tick_Timer_Reset(TickRaw);
                else
                    break;
            }

            if(rslt!=RESULT_DONE)
            {
                if(AtCmdRxBuff.Len==0)
                {
                    if(Tick_Timer_Is_Over_Ms(TickRaw, firstWait))
                    {
                        if(++ReTry>=tryCount)
                        {
                            ReTry=0;
                            DoNext=0;
                            __dbs("RX timeout");
                            return RESULT_ERR;
                        }
                        else
                            DoNext=3;
                    }
                }
                else if(Tick_Timer_Is_Over_Ms(TickRaw, lastWait)||(rslt==RESULT_ERR))
                {
                    if(++ReTry>=tryCount)
                    {
                        ReTry=0;
                        DoNext=0;
                        __dbs("Not found");
                        return RESULT_ERR;
                    }
                    else
                    {
                        rslt=RESULT_BUSY;
                        DoNext=3;
                    }
                }
            }
            else
            {
                DoNext=0;
                ReTry=0;
            }
            break;
    }

    return rslt;
}

int8_t ATCMD_Test(uint8_t tryCount)
{
    int8_t rslt;
    uint8_t type=tryCount&0xC0;

    tryCount&=0x3F;
    rslt=ATCMD_SendGetAck("AT\r", RES_OK, RES_ERROR, 250, 250, 1);

    switch(type)
    {
        default:
        case ALL_STATE_ON:
            if(rslt==RESULT_DONE)
            {
                if(++TestCount>=tryCount)
                {
                    TestCount=0;
                    return RESULT_DONE;
                }
            }
            else if(rslt==RESULT_ERR)
            {
                TestCount=0;
                return RESULT_ERR;
            }
            break;

        case ALL_STATE_OFF:
            if(rslt==RESULT_DONE)
            {
                TestCount=0;
                return RESULT_ERR;
            }
            else if(rslt==RESULT_ERR)
            {
                if(++TestCount>=tryCount)
                {
                    TestCount=0;
                    return RESULT_DONE;
                }
            }
            break;

        case AT_LEAST_1ON:
            if(rslt==RESULT_DONE)
            {
                TestCount=0;
                return RESULT_DONE;
            }
            else if(rslt==RESULT_ERR)
            {
                if(++TestCount>=tryCount)
                {
                    TestCount=0;
                    return RESULT_ERR;
                }
            }
            break;

        case AT_LEAST_1OFF:
            if(rslt==RESULT_DONE)
            {
                if(++TestCount>=tryCount)
                {
                    TestCount=0;
                    return RESULT_ERR;
                }
            }
            else if(rslt==RESULT_ERR)
            {
                TestCount=0;
                return RESULT_DONE;
            }
            break;
    }

    return RESULT_BUSY;
}

void ATCMD_Delay(uint16_t delayMs)
{
    DoNext=3;
    Tdelay=delayMs;
    Tick_Timer_Reset(TickRaw);
}