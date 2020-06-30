#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <esp_wifi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include <TimeLib.h>
#include "board_def.h"
#include "info.h"
#include "images.h"

#define WIFI_SSID "xxxx" // SSID of your WiFi Network 
#define WIFI_PASSWD "xxxx"   // Password of your WiFi Network
#define SERVER "1.2.3.4"        // IP or Hostname of your MQTT Server
#define SERVERPORT 1883            // Port of your MQTT Server
#define MQTT_USERNAME "xxxx"        // Username for your MQTT Server
#define MQTT_KEY "xxxx"             // Password for your MQTT Server
#define MQTT_TopicName "myTIER"    // MQTT Topic
#define BATTERY_KM 35              // Capacity of battery in km
#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 1800         // Time ESP32 will go to sleep (in seconds)
#define CHANNEL_0 0
#define IP5306_ADDR 0X75
#define IP5306_REG_SYS_CTL0 0x00
RTC_DATA_ATTR int bootCount = 0;

// Openweather API Url: change API key and coordinates
const String openweatherUrl = "http://api.openweathermap.org/data/2.5/onecall?lat=xxxx&lon=xxxx&exclude=current,minute,hourly&lang=de&&units=metric&appid=xxxxxxxxxxxxxxxxxxx";

//WifiConnectionTimeout
bool WifiNotConnected = true;
int WifiConnectionTimeoutCounter = 0;
bool msgArrived = false;
byte *old_payload;

WiFiClient wificlient;
// create MQTT object
PubSubClient client(wificlient);
eScooter myTier;
OpenWeatherMap openWeatherMap;

GxIO_Class io(SPI, ELINK_SS, ELINK_DC, ELINK_RESET);
GxEPD_Class display(io, ELINK_RESET, ELINK_BUSY);

void getWeather()
{
  HTTPClient https;
  Serial.println("[HTTPS] begin...");
  https.begin(openweatherUrl);
  Serial.print("[HTTPS] GET...");
  int httpCode = https.GET();
  if (httpCode > 0)
  {
    Serial.printf("code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK)
    {
      String response = https.getString();
      //Serial.println(response);
      openWeatherMap = parseCurrentJson(response);
     }
  }
  else
  {
    Serial.printf("failed, error: %s\n", https.errorToString(httpCode).c_str());
  }
  https.end();
}

void displayText(const String &str, int16_t y, uint8_t alignment)
{
  int16_t x = 0;
  int16_t x1, y1;
  uint16_t w, h;
  display.setCursor(x, y);
  display.getTextBounds(str, x, y, &x1, &y1, &w, &h);

  switch (alignment)
  {
  case RIGHT_ALIGNMENT:
    display.setCursor(display.width() - w - x1, y);
    break;
  case LEFT_ALIGNMENT:
    display.setCursor(0, y);
    break;
  case CENTER_ALIGNMENT:
    display.setCursor(display.width() / 2 - ((w + x1) / 2), y);
    break;
  default:
    break;
  }
  display.println(str);
}

void displayInit(void)
{
  static bool isInit = false;
  if (isInit)
  {
    return;
  }
  isInit = true;
  display.init();
  display.setRotation(1);
  display.eraseDisplay();
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&DEFAULT_FONT);
  display.setTextSize(0);
}

