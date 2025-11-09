## ✅ Recommended Stable Setup for ESP32

| Setting            | Value                        |
|--------------------|-----------------------------|
| **Upload Speed**   | `115200`                    |
| **Flash Mode**     | `DIO`                       |
| **Flash Frequency**| `80 MHz`                    |
| **Partition Scheme** | Default 4MB with SPIFFS   |
| **PSRAM**          | Disabled <br>*(unless your board has it)* |

```cpp
// Example of a single JSON set
FirebaseJson json;
json.set("string", stringValue);
json.set("int", intValue);
json.set("float", floatValue);
Database.set(aClient, "/pacientOne", json, processData, "RTDB_Send_ALL");
```