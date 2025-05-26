#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>

// DHT setup
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// LED pins
#define LED_RED 9
#define LED_YELLOW 10
#define LED_GREEN 6

// Buzzer & Fan
#define BUZZER_PIN 7
#define FAN_PIN 8

// SD card
#define SD_CS 4

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Flags
int flag_temp = 0;
int flag_hum = 0;
int last_led_state = -1;

unsigned long lastLogTime = 0;
const unsigned long LOG_INTERVAL = 1 * 60 * 1000UL;

void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);

  digitalWrite(FAN_PIN, LOW);

  lcd.init();
  lcd.backlight();

  if (!SD.begin(SD_CS)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SD Init Failed!");
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SD Ready");
    delay(1000);
  }
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  String message = "";

  // Evaluare umiditate
  if (hum < 35) {
    flag_hum = 2;
    message = "Low humidity!";
    lcd.clear();
    lcdMessage("Human needed", "Low humidity!");
  } else if (hum >= 35 && hum < 45) {
    flag_hum = 1;
  } else if (hum >= 45 && hum <= 65) {
    flag_hum = 0;
  } else if (hum > 65) {
    flag_hum = 2;
    message = "High humidity!";
    lcd.clear();
    lcdMessage("Human needed", "High humidity!");
  }

  // Evaluare temperatura
  if (temp < 25) {
    flag_temp = 2;
    message = "Low temperature!";
    lcd.clear();
    lcdMessage("Human needed", "Low temperature!");
    digitalWrite(FAN_PIN, LOW);
  } else if (temp >= 25 && temp < 26) {
    flag_temp = 1;
    digitalWrite(FAN_PIN, LOW);
  } else if (temp >= 26 && temp <= 27) {
    flag_temp = 0;
    digitalWrite(FAN_PIN, LOW);
  } else if (temp > 27) {
    flag_temp = 2;
    message = "High temperature!";
    lcd.clear();
    lcdMessage("Human needed", "High temperature!");
    digitalWrite(FAN_PIN, HIGH);
  }

  // LED & buzzer logic
  int led_state;
  if (flag_temp == 2 || flag_hum == 2) {
    led_state = 2; // roșu
  } else if (flag_temp == 1 || flag_hum == 1) {
    led_state = 1; // galben
  } else {
    led_state = 0; // verde
  }

  digitalWrite(LED_GREEN, led_state == 0);
  digitalWrite(LED_YELLOW, led_state == 1);
  digitalWrite(LED_RED, led_state == 2);

  // Buzzer cu TONE – doar când LED-ul roșu se aprinde
  if (led_state == 2 && last_led_state != 2) {
    for (int i = 0; i < 3; i++) {
      int freq = 1000 - (i * 200);  // 1000 → 800 → 600 Hz
      tone(BUZZER_PIN, freq);
      delay(300);
      noTone(BUZZER_PIN);
      delay(300);
    }
  } else {
    noTone(BUZZER_PIN);  // asigură oprirea completă
  }

  last_led_state = led_state;

  // Logging pe SD
  if (millis() - lastLogTime >= LOG_INTERVAL) {
    lastLogTime = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Logging data...");

    File dataFile = SD.open("log.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.print(millis() / 1000);
      dataFile.print(", ");
      dataFile.print(temp);
      dataFile.print(", ");
      dataFile.println(hum);
      dataFile.close();
    }

    delay(2000);
  }

  // Afișare implicită
  if (led_state != 2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temp);
    lcd.print(" C");
    lcd.setCursor(0, 1);
    lcd.print("Hum: ");
    lcd.print(hum);
    lcd.print(" %  ");
  }

  delay(2000);
}

void lcdMessage(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}