const unsigned char* setWeatherIcon(String icon)
{
  if(icon == "01d"){ return weather1; } 
    else if(icon == "02d"){ return weather3; } 
    else if(icon == "03d"){ return weather5; } 
    else if(icon == "04d"){ return weather7; } 
    else if(icon == "09d"){ return weather9; } 
    else if(icon == "10d"){ return weather11; } 
    else if(icon == "11d"){ return weather13; } 
    else if(icon == "13d"){ return weather14; } 
    else if(icon == "50d"){ return weather16; } 
    else if(icon == "01n"){ return weather2; } 
    else if(icon == "02n"){ return weather4; } 
    else if(icon == "03n"){ return weather6; } 
    else if(icon == "04n"){ return weather8; } 
    else if(icon == "09n"){ return weather10; } 
    else if(icon == "10n"){ return weather12; } 
    else if(icon == "11n"){ return weather14; } 
    else if(icon == "13n"){ return weather16; } 
  else return notavailable;
  }
  

  void showMainPage(void)
  {
    displayInit();
    display.fillScreen(GxEPD_WHITE);
    String lockState;
    int posY = 0;
    (myTier.lock) ? (lockState = "locked") : (lockState = "unlocked");
    displayText(String(myTier.snr), posY+=20, RIGHT_ALIGNMENT);
    displayText("Lock:", posY+=20, CENTER_ALIGNMENT);
    displayText("Batt:", posY+=20, CENTER_ALIGNMENT);
    displayText("Trip:", posY+=20, CENTER_ALIGNMENT);
    displayText("Left:", posY+=20, CENTER_ALIGNMENT);
    displayText("Total:", posY+=20, CENTER_ALIGNMENT);
    posY = 0;
    displayText(String(lockState), posY+=40, RIGHT_ALIGNMENT);
    displayText(String(myTier.battery) + " %", posY +=20, RIGHT_ALIGNMENT);
    displayText(String(myTier.trip) + " km", posY +=20, RIGHT_ALIGNMENT);
    displayText(String(myTier.battery * BATTERY_KM / 100) + " km", posY +=20, RIGHT_ALIGNMENT);
    displayText(String(myTier.total) + " km", posY +=20, RIGHT_ALIGNMENT);

    //weatherdata
    String icon = openWeatherMap.getIcon();
    displayText("Today "+ String(day(date))+"."+ String(month(date))+"."+String(year(date)), 20, LEFT_ALIGNMENT);
    displayText(String(openWeatherMap.getTempMax())+"  C", 40, LEFT_ALIGNMENT);
    //write °C Logo
    display.fillCircle(51, 30, 2, GxEPD_BLACK);  //Xpos,Ypos,r,color
    display.fillCircle(51, 30, 1, GxEPD_WHITE);  //Xpos,Ypos,r,color
    displayText(String(openWeatherMap.getMain()), 60, LEFT_ALIGNMENT);
    display.drawBitmap(setWeatherIcon(openWeatherMap.getIcon()), 10, 80, 40, 40, GxEPD_WHITE);
    display.update();
  }

  bool setPowerBoostKeepOn(int en)
  {
    Wire.beginTransmission(IP5306_ADDR);
    Wire.write(IP5306_REG_SYS_CTL0);
    if (en)
      Wire.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
    else
      Wire.write(0x35); // 0x37 is default reg value
    return Wire.endTransmission() == 0;
  }

  void mqttCallback(char *topic, byte *payload, unsigned int length)
  {
    if (old_payload != payload)
    {
      StaticJsonDocument<256> doc;
      deserializeJson(doc, payload, length);
      myTier.snr = doc["SNR"];
      myTier.lock = doc["Lock"];
      myTier.speed = doc["Speed"];
      myTier.trip = doc["Trip"];
      myTier.total = doc["Total"];
      myTier.consumption = doc["mAh"];
      myTier.battery = doc["Bat"];
      myTier.led = doc["LED"];
      msgArrived = true;
      old_payload = payload;
    }
  }

  void connect_wifi()
  {
    // Connect to WiFi network
    WiFi.begin(WIFI_SSID, WIFI_PASSWD);
    Serial.println("");

    // Wait for WiFi connection
    while (WiFi.status() != WL_CONNECTED)
    {
      WifiConnectionTimeoutCounter++;
      digitalWrite(2, LOW);
      delay(200);
      digitalWrite(2, HIGH);
      Serial.print(".");
      // If WiFi connection time out -> Retry!
      if (WifiConnectionTimeoutCounter > 50)
      {
        WifiNotConnected = true;
        Serial.println("WiFi Connection Timeout.... Retrying!");
        WifiConnectionTimeoutCounter = 0;
        break;
      }
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      WifiNotConnected = false;
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(WIFI_SSID);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      delay(1000);
    }
  }

  void print_wakeup_reason()
  {
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason)
    {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Wakeup caused by external signal using RTC_IO");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("Wakeup caused by external signal using RTC_CNTL");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Wakeup caused by timer");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      Serial.println("Wakeup caused by touchpad");
      break;
    case ESP_SLEEP_WAKEUP_ULP:
      Serial.println("Wakeup caused by ULP program");
      break;
    default:
      Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
      break;
    }
  }

  void setup()
  {
    // put your setup code here, to run once:
    Serial.begin(115200);

    //Increment boot number and print it every reboot
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));

    //Print the wakeup reason for ESP32
    print_wakeup_reason();

    //First we configure the wake up source: We set our ESP32 to wake up every 5 seconds
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

    client.setServer(SERVER, SERVERPORT);
    client.setCallback(mqttCallback);

    //Connect WiFi
    while (WifiNotConnected)
    {
      connect_wifi();
      delay(500);
    }

    // Connect to MQTT Server
    if (!client.connected())
    {
      Serial.println("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect("", MQTT_USERNAME, MQTT_KEY))
      {
        Serial.println("connected");
        //... and resubscribe
        client.subscribe(MQTT_TopicName, 1);
      }
      else
      {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    while (!msgArrived)
    {
      client.loop();
    }
    client.disconnect();

    //show data on display
    getWeather();
    showMainPage();

    Serial.println("Going to sleep now");
    Serial.flush();
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
  }

  void loop()
  {
    // put your main code here, to run repeatedly:
  }