
/*** Librerias ***/
#include <WiFi.h>
#include "time.h"
#include "Freenove_WS2812_Lib_for_ESP32.h"
#include <FirebaseESP32.h>
#include <Adafruit_BMP280.h>


/***Variables Globales***/

volatile int interruptCounter ; //Contador de interrupcion TIMER
bool flagWiFi = false; // Bandera para detectar el estado del WI-FI
int cantMed =0;
String dato = "0";
float temp =0;
float pres =0;
String fecha ="";
String hora ="11:11";

/*** Constantes ***/
const byte btn = 27; // Boton para carga de termo
const byte bombaOut = 12; //Salida Bomba Agua

/*** LED ***/

#define LEDS_COUNT  1
#define LEDS_PIN  23
#define CHANNEL   0
Freenove_ESP32_WS2812 strip = Freenove_ESP32_WS2812(LEDS_COUNT, LEDS_PIN, CHANNEL, TYPE_GRB);

/*** Wi-Fi ***/

#define WIFI_SSID "Casa"
#define WIFI_PASSWORD "algaleem64659296"

/*** RTC - NTP Server ***/

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -3*60*60;
const int   daylightOffset_sec = 3600;
struct tm timeinfo;

void printLocalTime()
{
  //struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  hora ="";
  fecha ="";
  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  //hora = timeinfo.tm_hour;
  //hora = hora + ":" + String(timeinfo.tm_min);
  int Year = timeinfo.tm_year - 100;
  int Month = timeinfo.tm_mon +1;
  hora = String(timeinfo.tm_hour)+ ":" + String(timeinfo.tm_min);
  fecha = String(timeinfo.tm_mday) + "/" + String(Month) + "/" + String(Year);
  
  //Serial.println("La Hora es: "+ hora);
   //Serial.println("La Fecha es: "+ fecha);
 

}

/*** Firebase ***/
String placaId = "1"; // Numero de identificacion de placas
#define FIREBASE_HOST "termosolaresp-default-rtdb.firebaseio.com" //Sin http:// o https:// 
#define FIREBASE_AUTH "aVAbd08k2Bpjrx8kto7zY41bUbBRFEMAJzUtzhyr"
FirebaseData firebaseData;
String path = "/EstacionMatera/Placa: " + placaId ;





/***Configuracion de interrupcion  (timer)***/

  hw_timer_t * timer = NULL;
  portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&timerMux);
  //Serial.println(interruptCounter);

 
}

/*** Sensro BMP 280 ***/
Adafruit_BMP280 bmp; // I2C

/*** Funcion de interrupcion pora boton carga de agua ***/
/*
 void IRAM_ATTR isr_btn_agua(){
if(flagBtn_enable){
  
  if(flagBtn_presionado){
    flagBtn_presionado = false;
    Serial.println("SE SOLTO");

   Serial.println(interruptCounter);
   tiempoDeCarga = interruptCounter;
   flagCargaTermos = true;
    }else{
      flagBtn_presionado = true;
      interruptCounter = 0;
        Serial.println("SE PRESIONO");
        
    }
}

  }
*/


void setup() {
 Serial.begin(115200);

   pinMode(btn, INPUT_PULLUP);
 // attachInterrupt(btn, isr_btn_agua,CHANGE); // Interrupcion boton de carga de agua
  pinMode(bombaOut, OUTPUT);
 
/*** Inicializar las interrupciones ***/
  timer = timerBegin(0, 800, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 100000, true);
  timerAlarmEnable(timer);

/*** Inicializar Wi-Fi  ***/

  if(flagWiFi == false){
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      Serial.print("Conectando a ....");
      interruptCounter =0;
        while (WiFi.status() != WL_CONNECTED && interruptCounter < 25)
            {
              Serial.print(".");
              delay(300);
            }
            if(WiFi.status() == WL_CONNECTED){
              flagWiFi = true;
                Serial.println();
                Serial.print("Conectado con la IP: ");
                Serial.println(WiFi.localIP());
                Serial.println();

              }else{
                Serial.println ("No se pudo conectar a la red Wi-Fi");
                }

 
    }

/*** Iniciar RTC Interno ***/

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  

/*** Configuracion de Firebase ***/

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  //Establezca el tiempo de espera de lectura de la base de datos en 1 minuto (máximo 15 minutos)
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  
  //Tamaño y  tiempo de espera de escritura, tiny (1s), small (10s), medium (30s) and large (60s).
  //tiny, small, medium, large and unlimited.

  Firebase.setwriteSizeLimit(firebaseData, "tiny");


/*** Configuracion LED ***/
  strip.begin();
  strip.setBrightness(10);  
  //strip.setLedColorData(0,255,33,17);
  //strip.show();


/*** Iniciar sensor Temp y Presion ***/
  unsigned status;
  status = bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);

  if (!status) {
    Serial.println("No se encontro sensor BMP-280");
  }
  else{
    Serial.println("Sensor BMP 280 OK!");
    }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

    

}

void MedirTempyPres(){
  temp = bmp.readTemperature();
  pres = bmp.readPressure()/100; // Resultado en hPa
  //Serial.println(temp);
 // Serial.println(pres);
  
  }
void ActualizarBD(){
  printLocalTime();
  MedirTempyPres();
  if(cantMed > 100){
    cantMed =0;
  }else{
    
    dato = "";
    dato += cantMed;
    Serial.println("Actualizando BD");
    Serial.println(dato);
    
     Firebase.setFloat(firebaseData, path + "/Mediciones/" + dato + "/Temperatura",temp);
     Firebase.setFloat(firebaseData, path + "/Mediciones/" + dato + "/Presion",pres);
     Firebase.setString(firebaseData, path + "/Mediciones/" + dato + "/Fecha",fecha);
     Firebase.setString(firebaseData, path + "/Mediciones/" + dato + "/Hora",hora);
     cantMed ++;
    }
  }


  
void loop() {

  
if(interruptCounter == 60){
  interruptCounter=0;
  //printLocalTime();
 //MedirTempyPres();
 ActualizarBD();
  
  }


  

}
 
