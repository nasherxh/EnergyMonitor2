/*****************
DC voltage measurement test
50:1 ratio potential divider to reduce voltage from 250V to 5V for sensing with adc.
Using MCP3008 adc chip: 10 bit and MCP3XXX library.
***********************/


#include <mcp3008.h>

mcp3008 mcp = mcp3008();


//for HSPI
const uint8_t _SS   = 13;//15;   // Pins for SPI interface
const uint8_t _MISO = 2;//12;     
const uint8_t _MOSI = 15;//13;   // example shows HSPI defaults. Two options.
const uint8_t _SCLK = 14;  


int DCVoltageSensorRawValue; //for saving DC voltage raw value in levels (0 to 1023, as is 10 bit)
float DCVoltage; //for saving DCvoltage reading
const int resistorRatio = 219;//11; //Potential divider ratio
const int Vref = 5; //reference voltage of adc
const int adcLevels = 1024; //No of possible levels for adc. In this case is 10 bit adc MCP3008 = 1024 levels
const int DCVoltageAnalogPin = 3;//2

void setup() {
  Serial.begin(115200); //open serial connection to print values to PC serial
 

  mcp.begin(_SCLK, _MISO, _MOSI, _SS);   
  
}

void loop() {

  getDCVoltage();
  Serial.print("adc1 level: ");
  Serial.print(DCVoltageSensorRawValue);
  Serial.print("\t");
  Serial.print("Calculated DC Voltage: ");  
  Serial.println(DCVoltage);

  delay(1000);
}

void getDCVoltage(){
  DCVoltageSensorRawValue = mcp.analogRead(DCVoltageAnalogPin);
  DCVoltage = (float)DCVoltageSensorRawValue*(float)Vref*(float)resistorRatio/(float)adcLevels;
}
