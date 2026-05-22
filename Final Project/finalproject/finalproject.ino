#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// === HARDWARE PIN ASSIGNMENTS ===
#define DHTPIN 2
#define DHTTYPE DHT11      // Gamitin ang DHT11 (Palitan ng DHT22 kung puti ang sensor niyo)
#define MQ135PIN A0
#define LDRPIN A1
#define BUZZERPIN 3

// === COMPONENT INITIALIZATION ===
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C Address 0x27 for LCD screen

unsigned long previous_telemetry_millis = 0;
const long telemetry_interval = 15000; // forward packets every 15 secs

void setup() {
  Serial.begin(9600);
  dht.begin();
  
  // I-initialize ang I2C LCD Module
  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  lcd.print("   AI - EMCS    ");
  lcd.setCursor(0, 1);
  lcd.print(" INITIALIZING.. ");
  
  pinMode(BUZZERPIN, OUTPUT);
  noTone(BUZZERPIN); 
  delay(2000);
  
  lcd.clear();
}

void loop() {
  unsigned long current_millis = millis();

  // 1. DATA TRANSMISSION LAYER 
  if (current_millis - previous_telemetry_millis >= telemetry_interval) {
    previous_telemetry_millis = current_millis;

    float current_temperature = dht.readTemperature();
    float current_humidity = dht.readHumidity();
    int raw_co2_value = analogRead(MQ135PIN);
    int raw_light_value = analogRead(LDRPIN);
    
    // Calibrated Mapping for LDR
    int light_percentage = map(raw_light_value, 200, 450, 0, 100); 
    light_percentage = constrain(light_percentage, 0, 100); 
    
    if (!isnan(current_temperature) && !isnan(current_humidity)) {
      Serial.print("{\"temp\":"); Serial.print(current_temperature, 1);
      Serial.print(",\"hum\":"); Serial.print(current_humidity, 1);
      Serial.print(",\"co2\":"); Serial.print(raw_co2_value);
      Serial.print(",\"light\":"); Serial.print(light_percentage);
      Serial.println("}");
    }
  }

  // 2. INCOMING COMMAND LISTENER 
  if (Serial.available() > 0) {
    char incoming_control_command = Serial.read();

    if (incoming_control_command == 'H') { // CRITICAL HAZARD STATE
      // (Rapid high-pitch chime)
      tone(BUZZERPIN, 2500);
      delay(100);
      noTone(BUZZERPIN);
      delay(100);
      
      lcd.setCursor(0, 0);
      lcd.print("STATUS: CRITICAL");
      lcd.setCursor(0, 1);
      lcd.print("EVACUATE ROOM!  ");
    }
    else if (incoming_control_command == 'W') { // ELEVATED WARNING STATE 
      // (Rhythmic pulse audio)
      tone(BUZZERPIN, 1200);
      delay(400);
      noTone(BUZZERPIN);
      delay(400);
      
      lcd.setCursor(0, 0);
      lcd.print("STATUS: WARNING ");
      lcd.setCursor(0, 1);
      lcd.print("CHECK ROOM AIR  ");
    }
    else if (incoming_control_command == 'S') { // STANDARD SAFE STATE
      noTone(BUZZERPIN); 
      
      lcd.setCursor(0, 0);
      lcd.print("STATUS: SECURE  ");
      lcd.setCursor(0, 1);
      lcd.print("SYSTEM OPTIMAL  ");
    }
  }
}