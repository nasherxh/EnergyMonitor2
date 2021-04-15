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


int DCCurrentSensorRawValue; //for saving DC voltage raw value in levels (0 to 1023, as is 10 bit)
float DCCurrent; //for saving DCvoltage reading
const int Vref = 5; //reference voltage of adc
const int adcLevels = 1024; //No of possible levels for adc. In this case is 10 bit adc MCP3008 = 1024 levels
int adcAt0A = 122;
int adcAt10A = 241;
const float m= 10.0/(adcAt10A - adcAt0A); //gradient of mapping adc input (x) to current (y)
const float c= -m*adcAt0A;// y-intercept for graph of current (y) vs adc input (x)

void setup() {
  Serial.begin(115200); //open serial connection to print values to PC serial
 

  mcp.begin(_SCLK, _MISO, _MOSI, _SS);   
  
}

void loop() {

 getDCCurrent();
  Serial.print("adc level: ");
  Serial.print(DCCurrentSensorRawValue);
  Serial.print("\t");
  Serial.print("Calculated DC Current: ");  
  Serial.println(DCCurrent);

  delay(1000);
}

void getDCCurrent(){
  DCCurrentSensorRawValue = mcp.analogRead(0);
  DCCurrent = ((float)DCCurrentSensorRawValue*m) +c;
}
