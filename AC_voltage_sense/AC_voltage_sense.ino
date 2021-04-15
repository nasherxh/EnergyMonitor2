#include "EmonLibADC.h"


//set up AC current sensor
EnergyMonitor emon1;
const int ACVoltageAnalogPin = 7;
const int ACVoltageCalibration = 465;//301;

double Irms;

#include <mcp3008.h>
//initiate an instance of MCP3008
mcp3008 mcp = mcp3008();
const uint8_t _SS   = 13;//15;   // Pins for SPI interface
const uint8_t _MISO = 2;//12;     
const uint8_t _MOSI = 15;//13;   // example shows HSPI defaults. Two options.
const uint8_t _SCLK = 14;   

// Make a callback method for reading the pin value from the MCP instance
int MCP3008PinReader(int _pin){
  return mcp.analogRead(_pin);
}

void setup()
{
  Serial.begin(115200);
  emon1.inputPinReader = MCP3008PinReader; // Replace the default pin reader with the customized ads pin reader
  mcp.begin(_SCLK, _MISO, _MOSI, _SS);    
  emon1.current(1, 60.6); 
  emon1.voltage(ACVoltageAnalogPin, ACVoltageCalibration, 1.7);  // Voltage: input pin, calibration, phase_shift            
}

void loop()
{
  emon1.calcVI(20,2000);         // Calculate all. No.of wavelengths, time-out
  emon1.serialprint();           // Print out all variables
}
