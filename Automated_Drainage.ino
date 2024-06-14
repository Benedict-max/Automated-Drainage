/*
 * Automated Drainage Monitoring and Control System
 * Presented by: Masela
 * 
 * This project monitors the drainage system using various sensors and
 * controls outputs like alarms and pumps. It sends SMS alerts in case of
 * detected issues such as blockages or high water levels.
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// Define sensor and module pins
#define TRIG_PIN 9
#define ECHO_PIN 10
#define FLOW_SENSOR_PIN 4
#define RELAY_PIN 5
#define PUMP_PIN 6
#define ALARM_PIN 7
#define BUTTON_PIN 8

// GSM and GPS module pins
#define GSM_RX_PIN 2
#define GSM_TX_PIN 3
#define GPS_RX_PIN 11
#define GPS_TX_PIN 12

// Define thresholds
#define WATER_LEVEL_THRESHOLD 20.0 // in cm
#define MAX_FLOW_RATE 10.0 // in L/min

// Timing and counting
volatile int pulseCount = 0;
unsigned long flowStartTime;
float flowRate = 0.0;

// LCD settings
LiquidCrystal_I2C lcd(0x27, 16, 2);

// GSM settings
SoftwareSerial gsmSerial(GSM_RX_PIN, GSM_TX_PIN); // RX, TX for GSM

// GPS settings
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN); // RX, TX for GPS
String gpsData = "";

// System state
bool blockageDetected = false;

// Function prototypes
void sendSMS(String message);
void readGPS();
void calculateFlowRate();
float readWaterLevel();
void controlSystem();
void displayStatus(float waterLevel, float flowRate, String gpsCoords);

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  
  // Initialize sensors and outputs
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(FLOW_SENSOR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(ALARM_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Attach interrupt for flow sensor
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), pulseCounter, FALLING);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  
  // Initialize GSM module
  gsmSerial.begin(9600);
  delay(1000);
  
  // Initialize GPS module
  gpsSerial.begin(9600);
  
  // Initial setup messages
  lcd.setCursor(0, 0);
  lcd.print("Drainage System");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
  delay(2000);
}

void loop() {
  // Read sensor data
  float waterLevel = readWaterLevel();
  calculateFlowRate();
  readGPS();
  
  // Control system based on sensor data
  controlSystem();
  
  // Display status on LCD
  displayStatus(waterLevel, flowRate, gpsData);
  
  // Simulate processing delay
  delay(1000);
}

void pulseCounter() {
  pulseCount++;
}

void calculateFlowRate() {
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - flowStartTime;
  
  if (elapsedTime >= 1000) {
    flowRate = (pulseCount / 7.5); // Convert to L/min
    pulseCount = 0;
    flowStartTime = currentTime;
  }
}

float readWaterLevel() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = (duration * 0.0343) / 2; // Distance in cm
  return distance;
}

void readGPS() {
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    if (c == '\n') {
      if (gpsData.indexOf("$GPGGA") != -1) {
        // Parse GPGGA sentence
        int commaIndex = gpsData.indexOf(',');
        gpsData = gpsData.substring(commaIndex + 1);
        // Extract latitude and longitude
        commaIndex = gpsData.indexOf(',');
        String latitude = gpsData.substring(0, commaIndex);
        gpsData = gpsData.substring(commaIndex + 1);
        commaIndex = gpsData.indexOf(',');
        gpsData = gpsData.substring(commaIndex + 1);
        commaIndex = gpsData.indexOf(',');
        String longitude = gpsData.substring(0, commaIndex);
        gpsData = "Lat:" + latitude + " Lon:" + longitude;
      }
      gpsData = "";
    } else {
      gpsData += c;
    }
  }
}

void controlSystem() {
  float waterLevel = readWaterLevel();
  calculateFlowRate();
  
  if (waterLevel > WATER_LEVEL_THRESHOLD || flowRate < 0.5) {
    blockageDetected = true;
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(PUMP_PIN, HIGH);
    digitalWrite(ALARM_PIN, HIGH);
    String alertMessage = "Alert: Blockage detected at " + gpsData;
    sendSMS(alertMessage);
  } else {
    blockageDetected = false;
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(PUMP_PIN, LOW);
    digitalWrite(ALARM_PIN, LOW);
  }
}

void sendSMS(String message) {
  gsmSerial.println("AT+CMGF=1"); // Set SMS mode to text
  delay(1000);
  gsmSerial.println("AT+CMGS=\"+1234567890\""); // Replace with actual phone number
  delay(1000);
  gsmSerial.print(message);
  delay(1000);
  gsmSerial.write(26); // ASCII code for Ctrl+Z to send message
  delay(1000);
}

void displayStatus(float waterLevel, float flowRate, String gpsCoords) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Water: ");
  lcd.print(waterLevel);
  lcd.print(" cm");
  lcd.setCursor(0, 1);
  lcd.print("Flow: ");
  lcd.print(flowRate);
  lcd.print(" L/min");
  
  if (blockageDetected) {
    lcd.setCursor(0, 0);
    lcd.print("Blockage Detected!");
    lcd.setCursor(0, 1);
    lcd.print(gpsCoords);
  }
}
