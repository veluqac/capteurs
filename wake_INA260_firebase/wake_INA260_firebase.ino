#include <WiFi.h>
#include <esp_sleep.h>
#include <Wire.h>
#include <Adafruit_INA260.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <esp_wpa2.h> // Needed for WPA2 Enterprise
#include <Firebase_ESP_Client.h>
#include <NTPClient.h>

#include <credentials.h>

#define INA260_ALERT_POWER 0b00100000  // Bit 5: Power alert

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

const int ledPin = 2;
Adafruit_INA260 ina260 = Adafruit_INA260();

// Power tracking
unsigned long startTime = 0;
float cumulativePower = 0;
int sampleCount = 0;

void connectToWPA2() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);

  esp_wifi_sta_wpa2_ent_enable();
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));

  WiFi.begin(WIFI_SSID);

  Serial.print("Connecting to WPA2 Enterprise WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected.");
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  connectToWPA2();

  // Firebase Config
  config.api_key = FIREBASE_API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = FIREBASE_EMAIL;
  auth.user.password = FIREBASE_PASSWORD;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // NTP
  timeClient.begin();
  
  while (!Serial) { delay(10); }

  Serial.println("Adafruit INA260 Test");

  if (!ina260.begin()) {
    Serial.println("Couldn't find INA260 chip");
    while (1);
  }
  Serial.println("Found INA260 chip");

  ina260.setAlertType(INA260_ALERT_POWER);   // Alert triggers on power condition
  ina260.setAlertLimit(100.0);               // Trigger if power < 100 mW

  // Configure wake from GPIO 33
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1); // Wake if GPIO 33 goes LOW

  startTime = millis();
}

void loop() {
  timeClient.update();
  String timestamp = String(timeClient.getEpochTime());

  float current = ina260.readCurrent();
  float voltage = ina260.readBusVoltage();
  float power = ina260.readPower(); // In mW

  Serial.println("Uploading values...");
  
  Firebase.RTDB.setInt(&fbdo, "/Current/" + timestamp, current);
  Firebase.RTDB.setInt(&fbdo, "/Voltage/" + timestamp, voltage);
  Firebase.RTDB.setInt(&fbdo, "/Power/" + timestamp, power);

  // Track cumulative power
  cumulativePower += power;
  sampleCount++;

  unsigned long elapsedTime = millis() - startTime;

  if (elapsedTime >= 2 * 60 * 1000) { // 2 minutes
    float avgPower = cumulativePower / sampleCount;
    Serial.print("Average power over 2 min: ");
    Serial.print(avgPower);
    Serial.println(" mW");

    if (avgPower <= 100.0) {
      Serial.println("Low power detected. Entering deep sleep...");
      delay(100); // Ensure messages go out
      esp_deep_sleep_start(); // Wake only by reset
    }

    // Reset tracking
    cumulativePower = 0;
    sampleCount = 0;
    startTime = millis();
  }

  delay(5000); // Delay between samples
}
