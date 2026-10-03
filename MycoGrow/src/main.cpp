#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

char ssid[] = "";       
char pass[] = "";  

#define DHTPIN 25
#define DHTTYPE DHT22
#define BUZZER_PIN 26
#define RELAY_PIN 27

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
BlynkTimer timer;

bool manualMode = false;
bool relayManualState = false;

// Fungsi kontrol buzzer pakai PWM LEDC
void kontrolBuzzer(bool state) {
  if (state) {
    ledcWriteTone(0, 1000);  // Bunyikan buzzer di 1 kHz
  } else {
    ledcWriteTone(0, 0);     // Matikan buzzer
  }
}

// Fungsi kontrol relay dan buzzer
void kontrolRelay(bool state) {
  digitalWrite(RELAY_PIN, state ? LOW : HIGH); // Relay aktif LOW
  kontrolBuzzer(state);
}

// Tombol Manual Mode (V3)
BLYNK_WRITE(V3) {
  manualMode = param.asInt();
}

// Tombol Relay Manual (V0)
BLYNK_WRITE(V0) {
  relayManualState = param.asInt();
  if (manualMode) kontrolRelay(relayManualState);
}

// Kirim data sensor ke Blynk dan tampilkan di LCD
void sendSensorData() {
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();

  if (isnan(suhu) || isnan(kelembapan)) return;

  Blynk.virtualWrite(V1, suhu);
  Blynk.virtualWrite(V2, kelembapan);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(suhu, 1);
  lcd.print("C H:");
  lcd.print(kelembapan, 0);
  lcd.print("%");

  if (!manualMode) {
    if (suhu > 30 || kelembapan < 90) kontrolRelay(true);
    else kontrolRelay(false);
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  lcd.init();
  lcd.backlight();

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Inisialisasi LEDC untuk buzzer (channel 0, freq 1kHz, resolusi 8 bit)
  ledcSetup(0, 1000, 8);
  ledcAttachPin(BUZZER_PIN, 0);

  kontrolRelay(false);

  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.print("WiFi Connected");

  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

  timer.setInterval(2000L, sendSensorData);
}

void loop() {
  Blynk.run();
  timer.run();
}
