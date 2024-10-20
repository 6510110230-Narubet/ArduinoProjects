#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad_I2C.h>
#include <Keypad.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

// Pin definitions for coin detector
const int sensorPins[] = {8, 9, 10, 11}; // Sensor pins connected to Arduino
const int buttonPin = 2; // Pin for the reset button
const int numSensors = 4; // Number of sensors
int sensorStates[numSensors]; // Variable to store the current state of sensors
int lastSensorStates[numSensors]; // Variable to store the last state of sensors
const int coinValues[] = {1, 2, 5, 10}; 
int totalAmount = 0; // Variable to store the total amount
int lastTotalAmount = 0; // Variable to store the last total amount
bool coinDetected = false; // Variable to track coin detection

// LCD setup
#define LCD_ADDRESS 0x27
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);

// Keypad setup
#define I2CADDR 0x20
const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
 {'1','2','3','A'},
 {'4','5','6','B'},
 {'7','8','9','C'},
 {'*','0','#','D'}
};
byte rowPins[ROWS] = {7, 6, 5, 4};
byte colPins[COLS] = {3, 2, 1, 0};
unsigned long last_time = 0;
bool blink_txt;
Keypad_I2C keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS, I2CADDR);

// Time tracking
unsigned long lastActivityTime = 0; // Time when the last activity occurred
const unsigned long sleepDelay = 20000; // Time to wait before entering sleep mode (20 seconds)

// Timekeeping
tmElements_t tm; // for DS1307
int Hour, Minute;

unsigned int digit;
bool flag = false;
bool blink_tx = false;

void print2digit(int val){
  if(val < 10 && val >= 0)
     lcd.print(0);
  lcd.print(val);
}

// Sleep mode settings
#define SLEEP_IDLE 0
#define SLEEP_ADC_REDUCE 1
#define SLEEP_POWER_DOWN 2
#define SLEEP_POWER_SAVE 3
#define SLEEP_STANDBY 6
#define SLEEP_EXT_STANDBY 7

void setup() {
  Serial.begin(38400);
  Wire.begin();
  keypad.begin();
  lcd.init();
  lcd.backlight(); // Turn on backlight
  digit = !digit;
  lcd.home(); 
  
  // Read total amount from EEPROM
  totalAmount = EEPROM.read(0);
  totalAmount |= EEPROM.read(1) << 8;
  lastTotalAmount = totalAmount;

  // Initialize sensor pins
  initializeSensors();
  setupInterrupts(); // Set up Pin Change Interrupts

  pinMode(buttonPin, INPUT); // Set pin for reset button to INPUT
  
  // Set up interrupt for reset button (INT0) to wake up the system
  attachInterrupt(digitalPinToInterrupt(buttonPin), wakeUp, FALLING);
}

void loop() {
  // Check if time is read from RTC
  if (RTC.read(tm)) { 
    char key = keypad.getKey(); // Read key from keypad

    if (key == 'B') { // Toggle digit for blinking
      digit = !digit;
      flag = true;
    } else if (key == 'C') { // Increase value
      if (!digit)
        Hour < 23 ? Hour++ : Hour = 0;
      else
        Minute < 59 ? Minute++ : Minute = 0;
    } else if (key == 'D') { // Decrease value
      if (!digit)
        Hour > 0 ? Hour-- : Hour = 23;
      else
        Minute > 0 ? Minute-- : Minute = 59;
    } else if (key == '#') { // Save changes
      digit = 0;
      tm.Hour = Hour;
      tm.Minute = Minute;
      RTC.write(tm);
      flag = false;
    }

    // Update time display
    lcd.setCursor(0, 0);
    lcd.print("Total: ");
    lcd.print(totalAmount); // Print total amount
    lcd.print(" Baht   "); // Print the currency
    lcd.setCursor(0, 1); // Set cursor to the second line
    lcd.print("Time: ");
    if (flag) { // If B was pressed
      if ((millis() - last_time) > 1000) { // Blink every 1 second
        blink_tx = !blink_tx;
        last_time = millis();
      }
      if (!digit) { // Blinking hour
        if (blink_tx)
          lcd.print("  ");
        else
          print2digit(Hour);
        lcd.print(":");
        print2digit(Minute);
      } else { // Blinking minute
        print2digit(Hour);
        lcd.print(":");
        if (blink_tx)
          lcd.print("  ");
        else
          print2digit(Minute);
      }
    } else { // No blinking
      Hour = tm.Hour;
      Minute = tm.Minute;
      print2digit(Hour);
      lcd.print(":");
      print2digit(Minute);
    }
  }

  // Check if the reset button is pressed
  if (digitalRead(buttonPin) == HIGH) { 
    resetTotalAmount(); // Reset total amount and EEPROM
    lastActivityTime = millis(); // Update last activity time
  }

  // Update EEPROM only when totalAmount changes
  if (totalAmount != lastTotalAmount) {
    EEPROM.write(0, totalAmount & 0xFF);
    EEPROM.write(1, (totalAmount >> 8) & 0xFF);
    lastTotalAmount = totalAmount; // Update last total amount
    lastActivityTime = millis(); // Update last activity time
  }

  // Update LCD if a coin is detected
  if (coinDetected) {
    lcd.backlight();
    updateLCD(totalAmount);
    coinDetected = false; // Reset coin detection flag
    lastActivityTime = millis(); // Update last activity time
  } else if (millis() - lastActivityTime >= sleepDelay) {
    // If no activity for 5 seconds, enter sleep mode
    enterSleepMode();
    lastActivityTime = millis(); // Reset last activity time after waking up
  }
}

