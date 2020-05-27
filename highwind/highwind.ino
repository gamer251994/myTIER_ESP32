#include "BLEDevice.h"
//#include "BLEScan.h"
#include <odroid_go.h>

static String BTPASSWD = "xxxxxx";    //Das Bluetooth-Passwort, welches bei den Kommandos mitgesendet wird
#define MyTier "XX:XX:XX:XX:XX:XX"        //Bluetooth-Adresse des Scooters, diese mit einem Bluetooth LE Scanner herausfinden und hier rein

static BLEUUID serviceUUID("00002c00-0000-1000-8000-00805f9b34fb"); //Service des Tier-Scooters
static BLEUUID    charUUID("00002c10-0000-1000-8000-00805f9b34fb"); //Charakteristik.. Hier landen die Befehle.

static int LockStatus;
static  float Speed;
static  float TripDistance;
static  float TripTotal;
static  int OddCounter;
static  int Battery;
static  int Headlight;

static boolean doConnect = false;
static boolean connected = false;
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
      lastStatus.replace("+ACK:BKINF,","");
      lastStatus.replace("\r\n","");
      lastStatus.replace("$","");     //Müll entfernen
      //+ACK:BKINF=LockStatus,Speed,TripDistance,TripTotal,OddCounter,Battery,Active$ 
      int spdvk, spdnk, tripvk, tripnk, triptvk, triptnk; //float stinkt
//      sscanf(lastStatus.c_str(), "%d,%.1f,%.2f,%.1f,%d,$d,%d",&LockStatus,&Speed,&TripDistance,&TripTotal,&OddCounter,&Battery,&Active);  //Geil: sscanf kann hier kein float.
      sscanf(lastStatus.c_str(), "%d,%d.%d,%d.%d,%d.%d,%d,%d,%d",&LockStatus,&spdvk, &spdnk, &tripvk, &tripnk, &triptvk, &triptnk,&OddCounter,&Battery,&Headlight);
      Speed = (float) spdvk + ((float) spdnk / 10); //20.1 kmh
      TripDistance = (float) tripvk + ((float) tripnk / 100); //30.33 km distanz
      TripTotal = (float) triptvk + ((float) triptnk / 10); //1666.6 km bisher
      GO.lcd.clear();
      GO.lcd.setCursor(0, 0);
//      GO.lcd.println("["+lastStatus+"]");
      GO.lcd.printf("Sperre: %s\n",(LockStatus) ? "Ein": "Aus" );
      GO.lcd.printf("Tacho: %.1f km/h\n",Speed);
      GO.lcd.printf("Trip: %.2f km\n",TripDistance);
      GO.lcd.printf("Trip Gesamt: %.1f km\n",TripTotal);
      GO.lcd.printf("Unbekannt: %d\n",OddCounter );
      GO.lcd.printf("Batterie: %d %\n",Battery);
      GO.lcd.printf("Frontleuchte: %s\n",(Headlight)? "Ein": "Aus");
      rcvStatus = false;
    }else{
      if(cmd.substring(5,10) == "BKINF"){
         lastStatus = (char*) pData;       
         rcvStatus = true;
      }else{
        GO.lcd.println("Empfangen: " + cmd);
      }
    }
//    GO.lcd.println(rcvd.substring(5,10));
//    GO.lcd.print("<< ");
//    GO.lcd.println((char*)pData);
    
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    GO.lcd.println("Verbindung getrennt..");
    delay(2000);
    doConnect = true;
  }
};

bool connectToServer() {
    GO.lcd.print("Verbinde... ");    
    BLEClient*  pClient  = BLEDevice::createClient();

    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(BLEAddress(MyTier));  //Mac Adresse des Scooters
    GO.lcd.println("...Verbunden");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      GO.lcd.print("Service UUID nicht gefunden: ");
      GO.lcd.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    GO.lcd.println("Service gefunden.");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      GO.lcd.print("Charakteristik UUID nicht gefunden: ");
      GO.lcd.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    GO.lcd.println("Charakteristik gefunden");

    if(pRemoteCharacteristic->canNotify())    //für Antworten Notification aktivieren
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}

void setup() {
  GO.begin();
  GO.lcd.setTextSize(2);
  GO.lcd.clear();
  GO.lcd.setCursor(0, 0);
  GO.lcd.println("Highwind v 0.1");   //FF7-Fans hier? :)
  BLEDevice::init("");
  doConnect = true;

} // End of setup.


// This is the Arduino main loop function.
void loop() {

  if (doConnect == true) {
    if (connectToServer()) {
      GO.lcd.println("Los gehts.");
    } else {
      GO.lcd.println("Verbindung fehlgeschlagen");
    }
    doConnect = false;
  }

  if (connected) {
//    GO.lcd.clear();
//    GO.lcd.setCursor(0, 0);
    String newValue1;
    String newValue2;
//    String cmd;
    GO.update();  //Registriert Tastendrücke..
    if(GO.BtnA.isPressed() && (GO.JOY_Y.isAxisPressed() == 2)){  //Joy up (Entsperren)
       newValue1 = "AT+BKSCT=" + BTPASSWD +",";
       newValue2 = "0$\r\n";
//       cmd = "Entsperren!";
    }else if(GO.BtnA.isPressed() && (GO.JOY_Y.isAxisPressed() == 1)){ //Joy down (Sperren)
       newValue1 = "AT+BKSCT=" + BTPASSWD +",";
       newValue2 = "1$\r\n";
//       cmd = "Sperren!";
    }else if(GO.BtnA.isPressed() && (GO.JOY_X.isAxisPressed() == 2)){ //Joy left (Licht an)
       newValue1 = "AT+BKLED=" + BTPASSWD +",";
       newValue2 = "0,1$\r\n";
//       cmd = "Licht an!";
    }else if(GO.BtnA.isPressed() && (GO.JOY_X.isAxisPressed() == 1)){ //joy right (Licht aus)
       newValue1 = "AT+BKLED=" + BTPASSWD +",";
       newValue2 = "0,0$\r\n";
//       cmd = "Licht aus!";
    }else if(GO.BtnB.isPressed() && (GO.JOY_X.isAxisPressed() == 1)){ //Ich lass dies hier drin um zu testen..
       newValue1 = "AT+BKLED=" + BTPASSWD +",";
       newValue2 = "1,1$\r\n";
//       cmd = "TEST COMMAND";
    }else{
       newValue1 = "AT+BKINF=" + BTPASSWD +",";
       newValue2 = "0$\r\n";
//      cmd = "Info!!!";
    }
  
//    GO.lcd.println(cmd);

    //Kommando und Passwort sind länger als 20 Bytes. Deswegen beide Teile direkt hintereinander kloppen.
    pRemoteCharacteristic->writeValue(newValue1.c_str(), newValue1.length());
    pRemoteCharacteristic->writeValue(newValue2.c_str(), newValue2.length());
  }  
  delay(500);
} // End of loop
