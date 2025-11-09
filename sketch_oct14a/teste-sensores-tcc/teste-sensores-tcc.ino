/*
// Valor mínimo de leitura do sensor
const int sensorMin = 0;

// Valor máximo de leitura do sensor
const int sensorMax = 4095;  // O ADC do ESP32 vai de 0 a 4095 por padrão

// Definindo o pino analógico (somente leitura)
const int photocellPin = 34;

void setup() {
  // Inicializando a porta serial com BaudRate = 115200 (mais comum no ESP32)
  Serial.begin(115200);
  delay(1000);
  Serial.println("Leitura do sensor iniciada...");
}

void loop() {
  int analogValue;
  int range;

  // Leitura do valor analógico no pino 34
  analogValue = analogRead(photocellPin);

  // Mapeamento do valor medido entre as escalas de mínimo e máximo,
  // convertendo para 4 faixas (0 a 3)
  range = map(analogValue, sensorMin, sensorMax, 0, 3);

  // Exibindo o valor lido e a faixa correspondente
  Serial.print("Valor bruto: ");
  Serial.print(analogValue);
  Serial.print(" | Faixa: ");
  Serial.println(range);

  delay(500);  // pequena pausa para facilitar a leitura no monitor serial
}
*/


// Bibliotecas Utilizadas
#include "DHT.h"

// Constantes definidas do GPIO e do tipo do sensor DHT
// *** CORREÇÃO PARA ESP32: Usamos o número do GPIO (Ex: GPIO 4) ***
#define DHTPIN 4
#define DHTTYPE DHT11
// #define DHTTYPE DHT22   // DHT 22 (AM2302), AM2321
// #define DHTTYPE DHT21   // DHT 21 (AM2301)

// Conecte pino 1 do sensor (esquerda) ao +5V (ou 3.3V no ESP32)
// Conecte pino 2 do sensor ao pino de dados definido no ESP32 (GPIO 4 neste caso)
// Conecte pino 4 do sensor ao GND
// Conecte o resistor de 10K entre pin 2 (dados)
// e ao pino 1 (VCC) do sensor
DHT dht(DHTPIN, DHTTYPE);

void setup()
{
  // A velocidade 115200 é padrão para o monitor serial do ESP32
  Serial.begin(115200); 
  delay(1000);
  Serial.println("Inicializando o Sensor DHT no ESP32....");
  dht.begin();
}

void loop()
{
  // A leitura da temperatura e umidade pode levar 250ms!
  // O atraso do sensor pode chegar a 2 segundos.
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // testa se retorno é valido, caso contrário algo está errado.
  if (isnan(t) || isnan(h))
  {
    Serial.println("Falha ao ler o sensor DHT! Verifique a fiação.");
  }
  else
  {
    Serial.print("Umidade: ");
    Serial.print(h);
    Serial.print(" % ");
    Serial.print("Temperatura: ");
    Serial.print(t);
    Serial.println(" C");
  }
  
  delay(3000); // Aguarda 3 segundos para a próxima leitura
}