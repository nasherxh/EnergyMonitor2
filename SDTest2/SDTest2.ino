/*********
  Rui Santos


  For the full code see: https://randomnerdtutorials.com/esp32-data-logging-temperature-to-microsd-card/
  This includes how to write more sensor data to the SD card, how to connect to wifi, how to send ESP32 into deep sleep mode.
*********/

// Libraries for SD card
#include "FS.h"
#include "SD.h"
#include <SPI.h>



// Define CS pin for the SD card module
#define SD_CS 5

int testValue=0; //test value for writing to SD card.
String dataMessage;


void setup() {
  // Start serial communication for debugging purposes
  Serial.begin(115200);



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
    writeFile(SD, "/data.txt", "TestValue \r\n");
  }
  else {
    Serial.println("File already exists");  
  }
  file.close();

  // Increment readingID on every new reading

}

void loop() {

  logSDCard();
  testValue = testValue+1;
  delay(1000);
  
}


// Write the sensor readings on the SD card
void logSDCard() {
  dataMessage = String(testValue) + "\r\n";
  Serial.print("Save data: ");
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
