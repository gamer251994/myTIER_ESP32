/*
myTIER Scooter Status - MQTT Sensor (Dual Scooters)
=====================================================================================
This sketch implements a way to query the status of two myTIER electric  Scooters via
Bluetooth Low Energy (BLE) using a ESP32. The received status messages are transcoded 
into a JSON string which is in turn transfered embedded in a MQTT message via WiFi to
an arbitary MQTT Server (such as a Home-Automation Server, e.g. Node-RED)
A Node-RED demo flow is provided with this sketch.

This code is under Public Domain License.

Author: Thomas Berndt <tomyb@gmx.net>
=====================================================================================
*/

#include "BLEDevice.h"
#include <esp_wifi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

#define WIFI_SSID                   "xxx"                             /* SSID of your WiFi Network */
#define WIFI_PASSWD                 "xxx"                             /* Password of your WiFi Network */
#define SERVER                      "xxx.xxx.xxx.xxx"                 /* IP or Hostname of your MQTT Server */
#define SERVERPORT                  1883                              /* Pot of your MQTT Server */
#define MQTT_USERNAME               "xxx"                             /* sername for your MQTT Server */
#define MQTT_KEY                    "xxx"                             /* Password for your MQTT Server */
#define MQTT_TopicName              "myTIER"                          /* MQTT Topic */
#define uS_TO_S_FACTOR 1000000ULL                                     /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  30                                             /* Time ESP32 will go to sleep (in seconds) */


RTC_DATA_ATTR int bootCount = 0;
BLEAddress myTier1 = BLEAddress("xx:xx:xx:xx:xx:xx");                 /* Bluetooth MAC Address of Scooter 1 */
BLEAddress myTier2 = BLEAddress("xx:xx:xx:xx:xx:xx");                 /* Bluetooth MAC Address of Scooter 2 */
static String BTPASSWD1 = "xxxxxxxxxx";                               /* Bluetooth passwort of Scooter 1 */ 
static String BTPASSWD2 = "xxxxxxxxxx";                               /* Bluetooth passwort of Scooter 2 */
static String myTier1Name = "ABxxxxxx";                               /* Serial Number of Scooter 1 */
static String myTier2Name = "ABxxxxxx";                               /* Serial Number of Scooter 1 */
static BLEUUID serviceUUID("00002c00-0000-1000-8000-00805f9b34fb");   /* Service ogf the TIER-Scooters */
static BLEUUID    charUUID("00002c10-0000-1000-8000-00805f9b34fb");   /* Charakteristic of the TIER-Scooters - This is where we send our commands to */
static boolean rcvStatus = false;
static String lastStatus = "";  
static String myTier1Status = "";
static String myTier2Status = "";
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
char valueStr[128];


void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

//WifiConnectionTimeout
boolean WifiNotConnected = true;
int WifiConnectionTimeoutCounter = 0;

WiFiClient WiFiClient;
// create MQTT object
PubSubClient client(WiFiClient);


void connect_wifi(){
    // Connect to WiFi network
    WiFi.begin(WIFI_SSID, WIFI_PASSWD);
    Serial.println("");

    // Wait for WiFi connection
    while (WiFi.status() != WL_CONNECTED) {
        WifiConnectionTimeoutCounter++;
        digitalWrite(2, LOW);
        delay(200);
        digitalWrite(2, HIGH);
        Serial.print(".");
        // If WiFi connection time out -> Retry!
        if (WifiConnectionTimeoutCounter > 50){
          WifiNotConnected = true;
          Serial.println("WiFi Connection Timeout.... Retrying!");
          WifiConnectionTimeoutCounter = 0;
          break;
        }
    }
    if (WiFi.status() == WL_CONNECTED){
      WifiNotConnected = false;
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(WIFI_SSID);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      delay(1000);
    }
}

static void notifyCallback(                                           /* Notifications of the BLE-Characteristic */
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    pData[length] = '\0';                                             /* Add Nullbyte, otherwise we get trash */
    String cmd = (char*) pData;
    if(rcvStatus){                                                    /* Commands like AT+BKINF are always received in 2 parts (max 20 Bytes). This commbines them! */
      lastStatus = lastStatus + (char*) pData; 
      lastStatus.replace("+ACK:BKINF,","");
      lastStatus.replace("\r\n","");
      lastStatus.replace("$","");                                     /* Remove trash */

      rcvStatus = false;
    }else{
      if(cmd.substring(5,10) == "BKINF"){
         lastStatus = (char*) pData;       ;
         rcvStatus = true;
      }
    }
   //Serial.print("<< ");
   //Serial.println((char*)pData);
}


class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    Serial.println("Connection terminated!!");
    delay(200);
    //esp_deep_sleep_start();
  }
};

