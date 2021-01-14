#include <arduino-timer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <SPI.h>
#include <SD.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define DHTPIN 4
#define MOTOR_PIN 2
#define VOL_POT_PIN A0
#define DAY_POT_PIN A1
#define HUMIDITY_SENSOR A2
#define DHTTYPE DHT11
#define MENU_BUTTON 3
#define LIGHT_PIN A6
DHT dht(DHTPIN, DHTTYPE);

const int PUMP_SPEED = 10; //mL per second
const long HOUR_TO_MILLI = 60 * 60 * 1000;
const int DRY = 106;
const int WET = 76;
// change this to match your SD shield or module;
const int chipSelect = 10;
const int PERIOD = 1000;
const int PERSISTANCE_FREQUENCY = 30;

auto update_timer = timer_create_default();

File myFile;

long water_volume = 0;
long frequency = 0;
long humidity = 0;
bool menu = true;
bool read_humidity = true;
long light_level = 0;
int data_timer = 0;
long water_time = 0;
long watering_time = frequency * HOUR_TO_MILLI;

int air_humidity = 0;
int air_temperature = 0;


void setup() {
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(MENU_BUTTON, INPUT);
  dht.begin();
  lcd.init();
  lcd.backlight();
  SD.begin();

  read_sensors();

  update_timer.every(PERIOD, update_stuff);
}

void loop() {
  update_timer.tick();
}


bool update_stuff() {
  check_menu();
  read_sensors();
  lcd.clear();
  if (menu) {
    print_menu_1();
    print_values_1();
  } else {
    print_menu_2();
    print_values_2();
  }
  if (data_timer >= PERSISTANCE_FREQUENCY) {
    data_timer = 0;
    write_to_sd();
  }
  data_timer = data_timer + 1;
  if (water_time > 0) {
    water_time = water_time - PERIOD;
    if (water_time <= 0) {
      digitalWrite(MOTOR_PIN, false);
    }
  }
  watering_time = watering_time - PERIOD;
  if (watering_time <= 0) {
    check_watering();
  }
  return true;
}

void write_to_sd() {
  myFile = SD.open("data.csv", FILE_WRITE);
  String to_write = "";
  to_write = to_write + humidity + ";" + air_humidity + ";" + air_temperature + ";" + light_level + ";" + water_time;
  myFile.println(to_write);
  myFile.close();
}

void check_menu() {
  int buttonState;
  buttonState = digitalRead(MENU_BUTTON);
  if (buttonState == LOW) {
    menu = !menu;
  }
}

void read_sensors() {
  water_volume = analogRead(VOL_POT_PIN);
  frequency = (analogRead(DAY_POT_PIN) / 41); //100 /41 = 24 to turn 1 to 100 range into 1 to 24 range
  humidity = map(analogRead(HUMIDITY_SENSOR), 600, 235, 0, 100);//get_humidity_percentage(100 - (analogRead(HUMIDITY_SENSOR) / 10));
  read_dht();
  light_level = analogRead(LIGHT_PIN);
}

int get_humidity_percentage(int sensor_reading) {
  return (2.5 * abs(sensor_reading)) - 88.5;
}

void read_dht() {
  int bair_humidity = 0;
  int bair_temperature = 0;
  if (read_humidity) {
    bair_humidity = dht.readHumidity();
    if (!isnan(bair_humidity)) {
      air_humidity = bair_humidity;
    }
  } else {
    bair_temperature = dht.readTemperature();
    if (!isnan(bair_temperature)) {
      air_temperature = bair_temperature;
    }
  }
  read_humidity = !read_humidity;
}

void print_menu_1() {
  lcd.setCursor(0, 0);
  lcd.print("V");

  lcd.setCursor(8, 0);
  lcd.print("WH");

  lcd.setCursor(0, 1);
  lcd.print("W");

  lcd.setCursor(3, 1);
  lcd.print("F");
}

void print_menu_2() {
  lcd.setCursor(0, 0);
  lcd.print("AH");

  lcd.setCursor(7, 0);
  lcd.print("L");

  lcd.setCursor(0, 1);
  lcd.print("AT");
}

void print_values_1() {
  String water_message = "";
  water_message = water_message + water_volume;
  water_message = water_message + "ml";
  lcd.setCursor(1, 0);
  //lcd.print("       ");
  lcd.setCursor(1, 0);
  lcd.print(water_message);

  String humidity_message = "";
  humidity_message = humidity_message + humidity;
  humidity_message = humidity_message + "%";
  lcd.setCursor(10, 0);
  //lcd.print("    ");
  lcd.setCursor(10, 0);
  lcd.print(humidity_message);

  String frequency_message = "";
  frequency_message = frequency_message + frequency;
  frequency_message = frequency_message + "hs";
  lcd.setCursor(4, 1);
  //lcd.print("     ");
  lcd.setCursor(4, 1);
  lcd.print(frequency_message);

  lcd.setCursor(1, 1);
  if (water_time > 0) {
    lcd.print("Y");
  } else {
    lcd.print("N");
  }

}

void print_values_2() {
  String air_humidity_message = "";
  air_humidity_message = air_humidity_message + air_humidity;
  air_humidity_message = air_humidity_message + "%";
  lcd.setCursor(2, 0);
  //lcd.print("       ");
  lcd.setCursor(2, 0);
  lcd.print(air_humidity_message);

  lcd.setCursor(8, 0);
  lcd.print(light_level);

  String air_temperature_message = "";
  air_temperature_message = air_temperature_message + air_temperature;
  air_temperature_message = air_temperature_message + "d";
  lcd.setCursor(2, 1);
  //lcd.print("       ");
  lcd.setCursor(2, 1);
  lcd.print(air_temperature_message);

}

void check_watering() {
  watering_time = frequency * HOUR_TO_MILLI;
  if (water_time <= 0 &&  (humidity < 50)) {
    digitalWrite(MOTOR_PIN, true);
    water_time = (water_volume / PUMP_SPEED) * 1000;
  }
}
