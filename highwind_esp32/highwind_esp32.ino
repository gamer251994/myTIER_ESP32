#include "BLEDevice.h"

static String BTPASSWD = "xxxxx";    //Das Bluetooth-Passwort, welches bei den Kommandos mitgesendet wird
#define MyTier "xx:xx:xx:xx:xx:x"        //Bluetooth-Adresse des Scooters, diese mit einem Bluetooth LE Scanner herausfinden und hier rein

#define WakeupPins 0x8007000 //Wakeup: gpio 13, 12, 14, 27 up

static BLEUUID serviceUUID("00002c00-0000-1000-8000-00805f9b34fb"); //Service des Tier-Scooters
static BLEUUID    charUUID("00002c10-0000-1000-8000-00805f9b34fb"); //Charakteristik.. Hier landen die Befehle.

static boolean rcvStatus = false;
static String lastStatus = "";  

static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

static void notifyCallback( //Notifications vom Characteristic
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    pData[length] = '\0';//Nullbyte hinzufügen. Sonst haben wir Müll 
    String cmd = (char*) pData;
    if(rcvStatus){ //Befehle wie AT+BKINF wird immer in 2 teilen empfangen. Ich will diesen damit abfangen, um alles zu empfangen. 
      lastStatus = lastStatus + (char*) pData; 
      Serial.println("teil2:["+lastStatus+"]");
      lastStatus.replace("+ACK:BKINF,","");
      lastStatus.replace("\r\n","");
      lastStatus.replace("$","");     //Müll entfernen

      rcvStatus = false;
    }else{
      if(cmd.substring(5,10) == "BKINF"){
         lastStatus = (char*) pData;       ;
         rcvStatus = true;
      }
    }

   Serial.print("<< ");
   Serial.println((char*)pData);
    //Stattdessen könnte man hier auch eine LED einschalten
}

void sendCommand(int pin){
    Serial.print(pin);
    String newValue1;
    String newValue2;
    switch(pin){
      case 12:   //Entsperren
        newValue1 = "AT+BKSCT=" + BTPASSWD +",";
        newValue2 = "0$\r\n";
      break;
      case 13:   //Sperren
       newValue1 = "AT+BKSCT=" + BTPASSWD +",";
       newValue2 = "1$\r\n";
      break;
      case 14:   //Licht an
       newValue1 = "AT+BKLED=" + BTPASSWD +",";
       newValue2 = "0,1$\r\n";
      break;
      case 27:   //Licht aus
       newValue1 = "AT+BKLED=" + BTPASSWD +",";
       newValue2 = "0,0$\r\n";
      break;
      default:    //Infos holen. Mein ESP hat manchman geglitcht und die GPIO Nummer 2673089463 oder so zurückgegeben.
       newValue1 = "AT+BKINF=" + BTPASSWD +",";
       newValue2 = "0$\r\n";
      break;
      
    }
    //Kommando und Passwort sind länger als 20 Bytes. Deswegen beide Teile direkt hintereinander kloppen.
    pRemoteCharacteristic->writeValue(newValue1.c_str(), newValue1.length());
    pRemoteCharacteristic->writeValue(newValue2.c_str(), newValue2.length());
 
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    Serial.println("Verbindung getrennt!! Schlafen.");
    delay(200);
    esp_deep_sleep_start();
  }
};

bool connectToServer() {
    Serial.print("Verbinde... ");    
    BLEClient*  pClient  = BLEDevice::createClient();

    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(BLEAddress(MyTier));  //Mac Adresse des Scooters
    Serial.println("...Verbunden");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
//      Serial.print("Service UUID nicht gefunden: ");
//      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
//    Serial.println("Service gefunden.");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
//      Serial.print("Charakteristik UUID nicht gefunden: ");
//      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
//    Serial.println("Charakteristik gefunden");

    if(pRemoteCharacteristic->canNotify())    //für Antworten Notification aktivieren
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    int GPIONum = esp_sleep_get_ext1_wakeup_status(); //Pin Nummer rausfinden
    int pin = (log(GPIONum))/log(2);
    sendCommand(pin);      //Kommando
    delay(1000);  //Gelegenheit eine Antwort zu senden geben.
    pClient->disconnect();
    return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Highwind v 0.2 ESP32");   //FF7-Fans hier? :)
  esp_sleep_enable_ext1_wakeup(WakeupPins, ESP_EXT1_WAKEUP_ANY_HIGH);

  esp_sleep_wakeup_cause_t wakeup = esp_sleep_get_wakeup_cause();
  if(wakeup == ESP_SLEEP_WAKEUP_EXT1){
    BLEDevice::init("");
    connectToServer();   //mit dem Scooter verbinden
  }else{
    Serial.println("Kein GPIO Wakeup. Gute Nacht");
    esp_deep_sleep_start(); //Gute nacht
  }

} // End of setup.


void loop() {
   //Hier könnte Ihre Werbung stehen 
}
