/*********
  ESP32
  Connect to MCP3008 ADC chip via HSPI
  Connect to SD card via VSPI (IO23=MOSI, IO18=SCK,IO19=MISO,IO5=CS)
  Connect to Wifi to upload data to ThingSpeak Channel --> 
    Note that ThingSpeak has only 8 channels, and there is a rate limit of 1 reading every 15 seconds.
  Write data from MCP3008 to SD card.
  Get time from RTC Module

  Uses forked EmonLib library with callback method, enabling use of external adc
    Library fork available here: https://github.com/PaulWieland/EmonLib/tree/4a965d87061c19ad8b0a35bb173caada014ecbd9

  Remember to tie GPIO2 to GPIO0 if using for HSPI (to allow flashing ESP32)

  Note that if power is lost momentarily from SD card, it will fail for all subsequent appends
  ---> Need to find a way of restarting connection each time such that this is not an issue 
  (I think it is software-fixable as restarting the program fixes the problem)

*********/
// Libraries for SD card
  #include "FS.h"
  #include "SD.h"
  #include <SPI.h>
  // Define CS pin for the SD card module
  #define SD_CS 5
  // String for writing data to SD card and serial port
  String dataMessage;


// Libraries for WiFi
  #include <WiFi.h>
  //*** Replace with your network credentials ***********************************
  const char* ssid     = "VM4995142";
  const char* password = "t3smWnchwrvx";


//Variabls to save date and time for RTC
  char dateStamp[] = "YYYY-MM-DD";
  char timeStamp[] = "hh:mm:ss";
  
//Libraries for RTC Module
  #include "RTClib.h"
  RTC_DS3231 rtc;

// Save reading number on ESP32 RTC memory
  RTC_DATA_ATTR int readingID = 0;

//Library for MCP3008 ADC chip
  #include <mcp3008.h>
  //initiate an instance of MCP3008
  mcp3008 mcp = mcp3008();
  //Setup pins for ADC SPI interface. Example shows HSPI options*************************************************
  const uint8_t _SS   = 13;//15;
  const uint8_t _MISO = 2;//12;//Note that if using '2', must tie pins 2 and 0 together. If using 12, remove connection during code upload.
  const uint8_t _MOSI = 15;//13;
  const uint8_t _SCLK = 14;   

//Calculated Sensor Values (shown below in order of ADC pin assignment)
  float I_DC50;
  float I_DC300;
  float V_DC50;
  float V_DC1000;
  float I1_AC100;
  float I2_AC100;
  float I3_AC100;
  float V_AC500;

//Calculated Power Values ***note this information is not currently sent to thingspeak or recorded on the SD Card******
  //for future interest**********************
  float RealACPower1;
  float RealACPower2;
  float RealACPower3;
  float ApparentACPower1;
  float ApparentACPower2;
  float ApparentACPower3;
  float ACPowerFactor1;
  float ACPowerFactor2;
  float ACPowerFactor3;  

//ThingSpeak server settings
//REPLACE WITH YOUR THINGSPEAK CHANNEL NUMBER AND API KEY IF DIFFERENT*****************************
  #include <ThingSpeak.h>
  unsigned long myChannelNumber = 1139755;
  const char * myWriteAPIKey = "F9Q1ROVIZW8TLDYE";
  WiFiClient  client;

//Library for AC Voltage and Current calculations
  //note this is not the standard EmonLib as it has been adapted with a callback instead of directly reading an analog pin
  #include "EmonLibADC.h"

//Create instance of EmonLib EnergyMonitor Class for each phase
  EnergyMonitor emon1;
  EnergyMonitor emon2;
  EnergyMonitor emon3;
  const int V_AC500_Pin = 7;
  const int V_AC500_Calibration = 465;//301; *****May need tuning (Should be sensor voltage when have 1V on ADC)******************
  const int V_AC500_Phase = 0;//1.7; //*****May need tuning. Is phase shift due to sensor**************
  const int I1_AC100_Pin = 4;
  const int I2_AC100_Pin = 5;
  const int I3_AC100_Pin = 6;
  const float I_AC100_Calibration = 60.6; //********May need tuning (Should be sensor current when have 1V on ADC)****************
  

