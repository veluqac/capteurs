const int sensorPin = A0; // Pin where the voltage sensor is connected
const float referenceVoltage = 5.0; // Reference voltage of Arduino (5V for most boards)
const int resolution = 1023; // 10-bit ADC resolution (0-1023)

void setup() {
    Serial.begin(115200); // Start serial communication at 9600 baud
}

void loop() {
    int sensorValue = analogRead(sensorPin); // Read the analog input
    float voltage = (sensorValue * referenceVoltage) / resolution; // Convert to voltage
     //float voltage = (sensorValue); // Convert to voltage
    
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.println(" V");
    
    delay(500); // Wait half a second before the next reading
}
// 0v -> 0
// 5v -> 203
// 10v -> 404
// 15v -> 606
// 20v -> 808
