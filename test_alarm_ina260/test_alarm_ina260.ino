#include <WiFi.h>
#include <Wire.h>
#include <esp_wpa2.h> // Needed for WPA2 Enterprise
#include <Firebase_ESP_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_INA260.h>

#include <credentials.h>

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

const int ledPin = 2;

Adafruit_INA260 ina260 = Adafruit_INA260();

void connectToWPA2() {
  WiFi.disconnect(true);  // Disconnect from any previous connections
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

  // User Authentication
  auth.user.email = FIREBASE_EMAIL;
  auth.user.password = FIREBASE_PASSWORD;

  // Assign the callback function to handle token status
  ///////config.token_status_callback = tokenStatusCallback;

  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Start NTP
  timeClient.begin();
  
  while (!Serial) { delay(10); }

  Serial.println("Adafruit INA260 Test");

  //Wire.begin(20, 22); // Optional, default pins; adjust if needed

  if (!ina260.begin()) {
    Serial.println("Couldn't find INA260 chip");
    while (1);
  }
  Serial.println("Found INA260 chip");
  
  ina260.setAlertType(INA260_ALERT_UNDERVOLTAGE); //over ou under? (over marche avec 1)
  ina260.setAlertLimit(100.0);  // mV
  
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1);  // Wake up when GPIO 33 is LOW (high) essaye avec 1 si marche pas (avec 1 dort 2 min)


}

unsigned long lowPowerStartTime = 0;
bool lowPower = false;

void loop() {
  timeClient.update();
  String timestamp = String(timeClient.getEpochTime());
  
  Serial.println("Uploading value: " + String(ina260.readBusVoltage()));

  if (Firebase.RTDB.setInt(&fbdo, "/Current/" + timestamp, ina260.readCurrent())) {
    Serial.println("Upload success.");
  } else {
    Serial.println("Upload FAILED: " + fbdo.errorReason());
  }

   if (Firebase.RTDB.setInt(&fbdo, "/Voltage/" + timestamp, ina260.readBusVoltage())) {
    Serial.println("Upload success.");
  } else {
    Serial.println("Upload FAILED: " + fbdo.errorReason());
  }

   if (Firebase.RTDB.setInt(&fbdo, "/Power/" + timestamp, ina260.readPower())) {
    Serial.println("Upload success.");
  } else {
    Serial.println("Upload FAILED: " + fbdo.errorReason());
  }

  if (ina260.readBusVoltage() < 100.0) {
    if (!lowPower) {
      lowPowerStartTime = millis();
      lowPower = true;
    } else if (millis() - lowPowerStartTime >= 60000) {
      Serial.println("Power has been below 100mW for 1 minute. Entering deep sleep.");
      
      // Optional: small delay to ensure logs print
      delay(100);

      // Configure wake-up source before sleep
      //esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 0);  // ALERT or button pin, LOW trigger

      // Go to deep sleep
      esp_deep_sleep_start();
    }
  } else {
    lowPower = false;
    lowPowerStartTime = 0;
  }

  delay(5000);
}