//Calibration/set-up values for DC Voltage
  const int V_DC50_Pin = 2;       //ADC Pin
  const int V_DC1000_Pin = 3;     //ADC Pin
  const int Vref = 5; //reference voltage of adc
  const int adcLevels = 1024;      //No of possible levels for adc. In this case is 10 bit adc MCP3008 = 1024 levels
 
  //*****Calibration values may need tuning (should be voltage when have 1V on ADC)***********************
  const int V_DC50_Calibration = 11; 
  const int V_DC1000_Calibration = 219;


//Calibration/set-up values for DC Current
  const int I_DC50_Pin = 0;       //ADC Pin
  const int I_DC300_Pin = 1;       //ADC Pin

  //*************Raw values input for calculating calibration constants m & x. May need tuning ******************
  int I_DC50_RawAt0A = 122;   //ADC reading at 0A
  int I_DC50_RawAt10A = 241;  //ADC reading at 10A
  const float m1= 10.0/(I_DC50_RawAt10A - I_DC50_RawAt0A); //gradient of mapping adc output (x) to current (y)
  const float c1= -m1*I_DC50_RawAt0A;// y-intercept for graph of current (y) vs adc output (x)

  int I_DC300_RawAt0A = 122;   //ADC reading at 0A
  int I_DC300_RawAt10A = 137;  //ADC reading at 10A ****note this hasn't been tested yet, and should be calibrated with higher values than 10A***
  const float m2= 10.0/(I_DC300_RawAt10A - I_DC300_RawAt0A); //gradient of mapping adc output (x) to current (y)
  const float c2= -m2*I_DC300_RawAt0A;// y-intercept for graph of current (y) vs adc output (x)

  
void setup() {
  // Start serial communication for debugging purposes
  Serial.begin(115200);

  //start MCP3008 connection
  mcp.begin(_SCLK, _MISO, _MOSI, _SS);   

  //Connect to Wifi
  WiFiSetup();

  //set Up SD card (check for card, write headings, prepare for writing values etc.)
  setUpSDCard();

  //Set up RTC with laptop time if first time using.
  setUpRTC();

  ThingSpeak.begin(client);  // Initialize ThingSpeak

  //Set up ADC sensors
  setUpACSensors();
}

void loop() {
  // Increment readingID on every new reading 
  readingID++;
  getReadings();
  getTimeStamp();
  logSDCard();
  updateThingSpeak();
  delay(5000);
}

// Function to update all ADC readings
void getReadings(){
  updateACValues();
  updateDCValues();
}

void WiFiSetup(){
    // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  
}

void setUpRTC(){
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  if (! rtc.lostPower()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}

// Function to get date and time from RTC module
void getTimeStamp() {

  //get time and date from RTC module
    rtc.now().toString(dateStamp);
    rtc.now().toString(timeStamp);
}

//set Up SD Card. Write data headings if new file.
void setUpSDCard(){
  // Initialize SD card
  SD.begin(SD_CS);  
  if(!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR - SD card initialization failed!");
    return;    // init failed
  }

  // If the data.txt file doesn't exist
  // Create a file on the SD card and write the data labels
  File file = SD.open("/data.txt");
  if(!file) {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/data.txt", "ReadingID, dayStamp, timeStamp, I_DC50, I_DC300, V_DC50, V_DC1000, I1_AC100, I2_AC100, I3_AC100, V_AC500 \r\n");
  }
  else {
    Serial.println("File already exists");  
  }
  file.close();
}

// Write the sensor readings on the SD card
void logSDCard() {
   SD.begin(SD_CS);  
  if(!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR - SD card initialization failed!");
    return;    // init failed
  } 
  dataMessage = String(readingID) + "," + String(dateStamp) + "," + String(timeStamp) + "," + 
         String(I_DC50)+ "," +String(I_DC300)+ ","+ String(V_DC50)+ "," +String(V_DC1000)+ "," + String(I1_AC100)+ 
        "," +String(I2_AC100)+ "," +String(I3_AC100)+ "," +String(V_AC500) + "\r\n";
  Serial.print("Saving data to SD Card: ");
  Serial.println(dataMessage);
  appendFile(SD, "/data.txt", dataMessage.c_str());
}

// Write to the SD card (DON'T MODIFY THIS FUNCTION)
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);
  
  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}


