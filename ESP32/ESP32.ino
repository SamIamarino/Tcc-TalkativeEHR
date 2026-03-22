// ============================================================
// esp32_fauxmo.ino
// 1 ESP32 → 2 quartos → cada quarto tem 1x DHT11 + 1x LDR
// Alexa chama "quarto um ligar" ou "quarto dois ligar"
// e só aquele quarto lê e envia seus sensores
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
#include "fauxmoESP.h"
#include "DHT.h"

// ------------------------------------------------------------
// CONFIGURAÇÕES
// ------------------------------------------------------------
const char* SSID     = "Cristiane_2.4G";
const char* PASSWORD = "31071974";
const char* API_BASE = "http://192.168.1.100:3000"; // IP do PC com server.js

// --- Pinos Quarto 1 ---
#define PINO_DHT_Q1   4    // GPIO digital DHT11 quarto 1
#define PINO_LDR_Q1   34   // GPIO analógico LDR quarto 1

// --- Pinos Quarto 2 ---
#define PINO_DHT_Q2   5    // GPIO digital DHT11 quarto 2
#define PINO_LDR_Q2   35   // GPIO analógico LDR quarto 2

// LDR conversão
const int   ADC_MAX      = 4095;
const float VOLT_REF     = 3.3;

// Intervalo de leitura periódica enquanto o quarto está ativo (ms)
const unsigned long INTERVALO = 5000;

// ------------------------------------------------------------
// Instâncias dos sensores
// ------------------------------------------------------------
DHT dhtQ1(PINO_DHT_Q1, DHT11);
DHT dhtQ2(PINO_DHT_Q2, DHT11);

// ------------------------------------------------------------
// Estado de cada quarto
// ------------------------------------------------------------
struct Quarto {
  int          id;
  const char*  nome;       // nome falado para a Alexa
  bool         ativo;
  unsigned long ultimaLeitura;
};

Quarto quartos[2] = {
  { 1, "quarto um",  false, 0 },
  { 2, "quarto dois", false, 0 },
};

fauxmoESP fauxmo;

// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------
void setup() {

  Serial.begin(115200);
  conectarWiFi();
  
  dhtQ1.begin();
  dhtQ2.begin();

  fauxmo.createServer(true);
  fauxmo.setPort(80);
  fauxmo.enable(true);

  // Registra os 2 quartos como dispositivos separados na Alexa
  for (auto& q : quartos) {
    fauxmo.addDevice(q.nome);
    Serial.printf("[Fauxmo] Dispositivo registrado: '%s'\n", q.nome);
  }

  // Callback único para os dois dispositivos
  fauxmo.onSetState([](unsigned char device_id, const char* device_name, bool state, unsigned char value) {
    Serial.printf("\n[Fauxmo] Alexa chamou '%s' → %s\n", device_name, state ? "LIGAR" : "DESLIGAR");

    // Encontra qual quarto foi chamado pelo nome
    for (auto& q : quartos) {
      if (strcmp(device_name, q.nome) == 0) {
        q.ativo = state;

        if (state) {
          lerESalvar(q);  // leitura imediata ao ligar
        } else {
          enviarLeitura(q.id, NAN, NAN, -1, "desligado");
        }
        break;
      }
    }
  });

  Serial.println("\n[ESP32] Pronto!");
  Serial.println("Fale: 'Alexa, quarto um ligar' ou 'Alexa, quarto dois ligar'");
}

// ------------------------------------------------------------
// Loop
// ------------------------------------------------------------
void loop() {
  fauxmo.handle(); // SEMPRE precisa estar aqui

  // Verifica leitura periódica para cada quarto ativo
  for (auto& q : quartos) {
    if (q.ativo && millis() - q.ultimaLeitura >= INTERVALO) {
      q.ultimaLeitura = millis();
      lerESalvar(q);
    }
  }
}

// ------------------------------------------------------------
// Lê os sensores do quarto e envia para a API
// ------------------------------------------------------------
void lerESalvar(Quarto& q) {
  float temperatura, umidade;
  int   ldrRaw;

  // Seleciona os sensores certos para o quarto
  if (q.id == 1) {
    temperatura = dhtQ1.readTemperature();
    umidade     = dhtQ1.readHumidity();
    ldrRaw      = analogRead(PINO_LDR_Q1);
  } else {
    temperatura = dhtQ2.readTemperature();
    umidade     = dhtQ2.readHumidity();
    ldrRaw      = analogRead(PINO_LDR_Q2);
  }

  // Retry se DHT falhar
  if (isnan(temperatura) || isnan(umidade)) {
    Serial.printf("[DHT11 Q%d] Falha! Tentando novamente...\n", q.id);
    delay(2000);
    if (q.id == 1) {
      temperatura = dhtQ1.readTemperature();
      umidade     = dhtQ1.readHumidity();
    } else {
      temperatura = dhtQ2.readTemperature();
      umidade     = dhtQ2.readHumidity();
    }
  }

  // LDR → luminosidade %
  // Inverta para (100, 0) se o seu circuito tiver o LDR invertido
  int luminosidade = map(ldrRaw, 0, ADC_MAX, 0, 100);

  Serial.printf("[Q%d] Temp: %.1f°C | Umidade: %.1f%% | LDR raw: %d | Lum: %d%%\n",
                q.id, temperatura, umidade, ldrRaw, luminosidade);

  enviarLeitura(q.id, temperatura, umidade, luminosidade, "ligado");
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
    delay(100);
  }

  Serial.println();
  Serial.printf("[WiFi] Conectado! SSID: %s | IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}