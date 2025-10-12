/*********
  TalkativeEHR - ESP32 Firebase + Alexa + DHT11 + LDR
*********/
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <DHT.h>
#include <FauxmoESP.h>

// ---------- Configuration ---------- //
#define WIFI_SSID "Cristiane_2.4G"
#define WIFI_PASSWORD "31071974"

#define Web_API_KEY "AIzaSyCTK2oXoZ-PtOtovJq9ZfSFOIQgID6t1AE"
#define DATABASE_URL "https://esp-firebase-talkativeehr-default-rtdb.firebaseio.com/"
#define USER_EMAIL "ferlaiamarino@gmail.com"
#define USER_PASS "carlinhosMaia@321"

// ---------- Sensors ---------- //
#define DHTPIN 4         // DHT11 connected to GPIO 4
#define DHTTYPE DHT11
#define LDRPIN 34        // LDR connected to analog GPIO 34

// ---------- Objects ---------- //
DHT dht(DHTPIN, DHTTYPE);
FauxmoESP fauxmo;
WiFiClientSecure ssl_client;
FirebaseApp app;
RealtimeDatabase Database;
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);

// ---------- Timer ---------- //
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // 10 seconds
bool sendDataEnabled = false;

// ---------- Function Prototypes ---------- //
void processData(AsyncResult &aResult);
void sendSensorData();

// ---------- Setup ---------- //
void setup() {
  Serial.begin(115200);
  Serial.println("\n[SETUP] Starting...");

  // Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\n✅ Wi-Fi Connected!");
  Serial.println(WiFi.localIP());

  // SSL for Firebase
  ssl_client.setInsecure();

  // Initialize Firebase
  initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);

  // Initialize DHT
  dht.begin();

  // Setup Fauxmo (Alexa)
  fauxmo.createServer(true);      // Create internal webserver
  fauxmo.setPort(80);             // HTTP port
  fauxmo.enable(true);

  // Register Alexa device
  fauxmo.addDevice("hospital sensors");

  fauxmo.onSetState([](unsigned char device_id, const char *device_name, bool state, unsigned char value) {
    Serial.printf("[Alexa] Device %s state: %s\n", device_name, state ? "ON" : "OFF");
    sendDataEnabled = state; // Alexa turns data sending ON/OFF
  });
}

// ---------- Loop ---------- //
void loop() {
  fauxmo.handle();
  app.loop();

  if (app.ready() && sendDataEnabled) {
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= sendInterval) {
      lastSendTime = currentTime;
      sendSensorData();
    }
  }
}

// ---------- Send Sensor Data ---------- //
void sendSensorData() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int lightLevel = analogRead(LDRPIN);

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("⚠️ Failed to read DHT11 data!");
    return;
  }

  Serial.printf("[DATA] Temp: %.2f°C | Humidity: %.2f%% | Light: %d\n", temperature, humidity, lightLevel);

  // Send to Firebase
  Database.set<float>(aClient, "/sensors/temperature", temperature, processData, "sendTemp");
  Database.set<float>(aClient, "/sensors/humidity", humidity, processData, "sendHum");
  Database.set<int>(aClient, "/sensors/light", lightLevel, processData, "sendLight");
}

// ---------- Firebase Result Handling ---------- //
void processData(AsyncResult &aResult) {
  if (!aResult.isResult())
    return;

  if (aResult.isError())
    Firebase.printf("🔥 Error: %s | Code: %d\n", aResult.error().message().c_str(), aResult.error().code());
  else if (aResult.available())
    Firebase.printf("✅ Sent: %s\n", aResult.c_str());
}