void updateThingSpeak(){
  //set what we want each ThingsSpeak fields and statuses to be updated to 
  ThingSpeak.setField(1,I_DC50);
  ThingSpeak.setField(2,I_DC300);
  ThingSpeak.setField(3,V_DC50);
  ThingSpeak.setField(4,V_DC1000);
  ThingSpeak.setField(5,I1_AC100);
  ThingSpeak.setField(6,I2_AC100);
  ThingSpeak.setField(7,I3_AC100);
  ThingSpeak.setField(8,V_AC500);
  ThingSpeak.setStatus("no dump load triggered");
  // write to the ThingSpeak channel
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200){
    Serial.println("ThingSpeak Channel update successful.");
   }
  else if(x == -401){
    Serial.println("Failed to write to ThingSpeak. Most probable cause is the rate limit of once every 15 seconds exceeded");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
}

// Callback method for reading the pin value from the MCP instance (needed for EmonLib) 
int MCP3008PinReader(int _pin){
  return mcp.analogRead(_pin);
}

void setUpACSensors(){
  emon1.inputPinReader = MCP3008PinReader; // Replace the default pin reader with the customized ADC pin reader
  emon2.inputPinReader = MCP3008PinReader;
  emon3.inputPinReader = MCP3008PinReader;
  
  emon1.voltage(V_AC500_Pin, V_AC500_Calibration, V_AC500_Phase); // Voltage: input pin, calibration, phase_shift
  emon1.current(I1_AC100_Pin, I_AC100_Calibration);               // Current: input pin, calibration.
  emon2.voltage(V_AC500_Pin, V_AC500_Calibration, V_AC500_Phase);  
  emon2.current(I2_AC100_Pin, I_AC100_Calibration);      
  emon3.voltage(V_AC500_Pin, V_AC500_Calibration, V_AC500_Phase);  
  emon3.current(I3_AC100_Pin, I_AC100_Calibration);       
}


void updateACValues(){
  emon1.calcVI(20,2000);       // Calculate all. No.of half wavelengths (crossings), time-out
  emon2.calcVI(20,2000);       
  emon3.calcVI(20,2000);       

  RealACPower1       = emon1.realPower;        //extract Real Power into variable
  ApparentACPower1   = emon1.apparentPower;    //extract Apparent Power into variable
  ACPowerFactor1     = emon1.powerFactor;      //extract Power Factor into Variable
  RealACPower2       = emon2.realPower;        
  ApparentACPower2   = emon2.apparentPower;    
  ACPowerFactor2     = emon2.powerFactor;      
  RealACPower3       = emon3.realPower;        
  ApparentACPower3   = emon3.apparentPower;    
  ACPowerFactor3     = emon3.powerFactor;      

  V_AC500            = emon1.Vrms;              //extract Vrms into Variable
  I1_AC100           = emon1.Irms;              //extract Irms into Variable
  I2_AC100           = emon2.Irms;      
  I3_AC100           = emon3.Irms;         
}

void updateDCValues(){
  //read ADC, convert to voltage, and scale by calibration factor
  V_DC50 = (mcp.analogRead(V_DC50_Pin))*(float)Vref*(float)V_DC50_Calibration/(float)adcLevels;
  V_DC1000 = (mcp.analogRead(V_DC1000_Pin))*(float)Vref*(float)V_DC1000_Calibration/(float)adcLevels;

  //read ADC, and scale using y= mx+c with calibration constants 'm' and 'c'
  I_DC50 = ((mcp.analogRead(I_DC50_Pin))*m1) +c1;
  I_DC300 = ((mcp.analogRead(I_DC300_Pin))*m2) +c2;
}
