#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include "SPIFFS.h"

const char* ssid = "Lotevaodiem10";
const char* password = "10101010";

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
WebServer server(80);

Adafruit_MPU6050 mpu;
File file;
String fileContent = "";
char record[70];
bool start = false;

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);

  delay(100);
  while (!Serial)
    delay(10);
  Serial.println("Adafruit MPU6050 test!");

  delay(100);
  server.on("/", handle_OnConnect);
  server.on("/start", handle_start);
  server.on("/stop", handle_stop);
  server.on("/download", handle_download);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");

  Serial.print("Find MPU6050 chip: ");
  while (!mpu.begin()) {
    Serial.print(".");
    delay(10);
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  Serial.println(mpu.getAccelerometerRange());

  delay(100);

  Serial.print("Initialize SPIFFS: ");
  while (!SPIFFS.begin(true)) {
    Serial.print(".");
    delay(10);
  }
  Serial.println("SPIFFS Initialized");

  File root = SPIFFS.open("/");
  File filePrevious = root.openNextFile();
  while (filePrevious) {
    Serial.print("FILE: ");
    Serial.println(filePrevious.name());
    filePrevious = root.openNextFile();
  }
  Serial.println("All files deleted from SPIFFS");
}
void loop() {
  server.handleClient();
  if (start && file) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    sprintf(record, "%010d;%4.4f;%4.4f;%4.4f;%4.4f;%4.4f;%4.4f", millis(),
            a.acceleration.x, a.acceleration.y, a.acceleration.z,
            g.gyro.x , g.gyro.y , g.gyro.z );

    Serial.println(record);
    file.println(record);
    delay(5);
  }
}
void handle_OnConnect() {
  start = false;
  Serial.println("MPU Status: OFF");
  server.send(200, "text/html", sendHTML(start));
}
void handle_start() {
  start = true;
  fileContent = "";

  Serial.println("MPU Status: ON");
  server.send(200, "text/html", sendHTML(start));

  if (SPIFFS.exists("/data.txt")) {
    SPIFFS.remove("/data.txt");
    Serial.println("Removed data.txt file from SPIFFS");
  }

  file = SPIFFS.open("/data.txt", FILE_APPEND);
  if (!file) {
    Serial.println("Cannot create file");
    return;
  }
  Serial.println("File was written!");
  file.println("time;acc_x;acc_y;acc_z;gyro_x;gyro_y;gyro_z");
}

void handle_stop() {
  start = false;
  Serial.println("MPU Status: OFF");

  if (!file) {
    Serial.println("File error!");
    return;
  }
  file.close();
  server.send(200, "text/html", sendHTML(start));
}

void handle_download() {
  File download = SPIFFS.open("/data.txt", "r");
  if (download) {
    server.sendHeader("Content-Type", "text/text");
    server.sendHeader("Content-Disposition", "attachment; filename=data.txt");
    server.sendHeader("Connection", "close");
    server.streamFile(download, "application/octet-stream");
    download.close();
  } else {
    Serial.println("Download failed");
  }
}

void handle_NotFound() {
  server.send(404, "text/plain", "Page Not found");
}

String sendHTML(bool start) {
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>MPU Control</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr += ".button {display: block;width: 200px;background-color: #3498db;border: none;color: white;padding: 13px 0px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button-on {background-color: #00FF7F;}\n";
  ptr += ".button-on:active {background-color: #7CCD7C;}\n";
  ptr += ".button-off {background-color: #FF4040;}\n";
  ptr += ".button-off:active {background-color: #CD3333;}\n";
  ptr += ".button-download {background-color: #e9ca1c;}\n";
  ptr += ".button-download:hover {background-color: #795004;}\n";
  ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<h1>ESP32 Web Server</h1>\n";
  ptr += "<h3>Access Point - AP mode</h3>\n";

  if (start)  ptr += "<p>Status: Run</p><a class=\"button button-off\" href=\"/stop\">Stop</a>\n";
  else {
    ptr += "<p>Status: Stop</p><a class=\"button button-on\" href=\"/start\">Start</a>\n";
    ptr += "<p>Data</p><a class=\"button button-download\" href=\"/download\">Download</a>\n";
  }

  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}
