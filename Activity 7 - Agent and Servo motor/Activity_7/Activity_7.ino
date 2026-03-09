// RONALD WILLIAM D. GERON
// CPE3B

// Include the Servo library
#include <Servo.h>

// Create a Servo object
Servo myServo;
const int servoPin = 9;

void setup() {
  // Initialize serial communication at 9600 bits per second
  Serial.begin(9600);
  myServo.attach(servoPin); // Attach the servo to pin 9
  myServo.write(90); // Start at neutral position (90 degrees)
  Serial.println("Arduino Ready. Send degrees (0-180):");
}

void loop() {
  // Check if data is available in the serial buffer
  if (Serial.available() > 0) {
    // Read the incoming string until a newline character
    String inputString = Serial.readStringUntil('\n');
    inputString.trim(); // Remove any leading/trailing whitespace

    // Validate if the input is purely numeric
    if (isNumeric(inputString)) {
      int degrees = inputString.toInt();

      // Validate the range of degrees (0 to 180)
      if (degrees >= 0 && degrees <= 180) {
        myServo.write(degrees); // Move the servo
        Serial.print("Success: Moved to ");
        Serial.print(degrees);
        Serial.println(" degrees.");
      } else {
        Serial.println("Error: Value out of range (0-180).");
      }
    } else {
      Serial.println("Error: Non-numeric input received.");
    }
  }
}

// Helper function to check if a string contains only digits
bool isNumeric(String str) {
  if (str.length() == 0) return false;
  for (byte i = 0; i < str.length(); i++) {
    if (!isDigit(str.charAt(i))) return false;
  }
  return true;
}