// ============================================================
// esp32_fauxmo.ino
// 1 ESP32 → 2 quartos → cada quarto tem 1x DHT11 + 1x LDR
// Alexa chama "quarto um ligar" ou "quarto dois ligar"
// ============================================================
// Bibliotecas:
//   - fauxmoESP (sivar2311) → https://github.com/sivar2311/fauxmoESP
//   - DHT sensor library (Adafruit)
//   - ArduinoJson (Benoit Blanchon)
//   - WiFi + HTTPClient (built-in ESP32)
// ============================================================

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <fauxmoESP.h>
#include "DHT.h"

// ------------------------------------------------------------
// CONFIGURAÇÕES
// ------------------------------------------------------------
const char* SSID     = "Cristiane_2.4G";
const char* PASSWORD = "31071974";
const char* API_BASE = "http://192.168.0.206:3000"; // IP do PC com server.js

// --- Pinos Quarto 1 ---
#define PINO_DHT_Q1   4
#define PINO_LDR_Q1   34

// --- Pinos Quarto 2 ---
#define PINO_DHT_Q2   5
#define PINO_LDR_Q2   35

const int ADC_MAX = 4095;
const unsigned long INTERVALO = 5000;

// ------------------------------------------------------------
// Instâncias dos sensores
// ------------------------------------------------------------
DHT dhtQ1(PINO_DHT_Q1, DHT11);
DHT dhtQ2(PINO_DHT_Q2, DHT11);

fauxmoESP fauxmo;

// ------------------------------------------------------------
// Estado dos quartos — sem structs, sem arrays
// ------------------------------------------------------------
bool          q1_ativo  = false;
unsigned long q1_ultima = 0;

bool          q2_ativo  = false;
unsigned long q2_ultima = 0;

// ------------------------------------------------------------
// Flags para processar ação fora do callback
// ------------------------------------------------------------
int  quartoAcionadoId    = -1;
bool quartoAcionadoState = false;
bool temAcao             = false;

// ------------------------------------------------------------
// Declarações antecipadas
// ------------------------------------------------------------
void lerESalvarQ1();
void lerESalvarQ2();
void enviarLeitura(int quartoId, float temperatura, float umidade, int luminosidade, String status);
void conectarWiFi();

// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(100);

  conectarWiFi();

  dhtQ1.begin();
  dhtQ2.begin();

  fauxmo.createServer(true);
  fauxmo.setPort(80);
  fauxmo.enable(true);

  fauxmo.addDevice("quarto um");
  Serial.println("[Fauxmo] Dispositivo registrado: 'quarto um'");

  fauxmo.addDevice("quarto dois");
  Serial.println("[Fauxmo] Dispositivo registrado: 'quarto dois'");

  // Callback — apenas sinaliza, não executa nada pesado aqui
  fauxmo.onSetState([](unsigned char device_id, const char* device_name, bool state, unsigned char value) {
    Serial.printf("\n[Fauxmo] Alexa chamou '%s' → %s\n", device_name, state ? "LIGAR" : "DESLIGAR");

    if (strcmp(device_name, "quarto um") == 0) {
      quartoAcionadoId    = 1;
      quartoAcionadoState = state;
      temAcao             = true;
    } else if (strcmp(device_name, "quarto dois") == 0) {
      quartoAcionadoId    = 2;
      quartoAcionadoState = state;
      temAcao             = true;
    }
  });

  Serial.println("\n[ESP32] Pronto!");
  Serial.println("Fale: 'Alexa, quarto um ligar' ou 'Alexa, quarto dois ligar'");
}

// ------------------------------------------------------------
// Loop
// ------------------------------------------------------------
void loop() {
  fauxmo.handle();

  // Processa ação da Alexa fora do callback
  if (temAcao) {
    temAcao = false;

    if (quartoAcionadoId == 1) {
      q1_ativo = quartoAcionadoState;
      if (q1_ativo) lerESalvarQ1();
      else enviarLeitura(1, NAN, NAN, -1, "desligado");

    } else if (quartoAcionadoId == 2) {
      q2_ativo = quartoAcionadoState;
      if (q2_ativo) lerESalvarQ2();
      else enviarLeitura(2, NAN, NAN, -1, "desligado");
    }
  }

  // Leitura periódica quarto 1
  if (q1_ativo && millis() - q1_ultima >= INTERVALO) {
    q1_ultima = millis();
    lerESalvarQ1();
  }

  // Leitura periódica quarto 2
  if (q2_ativo && millis() - q2_ultima >= INTERVALO) {
    q2_ultima = millis();
    lerESalvarQ2();
  }
}

// ------------------------------------------------------------
// Lê sensores do Quarto 1 e envia para a API
// ------------------------------------------------------------
void lerESalvarQ1() {
  float temperatura = dhtQ1.readTemperature();
  float umidade     = dhtQ1.readHumidity();
  int   ldrRaw      = analogRead(PINO_LDR_Q1);

  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println("[DHT11 Q1] Falha! Tentando novamente...");
    delay(2000);
    temperatura = dhtQ1.readTemperature();
    umidade     = dhtQ1.readHumidity();
  }

  int luminosidade = map(ldrRaw, 0, ADC_MAX, 0, 100);

  Serial.printf("[Q1] Temp: %.1f°C | Umidade: %.1f%% | LDR raw: %d | Lum: %d%%\n",
                temperatura, umidade, ldrRaw, luminosidade);

  enviarLeitura(1, temperatura, umidade, luminosidade, "ligado");
}

// ------------------------------------------------------------
// Lê sensores do Quarto 2 e envia para a API
// ------------------------------------------------------------
void lerESalvarQ2() {
  float temperatura = dhtQ2.readTemperature();
  float umidade     = dhtQ2.readHumidity();
  int   ldrRaw      = analogRead(PINO_LDR_Q2);

  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println("[DHT11 Q2] Falha! Tentando novamente...");
    delay(2000);
    temperatura = dhtQ2.readTemperature();
    umidade     = dhtQ2.readHumidity();
  }

  int luminosidade = map(ldrRaw, 0, ADC_MAX, 0, 100);

  Serial.printf("[Q2] Temp: %.1f°C | Umidade: %.1f%% | LDR raw: %d | Lum: %d%%\n",
                temperatura, umidade, ldrRaw, luminosidade);

  enviarLeitura(2, temperatura, umidade, luminosidade, "ligado");
}

// ------------------------------------------------------------
// POST /sensor → API Express
// ------------------------------------------------------------
void enviarLeitura(int quartoId, float temperatura, float umidade, int luminosidade, String status) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Sem conexão, pulando envio.");
    return;
  }

  HTTPClient http;
  String url = String(API_BASE) + "/sensor";

  StaticJsonDocument<256> doc;
  doc["quarto_id"] = quartoId;
  doc["status"]    = status;
  if (!isnan(temperatura))  doc["temperatura"]  = serialized(String(temperatura, 1));
  if (!isnan(umidade))      doc["umidade"]      = serialized(String(umidade, 1));
  if (luminosidade >= 0)    doc["luminosidade"] = luminosidade;

  String jsonBody;
  serializeJson(doc, jsonBody);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(jsonBody);
  Serial.printf("[API] POST /sensor Q%d → HTTP %d\n", quartoId, httpCode);
  http.end();
}

// ------------------------------------------------------------
// Conecta ao WiFi
// ------------------------------------------------------------
void conectarWiFi() {
  WiFi.mode(WIFI_STA);
  Serial.printf("[WiFi] Conectando a %s ", SSID);
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    yield();
  }

  Serial.println();
  Serial.printf("[WiFi] Conectado! SSID: %s | IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}
