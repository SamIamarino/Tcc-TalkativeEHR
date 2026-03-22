# Talkative EHR — Hospital IoT

Sistema de monitoramento de quartos hospitalares via ESP32 + Alexa com frontend de prontuário eletrônico (EHR).

---

## Diagrama do Sistema

<!-- Cole aqui o SVG do fluxo gerado -->

---

## Visão Geral

O sistema permite que um médico ou enfermeiro fale com a Alexa para ativar o monitoramento de um quarto. O ESP32 instalado no quarto lê os sensores de temperatura, umidade e luminosidade e envia os dados para uma API Express que salva no MySQL. O frontend atualiza automaticamente exibindo os dados do paciente internado e as leituras dos sensores em tempo real.

---

## Fluxo de Funcionamento

1. Médico fala **"Alexa, quarto um ligar"**
2. Alexa aciona o ESP32 via protocolo UPnP (fauxmoESP)
3. ESP32 lê os sensores DHT11 (temperatura e umidade) e LDR (luminosidade)
4. ESP32 faz `POST /sensor` com os dados para a API Express
5. API salva a leitura no MySQL vinculada ao paciente internado no quarto
6. Frontend consulta `GET /status` a cada 3s e detecta o quarto ativo
7. Frontend busca os dados do paciente (`GET /paciente/:quarto_id`) e as evoluções (`GET /evolucao/:paciente_id`)
8. Frontend inicia polling de sensores (`GET /sensor/:quarto_id`) a cada 5s e atualiza a tela

---

## Tecnologias

| Camada      | Tecnologia                          |
|-------------|-------------------------------------|
| Hardware    | ESP32, DHT11, LDR                   |
| Voz         | Amazon Alexa + fauxmoESP (sivar2311)|
| Backend     | Node.js + Express                   |
| Banco       | MySQL                               |
| Frontend    | HTML + CSS + JavaScript (vanilla)   |

---

## Estrutura do Projeto

```
hospital-iot/
├── server.js           # API Express + servidor do frontend
├── package.json
└── public/
    ├── index.html      # Frontend EHR
    ├── sensor.js       # Script de polling e atualização da tela
    └── styles.css      # Estilos
```

---

## Banco de Dados

```
hospital_iot
├── Medico
├── Leito
├── Paciente
├── EvolucaoPaciente
└── LeituraSensor
```

Os dados do paciente, médico e evoluções são inseridos manualmente (mockados). Apenas a tabela `LeituraSensor` é preenchida automaticamente pelo ESP32.

---

## Endpoints da API

| Método | Endpoint                    | Descrição                              |
|--------|-----------------------------|----------------------------------------|
| POST   | `/sensor`                   | Recebe leitura do ESP32 e salva no banco |
| GET    | `/sensor/:quarto_id`        | Retorna as últimas 20 leituras do quarto |
| GET    | `/paciente/:quarto_id`      | Retorna dados do paciente internado    |
| GET    | `/evolucao/:paciente_id`    | Retorna evoluções clínicas do paciente |
| GET    | `/status`                   | Retorna o quarto ativo no momento      |

---

## Hardware — Pinos ESP32

| Sensor  | Quarto | GPIO  |
|---------|--------|-------|
| DHT11   | 1      | 4     |
| LDR     | 1      | 34    |
| DHT11   | 2      | 5     |
| LDR     | 2      | 35    |

---

## Como Rodar

### 1. Banco de dados

```bash
mysql -u root -p < seed.sql
```

### 2. API

```bash
npm install express mysql2
node server.js
```

O servidor sobe em `http://localhost:3000` e já serve o frontend.

### 3. ESP32

- Instale as bibliotecas no Arduino IDE:
  - `fauxmoESP` (sivar2311)
  - `DHT sensor library` (Adafruit)
  - `ArduinoJson` (Benoit Blanchon)
- Configure o SSID, senha e IP da API no topo do `esp32_fauxmo.ino`
- Grave o firmware no ESP32

### 4. Alexa

No app da Alexa:

```
Dispositivos → + → Adicionar dispositivo → Outro → Descobrir dispositivos
```

---

## Testando a API

```bash
# Simular ESP32 enviando dados do quarto 1
curl -X POST http://localhost:3000/sensor \
  -H "Content-Type: application/json" \
  -d '{"quarto_id": 1, "temperatura": 24.5, "umidade": 58.0, "luminosidade": 47, "status": "ligado"}'

# Verificar quarto ativo
curl http://localhost:3000/status

# Buscar dados do paciente
curl http://localhost:3000/paciente/1

# Buscar leituras do sensor
curl http://localhost:3000/sensor/1

# Buscar evoluções do paciente
curl http://localhost:3000/evolucao/1
```

---

## Comandos de Voz

| Comando                       | Ação                                      |
|-------------------------------|-------------------------------------------|
| "Alexa, quarto um ligar"      | Ativa monitoramento do quarto 1           |
| "Alexa, quarto um desligar"   | Desativa monitoramento do quarto 1        |
| "Alexa, quarto dois ligar"    | Ativa monitoramento do quarto 2           |
| "Alexa, quarto dois desligar" | Desativa monitoramento do quarto 2        |
