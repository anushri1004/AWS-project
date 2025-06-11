#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <ArduinoJson.h>

const char* ssid = "M3device";
const char* password = "1234567890";
const char* apiUrl = "https://lmb5146fo8.execute-api.ap-south-1.amazonaws.com/prod/files";  // API to get file list
const char* bucketUrl = "https://s3.ap-south-1.amazonaws.com/testsample.192/"; // S3 bucket base URL

void setup() {
  Serial.begin(115200);
  
  // Initialize WiFi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize the SD card
  if (!SD.begin(16)) {
    Serial.println("Card Mount Failed");
    return;
  }
  
  // Fetch and download the latest file
  fetchAndDownloadLatestFile();
}

void fetchAndDownloadLatestFile() {
  // Step 1: Fetch list of files from API
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(apiUrl);  // API endpoint
    int httpCode = http.GET();  // Send GET request

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Response:");
      Serial.println(payload);  // Print the JSON response

      // Step 2: Parse JSON and find the latest file
      String latestFile = getLatestFileName(payload);
      if (latestFile != "") {
        Serial.print("Latest file found: ");
        Serial.println(latestFile);

        // Step 3: Append file name to S3 base URL
        String fileUrl = String(bucketUrl) + latestFile;
        Serial.print("File URL: ");
        Serial.println(fileUrl);

        // Step 4: Download and save the file to SD card
        downloadFile(fileUrl, latestFile);
      } else {
        Serial.println("No valid file found.");
      }
    } else {
      Serial.printf("Failed to fetch files, HTTP code: %d\n", httpCode);
    }
    http.end();  // Close connection
  }
}

String getLatestFileName(String jsonResponse) {
  const size_t capacity = JSON_ARRAY_SIZE(10) + 10 * JSON_OBJECT_SIZE(3) + 1000;
  DynamicJsonDocument doc(capacity);

  // Parse JSON response
  DeserializationError error = deserializeJson(doc, jsonResponse);
  if (error) {
    Serial.print(F("Failed to parse JSON: "));
    Serial.println(error.f_str());
    return "";
  }

  // Step 2: Loop through the JSON to find the latest file based on date
  String latestFile = "";
  String latestDate = "0000-00-00 00:00:00+00:00";
  for (JsonObject file : doc.as<JsonArray>()) {
    String fileName = file["name"];
    String fileDate = file["last_modified"];
    
    // Compare dates to find the latest one
    if (fileDate > latestDate) {
      latestDate = fileDate;
      latestFile = fileName;
    }
  }

  // Replace spaces with '+' in the file name
  latestFile.replace(" ", "+");

  return latestFile;
}

void downloadFile(String fileUrl, String fileName) {
  // Step 4: Download the file from S3
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(fileUrl);  // URL to download the file
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      // Step 5: Write the file to the SD card
      File file = SD.open("/" + fileName, FILE_WRITE);
      if (!file) {
        Serial.println("Failed to open file for writing");
        return;
      }

      // Get the expected file size from headers
      int fileSize = http.getSize();
      Serial.print("Expected file size: ");
      Serial.println(fileSize);

      // Buffer size for reading in chunks
      const size_t bufferSize = 1024; // 1 KB buffer
      uint8_t buffer[bufferSize];

      // Timing the file transfer
      unsigned long startTime = millis();  // Start time
      Serial.println("Starting file download...");

      // Write content to the file
      WiFiClient *stream = http.getStreamPtr();
      size_t totalBytesReceived = 0;
      int lastProgress = 0; // Variable to track last displayed progress

      // Variables for speed and time calculation
      unsigned long lastUpdate = startTime;  // Time of last update
      float downloadSpeed = 0;               // Speed in KB/s
      int remainingTime = 0;                 // Remaining time in seconds

      while (http.connected() && totalBytesReceived < fileSize) {
        size_t size = stream->readBytes(buffer, sizeof(buffer));
        if (size > 0) {
          file.write(buffer, size);
          totalBytesReceived += size;

          // Calculate and print the download progress percentage
          int progress = (totalBytesReceived * 100) / fileSize;
          
          // Update display only if progress changes significantly
          if (progress != lastProgress) {
            // Calculate the elapsed time since start
            unsigned long elapsedTime = millis() - startTime;

            // Calculate download speed in KB/s (total bytes received / time taken in seconds)
            downloadSpeed = (totalBytesReceived / 1024.0) / (elapsedTime / 1000.0);

            // Estimate remaining time in seconds (remaining bytes / download speed)
            remainingTime = (fileSize - totalBytesReceived) / (downloadSpeed * 1024);

            // Display progress, speed, and estimated time
            Serial.print("\rDownloaded: "); // Use \r to overwrite the previous line
            Serial.print(progress);
            Serial.print("% | Speed: ");
            Serial.print(downloadSpeed);
            Serial.print(" KB/s | Time Left: ");
            Serial.print(remainingTime);
            Serial.print(" s   ");  // Additional spaces for cleanup

            lastProgress = progress; // Update last progress
          }
        } else {
          delay(100);  // If no data is available, wait briefly
        }
      }

      // Close the file after writing
      file.close();
      Serial.println("\nFile saved to SD card successfully");

      // Check file size after writing
      Serial.print("Actual file size after download: ");
      Serial.println(file.size());

      // Calculate and display time taken
      unsigned long totalElapsedTime = millis() - startTime;
      Serial.print("File download complete in ");
      Serial.print(totalElapsedTime / 1000.0);
      Serial.println(" seconds.");

      // Send the file over serial communication
      sendFileOverSerial(fileName);
    } else {
      Serial.printf("Failed to download file, HTTP code: %d\n", httpCode);
    }
    http.end();  // Close connection
  }
}

void sendFileOverSerial(String fileName) {
  // Open the file to send
  File file = SD.open("/" + fileName);  
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  // Send file size first
  Serial.print("File size: ");
  Serial.println(file.size());

  // Read and send file contents in chunks
  const size_t bufferSize = 1024; // Adjust the buffer size as needed
  uint8_t buffer[bufferSize];

  while (file.available()) {
    size_t bytesRead = file.read(buffer, sizeof(buffer)); // Read a chunk of data
    Serial.write(buffer, bytesRead);                     // Send the chunk over serial
  }
  
  file.close();
  Serial.println("File sent successfully.");
}

void loop() {
  // Nothing to do in loop
}