bool connectToServer(BLEAddress myTier, String password) {
    Serial.print("Connecting... ");    
    BLEClient*  pClient  = BLEDevice::createClient();

    pClient->setClientCallbacks(new MyClientCallback());
    if(pClient->connect(myTier)){;  //Mac Adresse des Scooters
      Serial.println("...CONNECTED!");
      // Obtain a reference to the service we are after in the remote BLE server.
      BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
      if (pRemoteService == nullptr) {
        Serial.print("Service UUID not found: ");
        Serial.println(serviceUUID.toString().c_str());
        pClient->disconnect();
        return false;
      }
      Serial.println("Found Service!");

      // Obtain a reference to the characteristic in the service of the remote BLE server.
      pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
      if (pRemoteCharacteristic == nullptr) {
        Serial.print("Charakteristik UUID not found: ");
        Serial.println(charUUID.toString().c_str());
        pClient->disconnect();
        return false;
      }
      Serial.println("Found Charakteristic!");

      if(pRemoteCharacteristic->canNotify())                        /* For receiving Answers activate Notifications */
        pRemoteCharacteristic->registerForNotify(notifyCallback);
    
      String commandValue1 = "AT+BKINF=" + password +",";
      String commandValue2 = "0$\r\n";
      
      //Command and Passwort exceed 20 Bytes. That is why we send both parts immediately after each other.
      pRemoteCharacteristic->writeValue(commandValue1.c_str(), commandValue1.length());
      pRemoteCharacteristic->writeValue(commandValue2.c_str(), commandValue2.length());
      delay(1000);                                                  /* Give some time to receive the answer */
      pClient->disconnect();
      return true;
    }else{
      Serial.println("...Connection Failed");
      return false;
    }
}

String convert2JSON(String myTierName, String myTierStatus){
 /* Parse Param1 : LockStatus */
  int endOfParam1 = myTierStatus.indexOf(",");
  String param1 = myTierStatus.substring(0,endOfParam1);
  myTierStatus = myTierStatus.substring(endOfParam1+1);
  /* Parse Param2 : Speed */
  int endOfParam2 = myTierStatus.indexOf(",");
  String param2 = myTierStatus.substring(0,endOfParam2);
  myTierStatus = myTierStatus.substring(endOfParam2+1);
  /* Parse Param3 : Trip */
  int endOfParam3 = myTierStatus.indexOf(",");
  String param3 = myTierStatus.substring(0,endOfParam3);
  myTierStatus = myTierStatus.substring(endOfParam3+1);
  /* Parse Param4 : Total */
  int endOfParam4 = myTierStatus.indexOf(",");
  String param4 = myTierStatus.substring(0,endOfParam4);
  myTierStatus = myTierStatus.substring(endOfParam4+1);
  /* Parse Param5 : mAh */
  int endOfParam5 = myTierStatus.indexOf(",");
  String param5 = myTierStatus.substring(0,endOfParam5);
  myTierStatus = myTierStatus.substring(endOfParam5+1);
  /* Parse Param6 : Bat */
  int endOfParam6 = myTierStatus.indexOf(",");
  String param6 = myTierStatus.substring(0,endOfParam6);
  myTierStatus = myTierStatus.substring(endOfParam6+1);
  /* Parse Param7 : LED */
  String param7 = myTierStatus;
  //convert to JSON Format
  String json = "{ \"SNR\": \"" + myTierName +"\", \"Lock\": " + param1 + ", \"Speed\": "+ param2 +", \"Trip\": "+ param3 +", \"Total\": "+ param4 +", \"mAh\": "+ param5 +", \"Bat\": "+ param6 +", \"LED\": "+ param7 +" }";
  return json;  
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  print_wakeup_reason();
 
  //First we configure the wake up source: We set our ESP32 to wake up every 5 seconds
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

  client.setServer(SERVER, SERVERPORT);
  client.setCallback(mqtt_callback);
  
  //Connect WiFi
  while(WifiNotConnected){
    connect_wifi();
    delay(500);
  }

  // Connect to MQTT
  //Serial.println("Connecting to MQTT");
  yield();
  
  if(!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("", MQTT_USERNAME, MQTT_KEY)) {
      Serial.println("connected");
       //... and resubscribe
      client.subscribe("myTIER", 1);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  Serial.println("-=============================================-");
  Serial.println(" MQTT Sensor: myTIER_Batteries (Dual Scooters) ");
  Serial.println("-=============================================-");
  BLEDevice::init("");
  Serial.println("Retreiving Data from myTIER #1 : "+myTier1Name);
  lastStatus="";
  connectToServer(myTier1, BTPASSWD1);                              /* Connect to 1st Scooter */
  String jsonStr = convert2JSON(myTier1Name, lastStatus);
  Serial.println(jsonStr);
  if (client.connected() && lastStatus!=""){    
    jsonStr.toCharArray(valueStr, 128);
    client.publish("myTIER", valueStr);
    delay(500);       
  }

  delay(2000);
  
  Serial.println("Retreiving Data from myTIER #2 : "+myTier2Name);
  lastStatus="";
  connectToServer(myTier2, BTPASSWD2);                               /* Connect to 2nd Scooter */
  jsonStr = convert2JSON(myTier2Name, lastStatus);
  Serial.println(jsonStr);
  if (client.connected() && lastStatus!=""){    
    jsonStr.toCharArray(valueStr, 128);
    client.publish("myTIER", valueStr);
    delay(500);       
  }

  client.disconnect();
  WiFi.mode(WIFI_OFF);
  btStop();
  delay(500);

  Serial.println("Going to sleep now");
  Serial.flush(); 
  esp_deep_sleep_start();
  Serial.println("This will never be printed");

} // End of setup.

void loop() {
                                                                    /* Empty Loop */ 
}

void mqtt_callback(char* topic, byte * data, unsigned int length) {
  // handle message arrived {
    String topicString = String(topic);
    
  if (topicString == MQTT_TopicName){
    Serial.print(topic);
    Serial.print(": "); 
    for (int i = 0; i < length; i++) {
      Serial.print((char)data[i]);
    }
    Serial.println();
    if (data[1] == 'A')  {
      Serial.println("STATUS: A");      
    } else {
      Serial.println("STATUS: B");
    }
  }  
}
