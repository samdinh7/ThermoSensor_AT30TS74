#include "system.h"
#include "mcc.h"
#include "i2c_simple_master.h"
#include "cfg/Project_Cfg.h"
#include "stdio.h"
#include "System/TickTimer.h"

#define Thermo_ADD 0x48
#define Temp_REG 0x00
#define Confg_REG 0x01
#define TLow_REG 0x02
#define THigh_REG 0x03
uint16_t Thermo_ADC;
int dem = 0;
uint16_t Confg_REG_read, THigh_REG_read, TLow_REG_read;

float Real_Temp = 0;
 // <editor-fold defaultstate="collapsed" desc="AT30TS74 thermo sensor datasheet">
/*The AT30TS74 thermo sensor datasheet is read from:
  https://ww1.microchip.com/downloads/en/DeviceDoc/Web_AT30TS74_0217.pdf
 *Important page:
    
    16,17: how to use pointer register
    9: address pins 
    10: High-Speed Mode
    12: Fault Tolerance Limits graph
    13: Comparator Mode graph
    14: interrupt Mode graph
 * chu y: o interrupt mode thi chan Alert se active khi du dieu kien fault tolerence va se inactive khi bat ki mot thanh ghi nao duoc
    19: read data temp
    20: Configuration Reg
    22: config reg frame
    23: TLow and THigh// dung de dieu khien chan int cua sensor, neu gia tri cua sensor doc duoc ngoai khoang Tlow high thi sensor ve sleep mode
    15: one shot mode, shut down mode
 */// </editor-fold>
void Thermo_Sensor_Init(void) {
    i2c_write1ByteRegister(Thermo_ADD, Confg_REG, 0b01101011); //0b01100000// mode not use t high low
    // <editor-fold defaultstate="collapsed" desc="Configuration reg">
    /* Configuration reg 0x01 //, only the first 8-bits of the register are actually used while the least-significant 8-bits are reserved for future
     15: 0 Normal Operation
     14:13: 11 12-bits resolution: 0.0625 degree C
     12:11: 01 Alarm after 2 Fault
     10: 0 ALERT pin is Active Low (Default)
     9: 1 interrupt Mode 
     8: 1 Temperature Sensor Disabled and Device In Shutdown Mode
     * Different modes:
      0b01100000: read temp only mode
      0b01101100: comparator mode with Alarm after 2 Fault
      0b01101010: interrupt mode with Alarm after 2 Fault
      0b01101011: OS mode
     */
    //    i2c_write2ByteRegister(Thermo_ADD, TLow_REG, 0x1B);//27
    //    i2c_write2ByteRegister(Thermo_ADD, THigh_REG, 0x22);//33
    // </editor-fold>
    Thermo_ADC = i2c_read2ByteRegister(Thermo_ADD, Temp_REG);
}

void DataPlot(float Thermo_Float) {
    uint8_t *Thermo_8bit = (uint8_t*) & Thermo_Float;

    UART3_Write(0x03);
    //    UART3_Write((uint8_t)Thermo_ADC);
    //    UART3_Write((uint8_t) (Thermo_ADC >> 8));
    UART3_Write(Thermo_8bit[0]);
    UART3_Write(Thermo_8bit[1]);
    UART3_Write(Thermo_8bit[2]);
    UART3_Write(Thermo_8bit[3]);
    UART3_Write(0xFC);
}

int main(void) {
    tick_timer_t Tick;
    SYSTEM_Initialize();
    Tick_Timer_Init();
    Thermo_Sensor_Init();
    delay_ms(300);
    LED2_SetHigh();
    printf("\nhello\n");
    Tick_Timer_Reset(Tick);
    while (1) {
        ClrWdt();
        //OS mode
        if (Tick_Timer_Is_Over_Ms(Tick, 100)) {
            if (dem == 5) {
                i2c_write1ByteRegister(Thermo_ADD, Confg_REG, 0b11101011); //OS bit is 1, activate the device
//                printf("Thermo sensor just update value\n");
                Thermo_ADC = i2c_read2ByteRegister(Thermo_ADD, Temp_REG);
                dem = 0;
            } else {
                Thermo_ADC = i2c_read2ByteRegister(Thermo_ADD, Temp_REG);
                Real_Temp = (float) Thermo_ADC / 256; //0.0625 is a resolution of the sensor//float 32 bit double 64bit
                DataPlot(Real_Temp);
                dem++;
            }
            //            printf("TEM_INT_GetValue: %d\n", TEM_INT_GetValue());//read value INT pin
            
//             <editor-fold defaultstate="collapsed" desc="Normal mode">
//            uint16_t Confg_REG_read, THigh_REG_read, TLow_REG_read;
//            double Real_Temp=0;
//            Thermo_ADC = i2c_read2ByteRegister(Thermo_ADD, Temp_REG);
//            Real_Temp = (double)Thermo_ADC/256; //0.0625 is a resolution of the sensor
//            printf("Temperature: %.2f\n", Real_Temp);     
//             </editor-fold>
            
//            <editor-fold defaultstate="collapsed" desc="Comparator mode">  
//                Thermo_ADC = i2c_read2ByteRegister(Thermo_ADD, Temp_REG);
//                THigh_REG_read = (double)i2c_read2ByteRegister(Thermo_ADD, THigh_REG)/256;
//                TLow_REG_read = (double)i2c_read2ByteRegister(Thermo_ADD, TLow_REG)/256;
//                Real_Temp = (double)Thermo_ADC/256; //0.0625 is a resolution of the sensor
//                printf("TLow THigh: %d - %d\n", TLow_REG_read, THigh_REG_read);
//                printf("Temperature: %.2f\n", Real_Temp);      
//             </editor-fold>
                       
//            <editor-fold defaultstate="collapsed" desc="Interrupt mode">      
//            if(TEM_INT_GetValue()==0){ 
//                Thermo_ADC = i2c_read2ByteRegister(Thermo_ADD, Temp_REG);
//                THigh_REG_read = (double)i2c_read2ByteRegister(Thermo_ADD, THigh_REG)/256;
//                TLow_REG_read = (double)i2c_read2ByteRegister(Thermo_ADD, TLow_REG)/256;
//                Real_Temp = (double)Thermo_ADC/256; //0.0625 is a resolution of the sensor
//                printf("TLow THigh: %d - %d\n", TLow_REG_read, THigh_REG_read);
//                printf("Temperature: %.2f\n", Real_Temp);               
//           } </editor-fold>
               
            LED2_Toggle();
        }

    }
    return 1;
}





















