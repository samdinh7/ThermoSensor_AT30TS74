#include "SST25xx.h"

#ifdef USE_SST25XX_DEBUG
#include "Common/TinyPrintf.h"
#else
#define __dbs(...)
#define __dbh2(...)
#endif

#define SST_Make_Address(address) do{sstCMD[1]=(uint8_t) (address>>16); sstCMD[2]=(uint8_t) (address>>8); sstCMD[3]=(uint8_t) address;}while(0)

private uint8_t sstCMD[6];

/* [Chip select On/Off=1/0][data buffer][write length][read length][Chip select On/Off=1/0] */
private void SPI_Xfer(bool enCS, uint8_t *data, uint32_t lenIn, uint32_t lenOut, bool dnCS)
{
    uint32_t i;

    __dbs("\nSST Xfer: ");

    if(enCS)
    {
        FLASH_SPI_Open();
        FLASH_CS_N_SetLow();
    }

    for(i=0; i<lenIn; i++)
    {
        if(i==0)
            __dbs("\n   Write:");

        __dbh2(data[i]);
        FLASH_SPI_ExchangeByte(data[i]);
    }

    for(i=0; i<lenOut; i++)
    {
        if(i==0)
            __dbs("\n   Read:");

        data[i]=FLASH_SPI_ExchangeByte(0xFF);
        __dbh2(data[i]);
    }

    if(dnCS)
    {
        FLASH_CS_N_SetHigh();
        FLASH_SPI_Close();
    }
}

private void SST_Enable_Write(void)// Private function
{
    sstCMD[0]=SST_WRITE_EN;
    SPI_Xfer(1, sstCMD, 1, 0, 1);
}

private void SST_Disable_Write(void)// Private function
{
    sstCMD[0]=SST_WRITE_DIS;
    SPI_Xfer(1, sstCMD, 1, 0, 1);
}

private void SST_Poll_WR_Status(void)// Private function
{
    sstCMD[0]=SST_POLL_WR_STATUS;
    SPI_Xfer(1, sstCMD, 1, 0, 1);
}

private uint8_t SST_Read_Status(void)// Private function
{
    sstCMD[0]=SST_READ_STATUS;
    SPI_Xfer(1, sstCMD, 1, 1, 1);

    return sstCMD[0];
}

private void SST_Write_Status(uint8_t stt)// Private function
{
    sstCMD[0]=SST_WRITE_STATUS_EN;
    SPI_Xfer(1, sstCMD, 1, 0, 1);
    sstCMD[0]=SST_WRITE_STATUS;
    sstCMD[1]=stt;
    SPI_Xfer(1, sstCMD, 2, 0, 1);
}

private void SST_Check_Busy(void)// Private function
{
    while(SST_Read_Status()&0x01);
}

public void SST_Chip_Erase(void)// Public function
{
    SST_Check_Busy();
    SST_Enable_Write();
    sstCMD[0]=SST_BULK_ERASE;
    SPI_Xfer(1, sstCMD, 1, 0, 1);
    SST_Check_Busy();
    //__delay_ms(100);
}

public void SST_Sector_Erase(uint32_t BAddr)// Public function
{
    SST_Check_Busy();
    SST_Enable_Write();
    sstCMD[0]=SST_SECTOR_ERASE;
    SST_Make_Address(BAddr);
    SPI_Xfer(1, sstCMD, 4, 0, 1);
    SST_Check_Busy();
    //__delay_ms(50);
}

public void SST_Read_nByte(uint32_t BAddr, uint32_t len, uint8_t *buffer)// Public function
{
    SST_Check_Busy();
    sstCMD[0]=SST_FAST_READ_DATA;
    SST_Make_Address(BAddr);
    sstCMD[4]=0x00;
    SPI_Xfer(1, sstCMD, 5, 0, 0);
    SPI_Xfer(0, buffer, 0, len, 1);
}

public void SST_Write_Byte(uint32_t Addr, uint8_t data)// Public function
{
    SST_Check_Busy();
    SST_Enable_Write();
    sstCMD[0]=SST_BYTE_PROG;
    SST_Make_Address(Addr);
    sstCMD[4]=data;
    SPI_Xfer(1, sstCMD, 5, 0, 1);
    SST_Check_Busy();
    SST_Disable_Write();
    //__delay_us(20);
}

public void SST_Write_nByte(uint32_t BAddr, uint32_t len, const uint8_t *data)// Public function
{
    uint32_t i;

#ifdef SST_USE_AAI
    // Auto Address Increment (AAI) Word-Program
    SST_Check_Busy();
    SST_Enable_Write();
    sstCMD[0]=SST_WORD_PROG;
    SST_Make_Address(BAddr);
    sstCMD[4]=data[0];
    sstCMD[5]=data[1];
    SPI_Xfer(1, sstCMD, 6, 0, 1);
    SST_Check_Busy();

    for(i=2; i<len; i+=2)
    {
        sstCMD[0]=SST_WORD_PROG;
        sstCMD[1]=data[i];
        sstCMD[2]=data[i+1];
        SPI_Xfer(1, sstCMD, 3, 0, 1);
        SST_Check_Busy();
    }

    SST_Disable_Write();
#else
    for(i=0; i<len; i++)
        SST_Write_Byte(BAddr+i, *data++);
#endif
}

public uint8_t SST_Read_Byte(uint32_t Addr)
{
    uint8_t b;

    SST_Check_Busy();
    sstCMD[0]=SST_READ_DATA;
    SST_Make_Address(Addr);
    sstCMD[4]=0x00;
    SPI_Xfer(1, sstCMD, 5, 0, 0);
    SPI_Xfer(0, &b, 0, 1, 1);

    return b;
}

public void SST_Protect(sst_protect_range_t range)// Private function
{
    SST_Enable_Write();
    sstCMD[0]=SST_WRITE_STATUS;
    sstCMD[1]=FLASH_PROTECT_UNLOCK;
    SPI_Xfer(1, sstCMD, 2, 0, 1);
    SST_Enable_Write();
    sstCMD[0]=range|FLASH_PROTECT_LOCK;
    SPI_Xfer(1, sstCMD, 1, 0, 1);
}

public bool SST_Init(void)// Public function
{
    uint8_t tryTimes=0;
    uint32_t ReadID;
    uint32_t JEDEC_ID=(JEDEC_ID_BYTE0<<16)|(JEDEC_ID_BYTE1<<8)|(JEDEC_ID_BYTE2);

    __dbs("\nSST init: ");

LOOP:
    sstCMD[0]=SST_READ_JDECID;
    SPI_Xfer(1, sstCMD, 1, 3, 1);
    ReadID=sstCMD[0];
    ReadID<<=8;
    ReadID|=sstCMD[1];
    ReadID<<=8;
    ReadID|=sstCMD[2];

    if((ReadID&0xFFFF00)==(JEDEC_ID&0xFFFF00))
    {
        __dbh2(sstCMD[0]);
        __dbh2(sstCMD[1]);
        __dbh2(sstCMD[2]);

        if(ReadID!=JEDEC_ID)
            __dbs("\n   ->Incorrect memory size");
    }
    else
    {
        if(++tryTimes==10)
        {
            __dbh2(sstCMD[0]);
            __dbh2(sstCMD[1]);
            __dbh2(sstCMD[2]);
            return 0; // Try 256 times after return error
        }

        goto LOOP;
    }

    SST_Write_Status(0);

    return 1;
}