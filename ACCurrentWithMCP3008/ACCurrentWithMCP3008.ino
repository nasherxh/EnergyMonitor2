/*
 *  Sketch to test AC split core current sensor
 *  In this instance, we are using the SCT1013 100A/50mA sensor.
 *  Read input via MCP3008 adc chip, as onboard adc is nonlinear
 */

#include "EmonLiteESP.h" //Include the EmonLite library
#include <Arduino.h> //think the code for the EmonLite might need this. Not totally clear.
EmonLiteESP power; //create instance of EmonLiteESP


#include <MCP3XXX.h> //Include library for MCP3XXX adc chip using SPI connection
MCP3008 adc; //create instance of the adc chip
const int ADC_CHANNEL=1; //Channel being used on ADC chip


const int ADC_BITS=10;
const int REFERENCE_VOLTAGE=5; //Vref in ADC
const int CURRENT_RATIO=20; //the value in amps for a 1V output.
const int NUMBER_OF_SAMPLES=1000;

double ACCurrent = 0; //initialise variable for AC current

void setup()
{
  Serial.begin(115200); //open serial connection to print values to PC serial

  // Use the default SPI hardware interface for the adc chip
  adc.begin();

  //Configure power object
  power.initCurrent(currentCallback, ADC_BITS, REFERENCE_VOLTAGE, CURRENT_RATIO);

}

void loop()
{
ACCurrent = power.getCurrent(NUMBER_OF_SAMPLES);
Serial.println(ACCurrent);
delay(1000);

}

/*callback functioin to retrieve the ADC value for EmonLiteESP*/
unsigned int currentCallback() {
   return adc.analogRead(ADC_CHANNEL);
}
