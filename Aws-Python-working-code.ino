#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <FS.h>

const char* ssid = "KavinArul";
const char* password = "arulkavin";
const char* bucketURL = "https://s3.ap-south-1.amazonaws.com/testsample.192/zip+file.zip";  /
#define SD_CS_PIN 16  

void setup() {
  Serial.begin(115200); 
  WiFi.begin(ssid, password);
  

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

 
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Card Mount Failed");
    return;
  }

  
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);  
  Serial.printf("SD Card Size: %lluMB\n", cardSize);


  downloadAndSaveFile();
}

void loop() {
  static bool fileSent = false;  
  if (!fileSent) {
    sendFileOverSerial("/downloaded.zip");
    fileSent = true;  
    Serial.println("File sent. Reset to send again.");  
  }

  delay(1000);
}


void downloadAndSaveFile() {
  HTTPClient http;
  http.begin(bucketURL);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    File zipFile = SD.open("/downloaded.zip", FILE_WRITE);
    if (!zipFile) {
      Serial.println("Failed to create file on SD card");
      return;
    }

   
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buff[128] = { 0 };
    int len;
    while ((len = stream->available()) > 0) {
      size_t size = stream->readBytes(buff, sizeof(buff));
      zipFile.write(buff, size);
    }

    
    zipFile.close();
    Serial.println("File downloaded and saved to SD card");

    if (!SD.exists("/downloaded.zip")) {
      Serial.println("File does not exist or permission error");
    } else {
      Serial.println("File saved successfully and permissions are OK");
    }
  } else {
    Serial.printf("Failed to download file, HTTP code: %d\n", httpCode);
  }

  http.end();
}


void sendFileOverSerial(const char* filePath) {
  File file = SD.open(filePath, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

 
  Serial.printf("File size: %d\n", file.size());  

 
  const size_t bufferSize = 512;
  byte buffer[bufferSize];
  size_t bytesRead;
  size_t totalBytesSent = 0;  

  while ((bytesRead = file.read(buffer, bufferSize)) > 0) {
    Serial.write(buffer, bytesRead);
    totalBytesSent += bytesRead;  
  }


  Serial.write("EOF");  

  
  file.close();
  Serial.println("File sent over serial");

  Serial.print("Total file size transferred: ");
  Serial.print(totalBytesSent);
  Serial.println(" bytes");
}
