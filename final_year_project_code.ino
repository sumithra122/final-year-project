#include <Wire.h>
#include <SoftwareSerial.h>
#include <DHT.h>

// Pin Definitions
#define DHTPIN 4
#define DHTTYPE DHT11
#define FAN_PIN 5
#define PUMP1_PIN 6  // for pH
#define PUMP2_PIN 7  // for NPK
#define PH_PIN A0
#define RE 8
#define DE 9
// Modbus RTU requests for reading NPK values
const byte nitro[] = {0x01,0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c};
const byte phos[]  = {0x01,0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc};
const byte pota[]  = {0x01,0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0};

byte values[11];
const int nitrogen_THRESHOLD = 10;
const int phosphorous_THRESHOLD = 10;
const int potassium_THRESHOLD = 10;

SoftwareSerial npkSerial(10, 11);  // RX, TX
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  npkSerial.begin(9600);  // Set baud rate according to your NPK sensor
  dht.begin();
  pinMode(FAN_PIN, OUTPUT);
  pinMode(PUMP1_PIN, OUTPUT);
  pinMode(PUMP2_PIN, OUTPUT);
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
  digitalWrite(RE, LOW);
  digitalWrite(DE, LOW);
  digitalWrite(PUMP1_PIN, HIGH);  // OFF
  digitalWrite(PUMP2_PIN, HIGH);  // OFF
  digitalWrite(FAN_PIN, HIGH);    // OFF

  Serial.println("Hydroponics System Initializing...");
  delay(500);
}

void loop() {
  // Read temperature
  float temp = dht.readTemperature();
  if (isnan(temp)) {
    Serial.println("⚠️ DHT11 Error: Temperature read failed");
    return;
  }

  // Read pH sensor analog voltage
  int phRaw = analogRead(PH_PIN);
  float voltage = phRaw * (5.0 / 1023.0);
  float phValue = 7 + ((2.5 - voltage) / 0.18);  // Approximate equation

  // Read NPK values
  byte val1 = readNitrogen();
  delay(250);
  byte val2 = readPhosphorous();
  delay(250);
  byte val3 = readPotassium();
  delay(250);
  
  // Print Sensor Readings
  Serial.println("---- Sensor Readings ----");
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" °C");

  Serial.print("pH Voltage: ");
  Serial.print(voltage, 2);
  Serial.print(" V → pH: ");
  Serial.println(phValue, 2);
  
   Serial.print("Nitrogen: "); Serial.print(val1); Serial.println(" mg/kg");
  Serial.print("Phosphorous: "); Serial.print(val2); Serial.println(" mg/kg");
  Serial.print("Potassium: "); Serial.print(val3); Serial.println(" mg/kg");
  

  // FAN Control
  if (temp > 50) {
    digitalWrite(FAN_PIN, LOW);  // LOW = ON (for NC relay)
    Serial.println("FAN ON");
  } else {
    digitalWrite(FAN_PIN, HIGH);
    Serial.println("FAN OFF");
  }

  // Pump1 Control for pH
  if (phValue >= 10 && phValue <= 9) {
    digitalWrite(PUMP1_PIN, LOW);
    Serial.println("pH OK → PUMP1 ON");
    delay(3000);
     } else{
    digitalWrite(PUMP1_PIN, HIGH);
    Serial.println("PUMP1 OFF");
    Serial.println("⚠️ Abnormal pH → No action or pH adjustment needed");
  }

  // Pump2 Control for NPK
 
   if (val1 < nitrogen_THRESHOLD || val2 < phosphorous_THRESHOLD || val3 < potassium_THRESHOLD) {
    digitalWrite(PUMP2_PIN, LOW); // Turn ON
    Serial.println("Low NPK → PUMP ON");
    delay(3000);
  } else {
    digitalWrite(PUMP2_PIN, HIGH); // Turn OFF
    Serial.println("NPK levels OK → PUMP OFF");
  }

  delay(5000); // Wait before next reading
}
 
  

byte readNitrogen() {
  return readSensor(nitro, "Nitrogen");
}

byte readPhosphorous() {
  return readSensor(phos, "Phosphorous");
}

byte readPotassium() {
  return readSensor(pota, "Potassium");
}

byte readSensor(const byte *command, const char *label) {
  Serial.print("Requesting "); Serial.print(label); Serial.println("...");

  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(10);

  npkSerial.write(command, 8);
  npkSerial.flush();

  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);

  long timeout = millis();
  while (npkSerial.available() < 7) {
    if (millis() - timeout > 2000) {
      Serial.println("Timeout! No response.");
      return 255;
    }
    delay(10);
  }

  Serial.print("Response: ");
  for (byte i = 0; i < 7; i++) {
    values[i] = npkSerial.read();
    Serial.print(values[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  return values[4];
}
