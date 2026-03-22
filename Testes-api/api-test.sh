#!/bin/bash
# ============================================================
# teste_api.sh
# Testa todos os endpoints da API Hospital IoT
# Uso: bash teste_api.sh
# ============================================================

API="http://localhost:3000"
VERDE="\e[32m"
VERMELHO="\e[31m"
AMARELO="\e[33m"
RESET="\e[0m"

separador() {
  echo ""
  echo "────────────────────────────────────────"
  echo " $1"
  echo "────────────────────────────────────────"
}

# ------------------------------------------------------------
# 1. POST /sensor — simula ESP32 ligando quarto 1
# ------------------------------------------------------------
separador "1. POST /sensor — quarto 1 LIGADO"
curl -s -X POST "$API/sensor" \
  -H "Content-Type: application/json" \
  -d '{"quarto_id": 1, "temperatura": 24.5, "umidade": 58.0, "luminosidade": 47, "status": "ligado"}' \
  | json_pp
echo ""

# ------------------------------------------------------------
# 2. GET /status — deve retornar quarto_id: 1
# ------------------------------------------------------------
separador "2. GET /status — deve mostrar quarto 1 ativo"
curl -s "$API/status" | json_pp
echo ""

# ------------------------------------------------------------
# 3. GET /sensor/:quarto_id — leituras do quarto 1
# ------------------------------------------------------------
separador "3. GET /sensor/1 — leituras do quarto 1"
curl -s "$API/sensor/1" | json_pp
echo ""

# ------------------------------------------------------------
# 4. GET /paciente/:quarto_id — dados do paciente no quarto 1
# ------------------------------------------------------------
separador "4. GET /paciente/1 — paciente do quarto 1"
curl -s "$API/paciente/1" | json_pp
echo ""

# ------------------------------------------------------------
# 5. GET /evolucao/:paciente_id — evoluções do paciente 1
# ------------------------------------------------------------
separador "5. GET /evolucao/1 — evoluções do paciente 1"
curl -s "$API/evolucao/1" | json_pp
echo ""

# ------------------------------------------------------------
# 6. POST /sensor — simula ESP32 ligando quarto 2
# ------------------------------------------------------------
separador "6. POST /sensor — quarto 2 LIGADO"
curl -s -X POST "$API/sensor" \
  -H "Content-Type: application/json" \
  -d '{"quarto_id": 2, "temperatura": 23.1, "umidade": 61.0, "luminosidade": 32, "status": "ligado"}' \
  | json_pp
echo ""

# ------------------------------------------------------------
# 7. GET /status — deve retornar quarto_id: 2
# ------------------------------------------------------------
separador "7. GET /status — deve mostrar quarto 2 ativo"
curl -s "$API/status" | json_pp
echo ""

# ------------------------------------------------------------
# 8. GET /sensor/2 — leituras do quarto 2
# ------------------------------------------------------------
separador "8. GET /sensor/2 — leituras do quarto 2"
curl -s "$API/sensor/2" | json_pp
echo ""

# ------------------------------------------------------------
# 9. GET /paciente/2 — dados do paciente no quarto 2
# ------------------------------------------------------------
separador "9. GET /paciente/2 — paciente do quarto 2"
curl -s "$API/paciente/2" | json_pp
echo ""

# ------------------------------------------------------------
# 10. GET /evolucao/2 — evoluções do paciente 2
# ------------------------------------------------------------
separador "10. GET /evolucao/2 — evoluções do paciente 2"
curl -s "$API/evolucao/2" | json_pp
echo ""

# ------------------------------------------------------------
# 11. POST /sensor — simula Alexa desligando quarto 2
# ------------------------------------------------------------
separador "11. POST /sensor — quarto 2 DESLIGADO"
curl -s -X POST "$API/sensor" \
  -H "Content-Type: application/json" \
  -d '{"quarto_id": 2, "status": "desligado"}' \
  | json_pp
echo ""

# ------------------------------------------------------------
# 12. GET /status — deve retornar quarto_id: null
# ------------------------------------------------------------
separador "12. GET /status — deve mostrar null (nenhum quarto ativo)"
curl -s "$API/status" | json_pp
echo ""

echo ""
echo -e "${VERDE}✓ Todos os testes concluídos!${RESET}"
echo ""