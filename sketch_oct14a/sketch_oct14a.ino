/*********
  TalkativeEHR - ESP32 Firebase + Alexa + DHT11 + LDR
  Funcionalidade: Habilita o envio de dados contínuo (a cada 10s) 
  para o Quarto 1 OU Quarto 2 via comando de voz da Alexa.
  
  ATUALIZAÇÃO: Agora suporta 2x DHT11 e 2x LDR, um conjunto para cada quarto.
*********/
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <DHT.h>
#include "fauxmoESP.h"

// ---------- Configuração ---------- //
#define WIFI_SSID "Cristiane_2.4G"
#define WIFI_PASSWORD "31071974"

#define Web_API_KEY "AIzaSyCTK2oXoZ-PtOtovJq9ZfSFOIQgID6t1AE"
#define DATABASE_URL "https://esp-firebase-talkativeehr-default-rtdb.firebaseio.com/"
#define USER_EMAIL "ferlaiamarino@gmail.com"
#define USER_PASS "carlinhosMaia@321"

// ---------- Sensores (2 Conjuntos) ---------- //
#define DHTTYPE DHT11

// Quarto 1 (Sensores independentes)
#define DHTPIN_R1 4      // DHT11 Quarto 1 (Umidade/Temperatura) conectado ao GPIO 4
#define LDRPIN_R1 34     // LDR Quarto 1 (Luminosidade) conectado ao GPIO analógico 34

// Quarto 2 (Sensores independentes - Novos Pinos)
#define DHTPIN_R2 16     // DHT11 Quarto 2 (Umidade/Temperatura) conectado ao GPIO 16
#define LDRPIN_R2 35     // LDR Quarto 2 (Luminosidade) conectado ao GPIO analógico 35

// ---------- Objetos ---------- //
DHT dht1(DHTPIN_R1, DHTTYPE); // Objeto para o DHT11 do Quarto 1
DHT dht2(DHTPIN_R2, DHTTYPE); // Objeto para o DHT11 do Quarto 2
FauxmoESP fauxmo;
WiFiClientSecure ssl_client;
FirebaseApp app;
RealtimeDatabase Database;
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);

// ---------- Variáveis de Estado ---------- //
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // Intervalo de 10 segundos
bool room1SendEnabled = false; // Controle de envio para o Quarto 1
bool room2SendEnabled = false; // Controle de envio para o Quarto 2

// ---------- Protótipos de Função ---------- //
void processData(AsyncResult &aResult);
void sendRoomData(int roomId); // Função para ler e enviar dados para um quarto específico

// ---------- Setup ---------- //
void setup() {
  Serial.begin(115200);
  Serial.println("\n[SETUP] Iniciando...");

  // Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\n✅ Wi-Fi Conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // SSL para Firebase
  ssl_client.setInsecure();

  // Inicializa Firebase
  initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);

  // Inicializa DHTs
  dht1.begin();
  dht2.begin();

  // Configuração do Fauxmo (Alexa)
  fauxmo.createServer(true);      // Cria servidor web interno
  fauxmo.setPort(80);             // Porta HTTP
  fauxmo.enable(true);

  // Registra dispositivos Alexa para controle dos quartos
  fauxmo.addDevice("Quarto Um");
  fauxmo.addDevice("Quarto Dois");

  fauxmo.onSetState([](unsigned char device_id, const char *device_name, bool state, unsigned char value) {
    Serial.printf("[Alexa] Dispositivo %s estado: %s\n", device_name, state ? "LIGADO" : "DESLIGADO");
    
    // Lógica para Quarto Um
    if (strcmp(device_name, "Quarto Um") == 0) {
      room1SendEnabled = state;
      if (state) {
        room2SendEnabled = false; // Desabilita o outro quarto para evitar envio duplicado
        Serial.println("  -> Envio de dados para Quarto 1 HABILITADO.");
      } else {
        Serial.println("  -> Envio de dados para Quarto 1 DESABILITADO.");
      }
    } 
    // Lógica para Quarto Dois
    else if (strcmp(device_name, "Quarto Dois") == 0) {
      room2SendEnabled = state;
      if (state) {
        room1SendEnabled = false; // Desabilita o outro quarto
        Serial.println("  -> Envio de dados para Quarto 2 HABILITADO.");
      } else {
        Serial.println("  -> Envio de dados para Quarto 2 DESABILITADO.");
      }
    }
  });
}

// ---------- Loop ---------- //
void loop() {
  fauxmo.handle();
  app.loop();

  // Verifica se o Firebase está pronto e se algum quarto está habilitado para envio
  if (app.ready() && (room1SendEnabled || room2SendEnabled)) {
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= sendInterval) {
      lastSendTime = currentTime;
      
      if (room1SendEnabled) {
        Serial.println("[TIMER] Enviando dados para o Quarto 1...");
        sendRoomData(1);
      } else if (room2SendEnabled) {
        Serial.println("[TIMER] Enviando dados para o Quarto 2...");
        sendRoomData(2);
      }
    }
  }
}

// ---------- Função: Ler e Enviar Dados para um Quarto Específico ---------- //
void sendRoomData(int roomId) {
  float temperature, humidity;
  int lightLevel;
  String basePath;
  bool readSuccess = false;

  if (roomId == 1) {
    // Leitura dos sensores do Quarto 1 (DHT1 e LDR1)
    temperature = dht1.readTemperature();
    humidity = dht1.readHumidity();
    lightLevel = analogRead(LDRPIN_R1);
    basePath = "/rooms/1/sensors";
    readSuccess = !(isnan(temperature) || isnan(humidity));
    
  } else if (roomId == 2) {
    // Leitura dos sensores do Quarto 2 (DHT2 e LDR2)
    temperature = dht2.readTemperature();
    humidity = dht2.readHumidity();
    lightLevel = analogRead(LDRPIN_R2);
    basePath = "/rooms/2/sensors";
    readSuccess = !(isnan(temperature) || isnan(humidity));
    
  } else {
    Serial.println("Erro: ID de Quarto inválido.");
    return;
  }

  if (!readSuccess) {
    Serial.printf(" Falha ao ler dados do DHT11 do Quarto %d!\n", roomId);
    return;
  }

  Serial.printf("[ROOM %d DATA] Temp: %.2f°C | Umidade: %.2f%% | Luz: %d\n", roomId, temperature, humidity, lightLevel);

  // Envia para o Firebase
  Database.set<float>(aClient, basePath + "/temperatura", temperature, processData, "sendTempR" + String(roomId));
  Database.set<float>(aClient, basePath + "/umidade", humidity, processData, "sendHumR" + String(roomId));
  Database.set<int>(aClient, basePath + "/luminosidade", lightLevel, processData, "sendLightR" + String(roomId));
}

// ---------- Tratamento de Resultado do Firebase ---------- //
void processData(AsyncResult &aResult) {
  if (!aResult.isResult())
    return;

  if (aResult.isError())
    Firebase.printf("🔥 Erro: %s | Código: %d\n", aResult.error().message().c_str(), aResult.error().code());
  else if (aResult.available())
    Firebase.printf("✅ Enviado com sucesso: %s\n", aResult.c_str());
}
