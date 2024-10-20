// Blynk template information
#define BLYNK_AUTH_TOKEN "Og3y9YY1Ou-F_5X71MrtHvhOC0HR2wON"
#define BLYNK_TEMPLATE_ID "TMPL6dG_hvgmE"
#define BLYNK_TEMPLATE_NAME "Projects"

#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// Wi-Fi credentials
char ssid[] = "L";
char pass[] = "12345678";

SoftwareSerial pmsSerial(D2, D1); // D2 (RX), D1 (TX)

struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};

struct pms5003data data;

void setup() {
  Serial.begin(9600);
  pmsSerial.begin(9600);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Serial.println("NodeMCU connected to PMS5003.");
}

void loop() {
  Blynk.run();  

  if (readPMSdata(&pmsSerial)) {
    Serial.print("PM25: ");
    Serial.println(data.pm25_env);
    Blynk.virtualWrite(V1, data.pm25_env);
    Blynk.virtualWrite(V0, data.pm10_env);
    Blynk.virtualWrite(V2, data.pm100_env);
  }
}

boolean readPMSdata(Stream *s) {
  uint8_t buffer[32];
  uint16_t sum = 0;
  unsigned long start = millis();
  while (s->available() < 1 || s->peek() != 0x42) {
    if (s->available()) s->read();
    if (millis() - start > 1000) {
      Serial.println("Timeout waiting for start byte");
      return false;
    }
  }
  start = millis();
  while (s->available() < 32) {
    if (millis() - start > 1000) {
      Serial.println("Incomplete data packet, skipping...");
      return false;
    }
  }

  s->readBytes(buffer, 32);
  for (uint8_t i = 0; i < 30; i++) {
    sum += buffer[i];
  }
  uint16_t receivedChecksum = (buffer[30] << 8) | buffer[31];
  if (sum != receivedChecksum) {
    Serial.println("Checksum mismatch, skipping packet");
    return false;
  }
  data.framelen = (buffer[2] << 8) | buffer[3];
  data.pm10_standard = (buffer[4] << 8) | buffer[5];
  data.pm25_standard = (buffer[6] << 8) | buffer[7];
  data.pm100_standard = (buffer[8] << 8) | buffer[9];
  data.pm10_env = (buffer[10] << 8) | buffer[11];
  data.pm25_env = (buffer[12] << 8) | buffer[13];
  data.pm100_env = (buffer[14] << 8) | buffer[15];

  return true;
}
