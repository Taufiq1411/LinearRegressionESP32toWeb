#include <WiFi.h>
#include <FirebaseESP32.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   
#include <ArduinoJson.h>

#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#define WIFI_SSID "KAV 17A"
#define WIFI_PASSWORD "jolalibayarkos"

#define BOTtoken "5974115257:AAEvjaFCx3SYc0VPdgbIQS0iL8EozdIBgtA"
#define CHAT_ID "1719945004"

#define FIREBASE_HOST "https://react-firebase-esp32-default-rtdb.asia-southeast1.firebasedatabase.app/" 
#define FIREBASE_SECRET "VDeqtxfuSdihq7yuXS5wsoHnx22e73odtINOVKF1"

#define Volt_Sensor 34

FirebaseData fbdo;
unsigned long nMillis = 0;
float v=0, vADC=0, vReal=0, voltSum=0, vRead=0, range=0;
float vReadBeforeStart=0, dif=0;
int p=0, countS=0, countActive=0;

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

struct dataset {
   float waktu;
   float tegangan;
  };

dataset UPS[] = {
    {1, 12.33},
    {2, 12.16},
    {3, 12.03},
    {4, 11.88},
    {5, 11.82},
    {6, 11.74},
    {7, 11.69},
    {8, 11.53},
    {9, 11.49},
    {10, 11.39},
    {11, 11.29},
    {12, 11.21},
    {13, 11.07},
    {14, 10.90},
    {15, 10.52},
    {16, 10.15},
};

void regresiLinear(float vNow, int *p){
  float sumXi=0, sumXikuadrat=0, sumXibarudikuadrat=0, x=1;
  float sumYi=0, sumYikuadrat=0, sumXiYi=0, a=0, b=0, y=0;
  int n=16;
  for (int i = 0; i < n; i++)
  {
    sumXi += UPS[i].waktu;
    sumYi += UPS[i].tegangan; 
    sumXikuadrat += UPS[i].waktu*UPS[i].waktu;
    sumYikuadrat += UPS[i].tegangan*UPS[i].tegangan;
    sumXiYi += UPS[i].waktu*UPS[i].tegangan;
    sumXibarudikuadrat = sumXi*sumXi;
  }
  a = (((sumYi*sumXikuadrat)-(sumXi*sumXiYi))/((n*sumXikuadrat)-sumXibarudikuadrat));
  b = (((n*sumXiYi)-(sumXi*sumYi))/((n*sumXikuadrat)-sumXibarudikuadrat));

  float vCrit=0, tCrit=0, tNow=0, prediksi=0;
  vCrit = a+(b*16);
  tCrit = round((vCrit-a)/b);
  tNow = (vNow-a)/b;
  prediksi = tCrit-tNow;

  *p = round(prediksi);
  Serial.print("Prediksi sisa waktu nyala : ");
  Serial.println(*p);

  if (vNow <= 12.8) {
    for (int i = 0; i < 15; i++)
    {
      UPS[i].waktu = UPS[i+1].waktu;
      UPS[i].tegangan = UPS[i+1].tegangan;
    }
    UPS[15].waktu = tNow;
    UPS[15].tegangan = vNow;
    }
  else if (vNow > 12.8) {
    dataset UPS[] = {
    {1, 12.33},
    {2, 12.16},
    {3, 12.03},
    {4, 11.88},
    {5, 11.82},
    {6, 11.74},
    {7, 11.69},
    {8, 11.53},
    {9, 11.49},
    {10, 11.39},
    {11, 11.29},
    {12, 11.21},
    {13, 11.07},
    {14, 10.90},
    {15, 10.52},
    {16, 10.15},
    };
  }
}

void setup(){
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    while (WiFi.status() != WL_CONNECTED){
        delay(300);
    }
    Firebase.begin(FIREBASE_HOST, FIREBASE_SECRET);
}

void loop(){
  if (millis() - nMillis > 100){
    nMillis = millis();
    v = analogRead(Volt_Sensor);
    vReal = (v*(16.5/4095))+0.27; 
      if (v = 0) {
        vReal = 0;}
    voltSum += vReal;
    countS++;
  }

  if (countS == 100){
    vRead = voltSum/100;
    Serial.print("Tegangan : ");
    Serial.println(vRead);
    if (vRead >= 13) {                     
      dif = vReadBeforeStart - vRead;
      if (dif > 0.15) { 
        if (countActive == 0) {
          bot.sendMessage(CHAT_ID, "Terjadi Pemadaman Listrik, lihat laman https://react-firebase-esp32.web.app/ untuk mengetahui perkiraan sisa waktu nyala", "");
          countActive = 1;  
        }
      } else {
        vReadBeforeStart = vRead;
      }
    }
    voltSum = 0;
    countS = 0;
    
    regresiLinear(vRead, &p);
     if (Firebase.ready()){ 
      String path1 = "/UsersData/tegangan";
      String path2 = "/UsersData/prediksi";
      Firebase.RTDB.setFloat(&fbdo, path1, vRead);
      Firebase.RTDB.setInt(&fbdo, path2, p);
    }
    if (vRead < 11.5) {
      String predictTime = "Sisa waktu nyala UPS adalah ";
      predictTime += String(p);
      predictTime += " menit";
      bot.sendMessage(CHAT_ID, predictTime, "");
    }
  } 
}
