#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <avr/io.h>

// DHT setup
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Buzzer & Fan pins
#define BUZZER_PIN 7 // PD7
#define FAN_PIN 8    // PB0

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

  // Set pin directions using symbolic bit names
  DDRB |= (1 << PB1); // D9 - RED LED
  DDRB |= (1 << PB2); // D10 - YELLOW LED
  DDRD |= (1 << PD6); // D6 - GREEN LED
  DDRB |= (1 << PB0); // D8 - FAN
  DDRD |= (1 << PD7); // D7 - BUZZER

  // Initial state: FAN OFF
  PORTB &= ~(1 << PB0);

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

  // Humidity evaluation
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

  // Temperature evaluation
  if (temp < 25) {
    flag_temp = 2;
    message = "Low temperature!";
    lcd.clear();
    lcdMessage("", "Low temperature!");
    PORTB &= ~(1 << PB0); // FAN OFF
  } else if (temp >= 25 && temp < 26) {
    flag_temp = 1;
    PORTB &= ~(1 << PB0); // FAN OFF
  } else if (temp >= 26 && temp <= 27) {
    flag_temp = 0;
    PORTB &= ~(1 << PB0); // FAN OFF
  } else if (temp > 27) {
    flag_temp = 2;
    message = "High temperature!";
    lcd.clear();
    lcdMessage("Human needed", "High temperature!");
    PORTB |= (1 << PB0); // FAN ON
  }

  // Determine LED state
  int led_state;
  if (flag_temp == 2 || flag_hum == 2) {
    led_state = 2; // red
  } else if (flag_temp == 1 || flag_hum == 1) {
    led_state = 1; // yellow
  } else {
    led_state = 0; // green
  }

  // Turn off all LEDs
  PORTD &= ~(1 << PD6); // GREEN OFF
  PORTB &= ~(1 << PB2); // YELLOW OFF
  PORTB &= ~(1 << PB1); // RED OFF

  // Activate corresponding LED
  if (led_state == 0) {
    PORTD |= (1 << PD6); // GREEN ON
  } else if (led_state == 1) {
    PORTB |= (1 << PB2); // YELLOW ON
  } else if (led_state == 2) {
    PORTB |= (1 << PB1); // RED ON
  }

  // Buzzer with tone
  if (led_state == 2 && last_led_state != 2) {
    for (int i = 0; i < 3; i++) {
      int freq = 1000 - (i * 200); // 1000 → 800 → 600 Hz
      tone(BUZZER_PIN, freq);
      delay(300);
      noTone(BUZZER_PIN);
      delay(300);
    }
  } else {
    noTone(BUZZER_PIN);
  }

  last_led_state = led_state;

  // SD Logging
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

  // LCD display
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
