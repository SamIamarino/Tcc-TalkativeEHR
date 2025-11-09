/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete instructions at https://RandomNerdTutorials.com/esp32-firebase-realtime-database/
*********/
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include "DHT.h"
#include "Adafruit_Sensor.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>

// Network and Firebase credentials
#define WIFI_SSID "Cristiane_2.4G"
#define WIFI_PASSWORD "31071974"

//Sensors
#define DHT11_PIN1 7
#define DHT11_PIN2 8

#define Web_API_KEY "AIzaSyCTK2oXoZ-PtOtovJq9ZfSFOIQgID6t1AE"
#define DATABASE_URL "https://esp-firebase-talkativeehr-default-rtdb.firebaseio.com/"
#define USER_EMAIL "ferlaiamarino@gmail.com"
#define USER_PASS "carlinhosMaia@321"


// User function
void processData(AsyncResult &aResult);

// Authentication
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);

// Firebase components
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

// Timer variables for sending data every 10 seconds
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // 10 seconds in milliseconds

// Variables to send to the database
int intValue = 0;
float floatValue = 0.01;
String stringValue = "";

int intValue2 = 0;
float floatValue2 = 0.01;
String stringValue2 = "";

void setup(){
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  
  // Configure SSL client
  ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);
  
  // Initialize Firebase
  initializeApp(aClient, app, getAuth(user_auth), processData, " authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
}

void loop(){
  // Maintain authentication and async tasks
  app.loop();
  // Check if authentication is ready
  if (app.ready()){ 
    // Periodic data sending every 10 seconds
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= sendInterval){
      // Update the last send time
      lastSendTime = currentTime;
      
      // send a string
      stringValue = "value_" + String(currentTime);
      Database.set<String>(aClient, "/pacientOne/string", stringValue, processData, "RTDB_Send_String");
      // send an int
      Database.set<int>(aClient, "/pacientOne/int", intValue, processData, "RTDB_Send_Int");
      intValue++; //increment intValue in every loop
      // send a string
      floatValue = 0.01 + random (0,100);
      Database.set<float>(aClient, "/pacientOne/float", floatValue, processData, "RTDB_Send_Float");

      stringValue2 = "value_" + String(currentTime);
      Database.set<String>(aClient, "/pacientTwo/string", stringValue2, processData, "RTDB_Send_String");
      // send an int
      Database.set<int>(aClient, "/pacientTwo/int", intValue2, processData, "RTDB_Send_Int");
      intValue2++; //increment intValue in every loop
      // send a string
      floatValue2 = 0.01 + random (0,100);
      Database.set<float>(aClient, "/pacientTwo/float", floatValue2, processData, "RTDB_Send_Float");
    }
  }
}

void processData(AsyncResult &aResult) {
  if (!aResult.isResult())
    return;

  if (aResult.isEvent())
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());

  if (aResult.isDebug())
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());

  if (aResult.isError())
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());

  if (aResult.available())
    Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
}
 /*
float readTemperature(int room) {
  DHT *dht_instance; // Ponteiro para o objeto DHT (dht1, dht2, ou dht3)

  // 1. Mapeamento da sala para a instância do objeto DHT
  if (room == 1) {
    dht_instance = &dht1;
  } else if (room == 2) {
    dht_instance = &dht2;
  }  else {
    // Caso de ERRO
    Serial.print("Erro: Sala DHT invalida (");
    Serial.print(room);
    Serial.println(")");
    return -999.0;
  }

  // 2. Leitura e retorno da temperatura (em Celsius)
  float t = dht_instance->readTemperature();

  // 3. Verifica se a leitura foi bem-sucedida
  if (isnan(t)) {
    Serial.print("Falha na leitura do DHT da Sala ");
    Serial.println(room);
    return -999.0;
  }
  
  return t;
}

float readLuminosity(int room) {
  int ldrPin = -1; // Inicializa o pino para -1 (erro)

  // 1. Mapeamento da sala para o pino Analógico do LDR
  if (room == 1) {
    ldrPin = PINO_LDR_SALA_1;
  } else if (room == 2) {
    ldrPin = PINO_LDR_SALA_2;
  } else if (room == 3) {
    ldrPin = PINO_LDR_SALA_3;
  } else {
    // Caso de ERRO
    Serial.print("Erro: Sala LDR invalida (");
    Serial.print(room);
    Serial.println(")");
    return -999.0;
  }

  // 2. Leitura e retorno do valor analógico
  int analogValue = analogRead(ldrPin);
  
  // Para fins de retorno, mantemos o tipo float como você solicitou na estrutura original
  return (float)analogValue; 
}
float readHumidity(){

}
*/