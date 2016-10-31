/*************************************************** 
  These displays use SPI to communicate, 4 or 5 pins are required to  
  interface
  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/

// You can use any (4 or) 5 pins 
#define sclk D5 //--- connect this to the display module SCL pin      (Serial Clock)
#define mosi D7 //--- connect this to the display module SDA/MOSI pin (Serial Data)
#define rst  D6 //--- connect this to the display module RES pin      (Reset)
#define dc   D8 //--- connect this to the display module DC  pin      (Data or Command)
#define cs   D0 //--- connect this to the display module CS  pin      (Chip Select)

#define DHTPIN D1
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "Adafruit_MCP9808.h"
#include "Adafruit_SHT31.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
#include <Wire.h>
#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "ClosedCube_Si7051.h"

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS D2

//------------------------------------------------------------------
// Create the display object
Adafruit_ILI9341 display = Adafruit_ILI9341(cs, dc);
//------------------------------------------------------------------
// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
//------------------------------------------------------------------
// Create the DHT22 temperature and humidity sensor object
DHT dht(DHTPIN, DHTTYPE);
//------------------------------------------------------------------
// Create the SHT31D temperature and humidity sensor object
Adafruit_SHT31 sht31 = Adafruit_SHT31();
//------------------------------------------------------------------
// Create the BME280 temperature and humidity sensor object
Adafruit_BME280 bme; // Note Adafruit assumes I2C adress = 0x77 my module uses 0x76
//------------------------------------------------------------------
// Create the DS18B20 temperature sensor objects
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
//------------------------------------------------------------------
// Create the Si7051 temperature sensor object
ClosedCube_Si7051 si7051;

int numberOfDevices; // Number of temperature devices found

DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address

void setup(void) {
  Serial.begin(9600);
  display.begin();
  display.fillScreen(ILI9341_BLACK);
  display.setRotation(3);
  display.setTextSize(2);
  pinMode(D4, INPUT_PULLUP); // Set D4 with a pull-up
  pinMode(D3, INPUT_PULLUP); // Set D3 with a pull-up
  //pinMode(D2, INPUT_PULLUP); // Set D2 with a pull-up
  pinMode(D1, INPUT_PULLUP); // Set D1 with a pull-up

  dht.begin();
  pinMode(D2, INPUT_PULLUP);  // Needed for DS18B20
  sensors.begin();            // Start DS18B20 services
  Wire.begin(D3,D4);          // Wire.begin([SDA], [SCL])
  if (!tempsensor.begin()) {
    Serial.println("Couldn't find MCP9808!");
    while (1);
  } else Serial.println("Found MCP9808");
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
   Serial.println("Couldn't find SHT31");
   while (1) delay(1);
  }
  si7051.begin(0x40); 
  display.fillScreen(ILI9341_BLACK);
  display.setCursor(0,0);
}

void loop() {
  // MCP9808 temperature value
  tempsensor.shutdown_wake(0);   // Don't remove this line! required before reading temp
  float MCP9808_t = tempsensor.readTempC();
  delay(300);
  
  // Read DHT temperature and humidity values
  float DHT22_t = dht.readTemperature();
  float DHT22_h = dht.readHumidity();
  delay(300);

  // BME280 temperature and humidity values
  float BME280_t = bme.readTemperature();
  float BME280_h = bme.readHumidity();
  float BME280_p = bme.readPressure()/100;
  delay(300);

  // BDS18B20 temperature value
  sensors.requestTemperatures(); // Send the command to get DS18B20 temperatures
  float DS18B20_t = sensors.getTempCByIndex(0);
  delay(300);

  // SHT31D temperature and humidity values
  float SHT31_t = sht31.readTemperature();
  float SHT31_h = sht31.readHumidity();
  int err_cnt = 0;
  while (isnan(SHT31_t) && err_cnt < 10) {
    SHT31_t = sht31.readTemperature();
    err_cnt += 1;
  }
  err_cnt = 0;
  while (isnan(SHT31_h) && err_cnt < 10) {
    SHT31_t = sht31.readTemperature();
    err_cnt += 1;
  }
  
  // SHT31D temperature and humidity values
  float Si7051_t = si7051.readTemperature();
  
  // Clear screen and display titles
  display.fillScreen(ILI9341_BLACK);
  display.setCursor(0,0);
  display.setTextColor(ILI9341_YELLOW);
  display.println("  Temperature & Humidity"); 
  display.println("     Device Comparator"); 

  // Display temperature and humidity for each device. Humidity = 0 when device does not support that function
  display_temp_humi("Si7051",    0,  50, Si7051_t,      0,  "0.1","");
  display_temp_humi("MCP9808", 110,  50, MCP9808_t,     0,  "0.25","");
  display_temp_humi("SHT31D",  220,  50, SHT31_t, SHT31_h,  "0.3","2");
  display_temp_humi("DHT22",     0, 150, DHT22_t, DHT22_h,  "0.5","2");
  display_temp_humi("BME280",  110, 150, BME280_t, BME280_h,"0.5","3");
  display_temp_humi("DS18B20", 220, 150, DS18B20_t, 0,      "0.5","");
  delay(1000);
}

void display_temp_humi(String device, int x, int y, float temp, float humi, String tolerance1, String tolerance2) {
  display.setCursor(x,y);
  display.setTextColor(ILI9341_GREEN);
  display.print(device);
  display.setCursor(x,y+20);
  display.setTextColor(ILI9341_RED);
  display.print(temp,1);
  display.print(char(247));
  display.print("C");
  if (humi > 0) {
    display.setCursor(x,y+40);
    display.setTextColor(ILI9341_MAGENTA);
    display.print(humi,1);
    display.print("%rH");
  }
  display.setTextColor(ILI9341_WHITE);
  display.setCursor(x,y+60);
  display.setTextSize(1);
  display.print(char(240));
  display.print(tolerance1+char(247)+"C");
  if (tolerance2 != "") {
    
  // Display temperature and humidity for each device. Humidity = 0 when device does not support that function
  display_temp_humi("Si7051",    0,  50, Si7051_t,      0,  "0.1","");
  display_temp_humi("MCP9808", 110,  50, MCP9808_t,     0,  "0.25","");
  display_temp_humi("SHT31D",  220,  50, SHT31_t, SHT31_h,  "0.3","2");
  display_temp_humi("DHT22",     0, 150, DHT22_t, DHT22_h,  "0.5","2");
  display_temp_humi("BME280",  110, 150, BME280_t, BME280_h,"0.5","3");
  display_temp_humi("DS18B20", 220, 150, DS18B20_t, 0,      "0.5","");
  delay(1000);
}
void display_temp_humi(String device, int x, int y, float temp, float humi, String tolerance1, String tolerance2) {
  display.setCursor(x,y);
  display.setTextColor(ILI9341_GREEN);
  display.print(device);
  display.setCursor(x,y+20);
  display.setTextColor(ILI9341_RED);
  display.print(temp,1);
  display.print(char(247));
  display.print("C");
  if (humi > 0) {
    display.setCursor(x,y+40);
    display.setTextColor(ILI9341_MAGENTA);
    display.print(humi,1);
    display.print("%rH");
  }
  display.setTextColor(ILI9341_WHITE);
  display.setCursor(x,y+60);
  display.setTextSize(1);
  display.print(char(240));
  display.print(tolerance1+char(247)+"C");
  if (tolerance2 != "") {
    display.print(", "+String(char(240)));
    display.print(tolerance2+"%rH");
  }
  display.setTextSize(2);
}

