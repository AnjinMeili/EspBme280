// Setup for esp8266 via Wemos D1 Mini
//
// Creates a simple environment monitor for Temp, Humdity, Pressure
// Results are displayed on a 128x32 OLED, as well to a WebUI
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP8266WebServer.h>


// pin definitions
#define CS_PIN    D8
#define RST_PIN   D3
#define DC_PIN    D6
#define OLED_DATA D7
#define OLED_CLK  D5

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_DATA, OLED_CLK, DC_PIN, RST_PIN, CS_PIN);
  
// ttp225 touch sensor
#define TOUCH     D0
volatile bool touchState = LOW;

float temperature, humidity, pressure, altitude;

/*Put your SSID & Password*/
const char* ssid = "NotYourWifi";          // Enter SSID here
const char* password = "NotYourPassword";  // Enter Password here

ESP8266WebServer server(80);    
          
#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // use I2C interface
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();

//------------------------------------------------------------------------------

ICACHE_RAM_ATTR  void touched() {
  touchState = !touchState;
  // note: LOW == false == 0, HIGH == true == 1, so inverting the boolean is the same as switching between LOW and HIGH.
}

void setup() {
  Serial.begin(9600);
  pinMode(TOUCH, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TOUCH), touched, CHANGE); // trigger when button pressed, but not when released.

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  display.clearDisplay();

  if (!bme.begin(0x76)) {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    while (1) delay(10);
  }

    Serial.println("Connecting to ");
  Serial.println(ssid);

  //connect to your local wi-fi network
  WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");

  bme_temp->printSensorDetails();
  bme_pressure->printSensorDetails();
  bme_humidity->printSensorDetails();
  
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.setTextWrap(false);

}

void loop() {
  sensors_event_t temp_event, pressure_event, humidity_event;
  bme_temp->getEvent(&temp_event);
  bme_pressure->getEvent(&pressure_event);
  bme_humidity->getEvent(&humidity_event);

  display.clearDisplay();
  
  display.setCursor(0,2);     
  display.print(F("Temperature "));
  display.print((temp_event.temperature * 1.8)+32);
  display.print(" ");
  display.drawChar(display.getCursorX(),display.getCursorY(),0xF8,SSD1306_WHITE,BLACK,1);
  display.println(" F");

  display.setCursor(0,12); 
  display.print(F("Humidity    "));
  display.print(humidity_event.relative_humidity);
  display.println(" %");
  
  display.setCursor(0,22);    
  display.print(F("Pressure  ")); 
  display.print(pressure_event.pressure);
  display.println(" hPa");

  display.display();
  server.handleClient();
  delay(1000);
  }


void handle_OnConnect() {
  temperature = bme.readTemperature();
  humidity    = bme.readHumidity();
  pressure    = bme.readPressure() / 100.0F;
  altitude    = bme.readAltitude(SEALEVELPRESSURE_HPA);
  server.send(200, "text/html", SendHTML(temperature,humidity,pressure,altitude)); 
}  

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String SendHTML(float temperature,float humidity,float pressure,float altitude){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>Katy Cage Monitor</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr +="p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<div id=\"webpage\">\n";
  ptr +="<h1>Katy Cage Monitor</h1>\n";
  ptr +="<p>Temperature: ";
  ptr +=temperature;
  ptr +="&deg;C</p>";
  ptr +="<p>Humidity: ";
  ptr +=humidity;
  ptr +="%</p>";
  ptr +="<p>Pressure: ";
  ptr +=pressure;
  ptr +="hPa</p>";
  ptr +="<p>Altitude: ";
  ptr +=altitude;
  ptr +="m</p>";
  ptr +="</div>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}
