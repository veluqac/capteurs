#include <WiFi.h>

#include <esp_wpa2.h> // Needed for WPA2 Enterprise
#include <Firebase_ESP_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

const int ledPin = 2;

const int sensorPin = 35;

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
  pinMode(sensorPin, INPUT);

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
  

}

void loop() {
  timeClient.update();
  String timestamp = String(timeClient.getEpochTime());

  int sensorValue = analogRead(sensorPin); // Read the analog input
  
  float voltage = (sensorValue); // Convert to voltage
  
  Serial.println("Uploading value: ");
  
  Serial.println(voltage);
  

   if (Firebase.RTDB.setInt(&fbdo, "/Voltage/" + timestamp, voltage)) {
    Serial.println("Upload success.");
  } else {
    Serial.println("Upload FAILED: " + fbdo.errorReason());
  }

  
  delay(5000);
}