void initializeSensors() {
  for (int i = 0; i < numSensors; i++) {
    pinMode(sensorPins[i], INPUT);
    digitalWrite(sensorPins[i], HIGH); // Enable pull-up resistors
    
    // กำหนดค่าเริ่มต้นให้กับ lastSensorStates[]
    lastSensorStates[i] = digitalRead(sensorPins[i]); 
  }
}


// Function to set up interrupts
void setupInterrupts() {
  sei(); // Enable global interrupts
  PCICR |= (1 << PCIE0); // Enable Pin Change Interrupt for PCINT0..7
  PCMSK0 |= (1 << PCINT0) | (1 << PCINT1) | (1 << PCINT2) | (1 << PCINT3); // Enable interrupts for pins 8, 9, 10, 11
}

// เวลาการหน่วงที่ต้องรอ (500ms)
const unsigned long debounceDelay = 500;
unsigned long lastDetectionTime[numSensors]; // บันทึกเวลาที่ตรวจจับเหรียญได้ครั้งสุดท้าย

// Interrupt Service Routine for Pin Change Interrupt
ISR(PCINT0_vect) {
  unsigned long currentTime = millis();
  
  for (int i = 0; i < numSensors; i++) {
    // ตรวจสอบว่าผ่านเวลาการหน่วงไปแล้วหรือไม่
    if (currentTime - lastDetectionTime[i] > debounceDelay) {
      // ตรวจสอบสถานะการเปลี่ยนแปลงของเซนเซอร์
      if ((i < 3 && digitalRead(sensorPins[i]) == LOW && lastSensorStates[i] == HIGH) || 
          (i == 3 && digitalRead(sensorPins[i]) == HIGH && lastSensorStates[i] == LOW)) {
        totalAmount += coinValues[i];
        coinDetected = true; // ตั้งค่าสถานะว่าตรวจพบเหรียญแล้ว

        Serial.print("Coin of ");
        Serial.print(coinValues[i]);
        Serial.println(" detected!");
        
        // อัปเดต Serial
        Serial.print("Updated Total amount: ");
        Serial.println(totalAmount);
        
        // บันทึกเวลาการตรวจจับครั้งนี้
        lastDetectionTime[i] = currentTime;
      }
    }
    lastSensorStates[i] = digitalRead(sensorPins[i]);
  }
}


// Function to update the LCD with the total amount
void updateLCD(int amount) {
  lcd.setCursor(0, 0); // Set cursor to the first line
  lcd.print("Total: "); // Print text
  lcd.print(amount); // Print total amount
  lcd.print(" Baht    "); // Print the currency
}

// Function to reset total amount and EEPROM
void resetTotalAmount() {
  totalAmount = 0; // Reset total amount
  EEPROM.write(0, 0); // Reset EEPROM value
  EEPROM.write(1, 0);
  lastTotalAmount = totalAmount; // Update last total amount
  
  // Update LCD to reflect reset
  updateLCD(totalAmount);
}

// Function to enter sleep mode
void enterSleepMode() {
  // Disable ADC to save power
  ADCSRA &= ~(1 << ADEN);

  // Set sleep mode to power-save
  set_sleep_mode(SLEEP_POWER_SAVE);
  sleep_enable();
  
  // Turn off LCD backlight before sleeping
  lcd.noBacklight();
  
  // Enter sleep mode
  sleep_cpu();
  
  // Wake up and re-enable ADC
  sleep_disable();
  ADCSRA |= (1 << ADEN);

  // Re-initialize the LCD if necessary
  lcd.begin(16, 2);
  lcd.backlight();
  updateLCD(totalAmount);
}

// Function to wake up the system from sleep mode
void wakeUp() {
  // This function is intentionally empty
  // The system will wake up automatically when the interrupt is triggered
}
