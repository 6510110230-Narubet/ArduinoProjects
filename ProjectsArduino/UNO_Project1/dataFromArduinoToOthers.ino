#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

SoftwareSerial odroidCommu(0, 1);
SoftwareSerial pmsSerial(2, 3);

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(9600);
  pmsSerial.begin(9600);
  lcd.init();
  lcd.backlight();
}
 
struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};
 
struct pms5003data data;
    
void loop() {
  if (readPMSdata(&pmsSerial)) {
    Serial.print("PM25:");
    Serial.println(data.pm25_env);
    odroidCommu.print(data.pm25_env);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PM 2.5: ");
    lcd.print(data.pm25_env);
    lcd.print(" ug/m3");
    lcd.setCursor(0, 1);
    lcd.print("Status: ");
    lcd.print(getAirCondition(data.pm25_env));
    delay(2000);
    
  }
}

String getAirCondition(uint16_t pm25) {
  // Determine the air quality condition based on PM 2.5 value
  if (pm25 <= 12) return "Good";
  else if (pm25 <= 35) return "Moderate";
  else if (pm25 <= 55) return "Unhealthy for SG";
  else if (pm25 <= 150) return "Unhealthy";
  else if (pm25 <= 250) return "Very Unhealthy";
  else return "Hazardous";
}

boolean readPMSdata(Stream *s) {
  uint8_t buffer[32];
  uint16_t sum = 0;

  unsigned long start = millis();

  while (s->available() < 1 || s->peek() != 0x42) {
    s->read();
    if (millis() - start > 1000) {
      Serial.println("Timeout waiting for start byte");
      return false;
    }
  }
  start = millis();
  while (s->available() < 32) {
    if (millis() - start > 1000) {
      Serial.println("Incomplete data packet");
      return false;
    }
  }
  s->readBytes(buffer, 32);
  for (uint8_t i = 0; i < 30; i++) {
    sum += buffer[i];
  }
  uint16_t receivedChecksum = (buffer[30] << 8) | buffer[31];
  if (sum != receivedChecksum) {
    Serial.print("Checksum failure: calculated=");
    Serial.print(sum, HEX);
    Serial.print(" expected=");
    Serial.println(receivedChecksum, HEX);
    return false;
  }
  data.framelen = (buffer[2] << 8) | buffer[3];
  data.pm10_standard = (buffer[4] << 8) | buffer[5];
  data.pm25_standard = (buffer[6] << 8) | buffer[7];
  data.pm100_standard = (buffer[8] << 8) | buffer[9];
  data.pm10_env = (buffer[10] << 8) | buffer[11];
  data.pm25_env = (buffer[12] << 8) | buffer[13];
  data.pm100_env = (buffer[14] << 8) | buffer[15];
  data.particles_03um = (buffer[16] << 8) | buffer[17];
  data.particles_05um = (buffer[18] << 8) | buffer[19];
  data.particles_10um = (buffer[20] << 8) | buffer[21];
  data.particles_25um = (buffer[22] << 8) | buffer[23];
  data.particles_50um = (buffer[24] << 8) | buffer[25];
  data.particles_100um = (buffer[26] << 8) | buffer[27];
  data.unused = (buffer[28] << 8) | buffer[29];
  data.checksum = receivedChecksum;

  return true;
}


