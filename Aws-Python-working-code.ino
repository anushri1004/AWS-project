#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <FS.h>

const char* ssid = "KavinArul";
const char* password = "arulkavin";
const char* bucketURL = "https://s3.ap-south-1.amazonaws.com/testsample.192/zip+file.zip";  // Replace with your AWS S3 file URL

#define SD_CS_PIN 16  // Define CS pin for SD card

void setup() {
  Serial.begin(115200);  // Initialize serial communication at 115200 baud rate
  WiFi.begin(ssid, password);
  
  // Connect to Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Card Mount Failed");
    return;
  }

  // Check SD card size
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);  // Size in MB
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  // Automatically download the ZIP file from AWS and save to SD card
  downloadAndSaveFile();
}

void loop() {
  static bool fileSent = false;  // Static variable to keep track of sending status

  if (!fileSent) {
    sendFileOverSerial("/downloaded.zip");
    fileSent = true;  // Set the flag to true after sending
    Serial.println("File sent. Reset to send again.");  // Inform user to reset for another send
  }

  // Optionally, add a small delay to avoid overwhelming the serial output
  delay(1000);
}

// Download ZIP file from AWS S3 and save to SD card
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

    // Write downloaded data to SD card
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buff[128] = { 0 };
    int len;
    while ((len = stream->available()) > 0) {
      size_t size = stream->readBytes(buff, sizeof(buff));
      zipFile.write(buff, size);
    }

    // Ensure the file is properly closed after writing
    zipFile.close();
    Serial.println("File downloaded and saved to SD card");

    // Ensure the file has read/write permissions
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

// Send file data over serial communication and display the size of the file transferred
void sendFileOverSerial(const char* filePath) {
  File file = SD.open(filePath, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  // Inform Python about the file size before sending
  Serial.printf("File size: %d\n", file.size());  // Inform Python about file size

  // Send file data in chunks and track total bytes sent
  const size_t bufferSize = 512;
  byte buffer[bufferSize];
  size_t bytesRead;
  size_t totalBytesSent = 0;  // Variable to accumulate total bytes sent

  while ((bytesRead = file.read(buffer, bufferSize)) > 0) {
    Serial.write(buffer, bytesRead);
    totalBytesSent += bytesRead;  // Accumulate total bytes sent
  }

  // Send an end-of-file marker
  Serial.write("EOF");  // This marks the end of the file transfer

  // Ensure the file is properly closed
  file.close();
  Serial.println("File sent over serial");

  // Print total file size sent
  Serial.print("Total file size transferred: ");
  Serial.print(totalBytesSent);
  Serial.println(" bytes");
}
