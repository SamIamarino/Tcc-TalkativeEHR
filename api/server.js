// ============================================================
// server.js
// API Express para receber dados do ESP32 e salvar no MySQL
// + Hospedagem do frontend estático na pasta /public
//
// Endpoints:
//   POST /sensor              → registra leitura do sensor no banco
//   GET  /sensor/:quarto_id   → retorna leituras do quarto
//   GET  /paciente/:quarto_id → retorna dados do paciente no quarto
//   GET  /status              → retorna o quarto ativo no momento
//
// Frontend acessível em: http://192.168.1.100:3000/
// ============================================================
// Dependências:
//   npm install express mysql2
// ============================================================

const express = require("express");
const path    = require("path");
const mysql   = require("mysql2/promise");

const app = express();
app.use(express.json());

// ------------------------------------------------------------
// Serve o frontend estático da pasta /public
// ------------------------------------------------------------
app.use(express.static(path.join(__dirname, "public")));

// ------------------------------------------------------------
// CORS — permite o frontend acessar a API
// ------------------------------------------------------------
app.use((req, res, next) => {
  res.header("Access-Control-Allow-Origin", "*");
  res.header("Access-Control-Allow-Headers", "Content-Type, x-api-key");
  next();
});

// ------------------------------------------------------------
// CONFIGURAÇÕES
// ------------------------------------------------------------
const PORT    = 3000;
const API_KEY = "tPmAT5Ab3j7F9";

const DB_CONFIG = {
  host     : "localhost",
  user     : "esp32",
  password : "esp32mysql",
  database : "hospital_iot",
};

// ------------------------------------------------------------
// Quarto ativo (atualizado pelo ESP32 via POST /sensor)
// ------------------------------------------------------------
let quartoAtivo = null;

// ------------------------------------------------------------
// Conexão com o banco
// ------------------------------------------------------------
let db;
async function conectarBanco() {
  try {
    db = await mysql.createPool(DB_CONFIG);
    console.log("[DB] Conectado ao MySQL com sucesso!");
  } catch (err) {
    console.error("[DB] Falha na conexão:", err.message);
    process.exit(1);
  }
}

// ------------------------------------------------------------
// Middleware: valida API Key
// ------------------------------------------------------------
function validarApiKey(req, res, next) {
  const chave = req.headers["x-api-key"] || req.body?.api_key;
  if (chave !== API_KEY) {
    return res.status(401).json({ erro: "API Key inválida ou ausente." });
  }
  next();
}

// ------------------------------------------------------------
// POST /sensor
// Recebe dados do ESP32 e salva em LeituraSensor
// ------------------------------------------------------------
app.post("/sensor", validarApiKey, async (req, res) => {
  const { quarto_id, temperatura, umidade, luminosidade, status } = req.body;

  if (!quarto_id) {
    return res.status(400).json({ erro: "Campo 'quarto_id' é obrigatório." });
  }

  // Atualiza o quarto ativo baseado no status enviado pelo ESP32
  quartoAtivo = status === "desligado" ? null : quarto_id;
  console.log(`[Status] Quarto ativo: ${quartoAtivo ?? "nenhum"}`);

  try {
    const [pacientes] = await db.query(
      `SELECT p.id AS paciente_id
       FROM Paciente p
       JOIN Leito l ON p.leito_id = l.id
       WHERE l.id = ? AND p.alta_em IS NULL
       LIMIT 1`,
      [quarto_id]
    );

    if (pacientes.length === 0) {
      return res.status(404).json({
        erro: `Nenhum paciente internado encontrado no quarto ${quarto_id}.`,
      });
    }

    const paciente_id = pacientes[0].paciente_id;

    await db.query(
      `INSERT INTO LeituraSensor (paciente_id, temperatura, umidade, luminosidade)
       VALUES (?, ?, ?, ?)`,
      [paciente_id, temperatura ?? null, umidade ?? null, luminosidade ?? null]
    );

    console.log(
      `[POST /sensor] Quarto ${quarto_id} | Paciente ID ${paciente_id} | ` +
      `Temp: ${temperatura}°C | Umidade: ${umidade}% | Lum: ${luminosidade}% | Status: ${status}`
    );

    return res.status(201).json({
      mensagem: "Leitura registrada com sucesso!",
      paciente_id, quarto_id, temperatura, umidade, luminosidade, status,
    });

  } catch (err) {
    console.error("[POST /sensor] Erro:", err.message);
    return res.status(500).json({ erro: "Erro interno ao salvar leitura." });
  }
});

