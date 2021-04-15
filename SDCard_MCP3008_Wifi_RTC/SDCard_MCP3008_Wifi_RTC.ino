/*********
  ESP32
  Connect to MCP3008 via HSPI
  Connect to SD card via VSPI
  Connect to Wifi so could get time from NTPClient
    Note that the NTPClient is not the standard Arduino Library but a fork of it from here:
    https://github.com/taranais/NTPClient
    The Arduino library does not have timeClient.getFormattedDate();
  Write data from MCP3008 to SD card.
  Get time from RTC Module
  Write to ThingSpeak --> Note that ThingSpeak has only 8 channels, and there is a rate limit of 1 reading every 15 seconds.

  See following to see how to integrate deep sleep:
  https://randomnerdtutorials.com/esp32-data-logging-temperature-to-microsd-card/

  Remember to unplug GPIO 12 when first starting if using for HSPI (to allow flashing ESP32)
  Remember to tie GPIO2 to GPIO0 if using for HSPI (to allow flashing ESP32)

  Note that if power is lost momentarily from SD card, it will fail for all subsequent appends
  ---> Need to find a way of restarting connection each time such that this is not an issue 
  (I think it is software-fixable)

*********/

// Libraries for SD card
  #include "FS.h"
  #include "SD.h"
  #include <SPI.h>
  // Define CS pin for the SD card module
  #define SD_CS 5
  // String for writing data to SD card and serial port
  String dataMessage;



// Libraries for WiFi and to get time from NTP Server
  #include <WiFi.h>
  #include <NTPClient.h>
  #include <WiFiUdp.h>
  // Replace with your network credentials
  const char* ssid     = "PIE";
  const char* password = "bagofchips7";

// Define NTP Client to get time
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP);

// Variables to save date and time for using NTP
//  String formattedDate;
//  String dayStamp;
//  String timeStamp;

//Variabls to save date and time for RTC (note that the two cannot be run simultaneously at the moment)
  char dateStamp[] = "YYYY-MM-DD";
  char timeStamp[] = "hh:mm:ss";
  
//Libraries for RTC Module
  #include "RTClib.h"
  //  RTC_DS3231 rtc;
  RTC_DS1307 rtc;

// Save reading number on ESP32 RTC memory
  RTC_DATA_ATTR int readingID = 0;

//Llibrary for MCP3008 ADC chip
  #include <mcp3008.h>
  //initiate an instance of MCP3008
  mcp3008 mcp = mcp3008();
  const uint8_t _SS   = 13;//15;   // Pins for SPI interface
  const uint8_t _MISO = 2;//12;     
  const uint8_t _MOSI = 15;//13;   // example shows HSPI defaults. Two options.
  const uint8_t _SCLK = 14;   

// Sensor variables
  uint16_t ADC0;
  uint16_t ADC1;
  uint16_t ADC2;
  uint16_t ADC3;
  uint16_t ADC4;
  uint16_t ADC5;
  uint16_t ADC6;
  uint16_t ADC7;

//ThingSpeak server settings
  #include <ThingSpeak.h>
  unsigned long myChannelNumber = 1139755;
  const char * myWriteAPIKey = "F9Q1ROVIZW8TLDYE";
  WiFiClient  client;

void setup() {
  // Start serial communication for debugging purposes
  Serial.begin(115200);

  //start MCP3008 connection
  mcp.begin(_SCLK, _MISO, _MOSI, _SS);   

  //Connect to Wifi
  WiFiSetup();

  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(0);

  //set Up SD card to start writing values
  setUpSDCard();
  setUpRTC();

  ThingSpeak.begin(client);  // Initialize ThingSpeak
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
  ADC0 = mcp.analogRead(0);
  ADC1 = mcp.analogRead(1);
  ADC2 = mcp.analogRead(2);
  ADC3 = mcp.analogRead(3);
  ADC4 = mcp.analogRead(4);
  ADC5 = mcp.analogRead(5);
  ADC6 = mcp.analogRead(6);
  ADC7 = mcp.analogRead(7);
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

//  if (! rtc.isrunning()) {
//    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
//  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
}

// Function to get date and time from NTPClient or RTC module
void getTimeStamp() {

  //get time and date from RTC module
    rtc.now().toString(dateStamp);
    rtc.now().toString(timeStamp);

// //get time and date from NTPClient
//
//  while(!timeClient.update()) {
//    timeClient.forceUpdate();
//  }
//  // The formattedDate comes with the following format:
//  // 2018-05-28T16:00:13Z
//  // We need to extract date and time
//  formattedDate = timeClient.getFormattedDate();
//  Serial.println(formattedDate);
//
//  // Extract date
//  int splitT = formattedDate.indexOf("T");
//  dateStamp = formattedDate.substring(0, splitT);
//  Serial.println(dateStamp);
//  // Extract time
//  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
//  Serial.println(timeStamp);
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
    writeFile(SD, "/data.txt", "ReadingID, dayStamp, timeStamp, ADC0, ADC1, ADC2, ADC3, ADC4, ADC5, ADC6, ADC7 \r\n");
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
         String(ADC0)+ "," +String(ADC1)+ ","+ String(ADC2)+ "," +String(ADC3)+ "," + String(ADC4)+ 
        "," +String(ADC5)+ "," +String(ADC6)+ "," +String(ADC7) + "\r\n";
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
  ThingSpeak.setField(1,ADC0);
  ThingSpeak.setField(2,ADC1);
  ThingSpeak.setField(3,ADC2);
  ThingSpeak.setField(4,ADC3);
  ThingSpeak.setField(5,ADC4);
  ThingSpeak.setField(6,ADC5);
  ThingSpeak.setField(7,ADC6);
  ThingSpeak.setField(8,ADC7);
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
