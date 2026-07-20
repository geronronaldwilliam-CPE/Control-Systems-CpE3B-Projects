#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// === HARDWARE CONFIGURATION ===
#define DHTPIN 2          
#define DHTTYPE DHT11     
#define LDR_PIN A1        
#define MQ_PIN A0         
#define BUZZER_PIN 8      

// === INITIALIZATION ===
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); 

unsigned long lastSensorRead = 0;
const unsigned long interval = 2000; 

bool receivedFirstCommand = false; 
String currentMessage = "";
String currentHeader = "";
char currentMode = 'S';
int scrollPos = 0;

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN); 
  
  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  lcd.print("   AI - EMCS    ");
  lcd.setCursor(0, 1);
  lcd.print("INITIALIZING....");
  delay(2000);
  lcd.clear();
}

void loop() {
  unsigned long currentMillis = millis();

  static float currentTemp = 0.0;
  static float currentHum = 0.0;
  static int currentCO2 = 0;
  static int currentLight = 0;

  // 1. TIMED SENSOR READOUT LAYER (Kada 2 segundo, nagpapadala sa Python)
  if (currentMillis - lastSensorRead >= interval) {
    lastSensorRead = currentMillis;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int raw_co2 = analogRead(MQ_PIN);
    int raw_light = analogRead(LDR_PIN);

    if (isnan(h) || isnan(t)) {
      t = 0.0;
      h = 0.0;
    }

    int light_percentage = map(raw_light, 0, 1023, 0, 100);
    if(light_percentage > 100) light_percentage = 100;
    if(light_percentage < 0) light_percentage = 0;

    int co2_index = map(raw_co2, 0, 1023, 10, 400); 

    currentTemp = t;
    currentHum = h;
    currentCO2 = co2_index;
    currentLight = light_percentage;

    // Ipadala ang data sa Python
    Serial.print("{\"temp\": ");
    Serial.print(t, 1);
    Serial.print(", \"hum\": ");
    Serial.print(h, 1);
    Serial.print(", \"co2\": ");
    Serial.print(co2_index);
    Serial.print(", \"light\": ");
    Serial.print(light_percentage);
    Serial.println("}");

    // Default Preview Screen (Gagana LANG kung walang emergency)
    if (currentLight >= 30 && currentCO2 <= 200 && currentTemp <= 37.0 && currentMode == 'S' && !receivedFirstCommand) {
      lcd.setCursor(0, 0);
      lcd.print("T:");
      lcd.print(currentTemp, 1);
      lcd.print("C | H:");    
      lcd.print(currentHum, 0);
      lcd.print("%  "); // May space sa dulo para burado ang lumang letra
      
      lcd.setCursor(0, 1);
      lcd.print("CO2:");
      lcd.print(currentCO2);
      if (currentCO2 < 10) lcd.print("   | L:"); 
      else if (currentCO2 < 100) lcd.print("  | L:");  
      else lcd.print(" | L:");   
      lcd.print(currentLight);
      lcd.print("%   ");
    }
  }

  // ⚡ 1.5 INSTANT HARDWARE OVERRIDE (On-Time Protection Layer)
  if (currentLight < 30 && currentMode != 'W') {
    currentMode = 'W';
    currentHeader = "STATUS: WARNING ";
    currentMessage = "Low visibility! Please switch on the indoor lights.";
    receivedFirstCommand = true;
    scrollPos = 0;
    lcd.clear();
    delay(50); // Kaunting hinga para sa I2C bus
    lcd.setCursor(0, 0);
    lcd.print(currentHeader);
  }
  else if (currentCO2 > 200 && currentMode != 'H') {
    currentMode = 'H';
    currentHeader = "STATUS: HAZARD! ";
    currentMessage = "High CO2 detected! Open windows immediately.";
    receivedFirstCommand = true;
    scrollPos = 0;
    lcd.clear();
    delay(50); 
    lcd.setCursor(0, 0);
    lcd.print(currentHeader);
  }
  // Kung bumalik na sa normal ang lahat, linisin ang screen bago mag-safe mode
  else if (currentLight >= 30 && currentCO2 <= 200 && currentTemp <= 37.0 && (currentMode == 'W' || currentMode == 'H')) {
    currentMode = 'S';
    receivedFirstCommand = false;
    currentMessage = "";
    noTone(BUZZER_PIN);
    lcd.clear();
    delay(50);
  }

  // 2. SERIAL STRING PARSER (Mula sa Python papuntang Arduino)
  if (Serial.available() > 0) {
    String inputString = Serial.readStringUntil('\n');
    inputString.trim();

    if (inputString.length() > 2 && inputString.charAt(1) == ':') {
      char newMode = inputString.charAt(0);
      String newMessage = inputString.substring(2);
      
      // Mag-a-update lang ang LCD kung may totoong pagbabago sa utos para iwas-glitch
      if (newMode != currentMode || newMessage != currentMessage) {
        currentMode = newMode;
        currentMessage = newMessage;
        scrollPos = 0; 
        receivedFirstCommand = (currentMode != 'S');
        
        if (currentMode == 'H') currentHeader = "STATUS: HAZARD! ";
        else if (currentMode == 'W') currentHeader = "STATUS: WARNING ";
        else currentHeader = "STATUS: SECURE  ";
        
        lcd.clear();
        delay(50);
        lcd.setCursor(0, 0);
        lcd.print(currentHeader);
      }
    }
  }

  // 3. CONTINUOUS ANIMATION & PASSIVE AUDIO LAYER (Ginawang 400ms para mas kalmado ang LCD)
  static unsigned long lastAnimationTick = 0;
  if (receivedFirstCommand && (currentMillis - lastAnimationTick >= 400)) { 
    lastAnimationTick = currentMillis;

    if (currentMessage.length() > 0) {
      String paddedMessage = currentMessage + "    *** "; 
      lcd.setCursor(0, 1);
      
      for (int i = 0; i < 16; i++) {
        int charIndex = (scrollPos + i) % paddedMessage.length();
        lcd.print(paddedMessage.charAt(charIndex));
      }
      scrollPos++;
    }

    // Buzzer Control
    if (currentMode == 'H') {
      static bool toggle = false;
      toggle = !toggle;
      if (toggle) tone(BUZZER_PIN, 2500); 
      else noTone(BUZZER_PIN);
    } 
    else if (currentMode == 'W') {
      static int warningPulse = 0;
      warningPulse++;
      if (warningPulse % 4 == 0) tone(BUZZER_PIN, 1500);
      else noTone(BUZZER_PIN);
    } 
    else {
      noTone(BUZZER_PIN);
    }
  }
}