// ------------------------------------------------------------
// GET /sensor/:quarto_id
// Retorna as últimas 20 leituras do quarto informado
// ------------------------------------------------------------
app.get("/sensor/:quarto_id", validarApiKey, async (req, res) => {
  const quarto_id = parseInt(req.params.quarto_id);

  if (isNaN(quarto_id)) {
    return res.status(400).json({ erro: "quarto_id inválido." });
  }

  try {
    const [leituras] = await db.query(
      `SELECT
         ls.id,
         ls.temperatura,
         ls.umidade,
         ls.luminosidade,
         ls.registrado_em
       FROM LeituraSensor ls
       JOIN Paciente p ON ls.paciente_id = p.id
       JOIN Leito    l ON p.leito_id     = l.id
       WHERE l.id = ?
       ORDER BY ls.registrado_em DESC
       LIMIT 20`,
      [quarto_id]
    );

    if (leituras.length === 0) {
      return res.status(404).json({
        mensagem: `Nenhuma leitura encontrada para o quarto ${quarto_id}.`,
      });
    }

    return res.status(200).json({ quarto_id, total: leituras.length, leituras });

  } catch (err) {
    console.error(`[GET /sensor/${quarto_id}] Erro:`, err.message);
    return res.status(500).json({ erro: "Erro interno ao buscar leituras." });
  }
});

// ------------------------------------------------------------
// GET /paciente/:quarto_id
// Retorna os dados do paciente internado no quarto informado
// ------------------------------------------------------------
app.get("/paciente/:quarto_id", validarApiKey, async (req, res) => {
  const quarto_id = parseInt(req.params.quarto_id);

  if (isNaN(quarto_id)) {
    return res.status(400).json({ erro: "quarto_id inválido." });
  }

  try {
    const [rows] = await db.query(
      `SELECT
         p.id,
         p.nome_completo,
         p.data_nascimento,
         p.sexo,
         p.tipo_sanguineo,
         p.alergias,
         p.telefone,
         p.contato_emergencia,
         p.telefone_emergencia,
         p.internado_em,
         l.numero_leito,
         l.andar,
         l.ala,
         m.nome_completo   AS medico_nome,
         m.especialidade   AS medico_especialidade,
         m.telefone        AS medico_telefone
       FROM Paciente p
       JOIN Leito  l ON p.leito_id  = l.id
       LEFT JOIN Medico m ON p.medico_id = m.id
       WHERE l.id = ? AND p.alta_em IS NULL
       LIMIT 1`,
      [quarto_id]
    );

    if (rows.length === 0) {
      return res.status(404).json({
        mensagem: `Nenhum paciente internado no quarto ${quarto_id}.`,
      });
    }

    const p = rows[0];

    return res.status(200).json({
      quarto_id,
      paciente: {
        id                  : p.id,
        nome                : p.nome_completo,
        data_nascimento     : p.data_nascimento,
        sexo                : p.sexo,
        tipo_sanguineo      : p.tipo_sanguineo,
        alergias            : p.alergias,
        telefone            : p.telefone,
        contato_emergencia  : p.contato_emergencia,
        telefone_emergencia : p.telefone_emergencia,
        internado_em        : p.internado_em,
      },
      leito: {
        numero : p.numero_leito,
        andar  : p.andar,
        ala    : p.ala,
      },
      medico: {
        nome          : p.medico_nome,
        especialidade : p.medico_especialidade,
        telefone      : p.medico_telefone,
      },
    });

  } catch (err) {
    console.error(`[GET /paciente/${quarto_id}] Erro:`, err.message);
    return res.status(500).json({ erro: "Erro interno ao buscar paciente." });
  }
});

// ------------------------------------------------------------
// GET /status
// Retorna qual quarto está ativo no momento (ativado pela Alexa)
// ------------------------------------------------------------
app.get("/status", (req, res) => {
  return res.status(200).json({ quarto_id: quartoAtivo });
});

// ------------------------------------------------------------
// Qualquer rota não encontrada serve o index.html do frontend
// ------------------------------------------------------------
app.get("*", (req, res) => {
  res.sendFile(path.join(__dirname, "public", "index.html"));
});

// ------------------------------------------------------------
// Inicia o servidor
// ------------------------------------------------------------
conectarBanco().then(() => {
  app.listen(PORT, () => {
    console.log(`[API] Servidor rodando em http://localhost:${PORT}`);
    console.log(`[API] Frontend disponível em http://localhost:${PORT}/`);
    console.log(`[API] Endpoints disponíveis:`);
    console.log(`        POST http://localhost:${PORT}/sensor`);
    console.log(`        GET  http://localhost:${PORT}/sensor/:quarto_id`);
    console.log(`        GET  http://localhost:${PORT}/paciente/:quarto_id`);
    console.log(`        GET  http://localhost:${PORT}/status`);
  });
});
