const express = require("express");
const path    = require("path");
const mysql   = require("mysql2/promise");

const app = express();
app.use(express.json());
app.use(express.static(path.join(__dirname, "public")));

app.use((req, res, next) => {
  res.header("Access-Control-Allow-Origin", "*");
  res.header("Access-Control-Allow-Headers", "Content-Type, x-api-key");
  res.header("Cache-Control", "no-store, no-cache, must-revalidate");
  res.header("Pragma", "no-cache");
  res.header("Expires", "0");
  next();
});

const PORT = 3000;

const DB_CONFIG = {
  host     : "localhost",
  user     : "root",
  password : "",
  database : "hospital_iot",
};

// ------------------------------------------------------------
// ÚNICO estado que importa: qual foi o ÚLTIMO quarto chamado
// pela Alexa. Só muda quando a Alexa chamar outro quarto.
// Leituras periódicas do ESP32 NÃO alteram isso.
// ------------------------------------------------------------
let ultimoQuarto = null;

let db;
async function conectarBanco() {
  try {
    db = await mysql.createPool(DB_CONFIG);
    console.log("[DB] Conectado ao MySQL!");
  } catch (err) {
    console.error("[DB] Falha:", err.message);
    process.exit(1);
  }
}

// ------------------------------------------------------------
// POST /sensor
// O ESP32 manda "tipo": "ativacao" quando a Alexa ligar
//                "tipo": "leitura"  nas leituras periódicas
//                "tipo": "desligado" quando desligar
// ------------------------------------------------------------
app.post("/sensor", async (req, res) => {
  const { quarto_id, temperatura, umidade, luminosidade, status } = req.body;

  if (!quarto_id) {
    return res.status(400).json({ erro: "Campo 'quarto_id' é obrigatório." });
  }

  // Só atualiza o quarto ativo quando for uma ativação nova pela Alexa
  if (status === "ligado") {
    ultimoQuarto = quarto_id;
    console.log(`[Alexa] Quarto ativo agora: ${ultimoQuarto}`);
  } else if (status === "desligado") {
    if (ultimoQuarto === quarto_id) ultimoQuarto = null;
    console.log(`[Alexa] Quarto ${quarto_id} desligado. Ativo: ${ultimoQuarto ?? "nenhum"}`);
  }
  // leituras periódicas (status = "ligado" mas já estava ativo) não mudam nada no front

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
      return res.status(404).json({ erro: `Nenhum paciente no quarto ${quarto_id}.` });
    }

    const paciente_id = pacientes[0].paciente_id;

    if (temperatura != null || umidade != null || luminosidade != null) {
      await db.query(
        `INSERT INTO LeituraSensor (paciente_id, temperatura, umidade, luminosidade)
         VALUES (?, ?, ?, ?)`,
        [paciente_id, temperatura ?? null, umidade ?? null, luminosidade ?? null]
      );
    }

    console.log(`[POST /sensor] Q${quarto_id} | Temp:${temperatura} Umid:${umidade} Lum:${luminosidade} | ${status}`);

    return res.status(201).json({ mensagem: "OK", paciente_id, quarto_id, status });

  } catch (err) {
    console.error("[POST /sensor] Erro:", err.message);
    return res.status(500).json({ erro: "Erro interno." });
  }
});

// ------------------------------------------------------------
// GET /status — retorna apenas o último quarto chamado pela Alexa
// ------------------------------------------------------------
app.get("/status", (req, res) => {
  return res.status(200).json({ quarto_id: ultimoQuarto });
});

// ------------------------------------------------------------
// GET /sensor/:quarto_id
// ------------------------------------------------------------
app.get("/sensor/:quarto_id", async (req, res) => {
  const quarto_id = parseInt(req.params.quarto_id);
  if (isNaN(quarto_id)) return res.status(400).json({ erro: "quarto_id inválido." });

  try {
    const [leituras] = await db.query(
      `SELECT ls.id, ls.temperatura, ls.umidade, ls.luminosidade, ls.registrado_em
       FROM LeituraSensor ls
       JOIN Paciente p ON ls.paciente_id = p.id
       JOIN Leito    l ON p.leito_id = l.id
       WHERE l.id = ?
       ORDER BY ls.registrado_em DESC
       LIMIT 20`,
      [quarto_id]
    );

    if (leituras.length === 0) {
      return res.status(404).json({ mensagem: `Nenhuma leitura para o quarto ${quarto_id}.` });
    }

    return res.status(200).json({ quarto_id, total: leituras.length, leituras });

  } catch (err) {
    console.error(`[GET /sensor] Erro:`, err.message);
    return res.status(500).json({ erro: "Erro interno." });
  }
});

// ------------------------------------------------------------
// GET /paciente/:quarto_id
// ------------------------------------------------------------
app.get("/paciente/:quarto_id", async (req, res) => {
  const quarto_id = parseInt(req.params.quarto_id);
  if (isNaN(quarto_id)) return res.status(400).json({ erro: "quarto_id inválido." });

  try {
    const [rows] = await db.query(
      `SELECT
         p.id, p.nome_completo, p.data_nascimento, p.sexo,
         p.tipo_sanguineo, p.alergias, p.telefone,
         p.contato_emergencia, p.telefone_emergencia, p.internado_em,
         l.numero_leito, l.andar, l.ala,
         m.nome_completo AS medico_nome,
         m.especialidade AS medico_especialidade,
         m.telefone      AS medico_telefone
       FROM Paciente p
       JOIN Leito  l ON p.leito_id = l.id
       LEFT JOIN Medico m ON p.medico_id = m.id
       WHERE l.id = ? AND p.alta_em IS NULL
       LIMIT 1`,
      [quarto_id]
    );

    if (rows.length === 0) {
      return res.status(404).json({ mensagem: `Nenhum paciente no quarto ${quarto_id}.` });
    }

    const p = rows[0];
    return res.status(200).json({
      quarto_id,
      paciente: {
        id: p.id, nome: p.nome_completo, data_nascimento: p.data_nascimento,
        sexo: p.sexo, tipo_sanguineo: p.tipo_sanguineo, alergias: p.alergias,
        telefone: p.telefone, contato_emergencia: p.contato_emergencia,
        telefone_emergencia: p.telefone_emergencia, internado_em: p.internado_em,
      },
      leito:  { numero: p.numero_leito, andar: p.andar, ala: p.ala },
      medico: { nome: p.medico_nome, especialidade: p.medico_especialidade, telefone: p.medico_telefone },
    });

  } catch (err) {
    console.error(`[GET /paciente] Erro:`, err.message);
    return res.status(500).json({ erro: "Erro interno." });
  }
});

// ------------------------------------------------------------
// GET /evolucao/:paciente_id
// ------------------------------------------------------------
app.get("/evolucao/:paciente_id", async (req, res) => {
  const paciente_id = parseInt(req.params.paciente_id);
  if (isNaN(paciente_id)) return res.status(400).json({ erro: "paciente_id inválido." });

  try {
    const [evolucoes] = await db.query(
      `SELECT ep.id, ep.observacao, ep.registrado_em, m.nome_completo AS medico
       FROM EvolucaoPaciente ep
       LEFT JOIN Medico m ON ep.medico_id = m.id
       WHERE ep.paciente_id = ?
       ORDER BY ep.registrado_em DESC`,
      [paciente_id]
    );

    if (evolucoes.length === 0) {
      return res.status(404).json({ mensagem: `Nenhuma evolução para o paciente ${paciente_id}.` });
    }

    return res.status(200).json({ paciente_id, total: evolucoes.length, evolucoes });

  } catch (err) {
    console.error(`[GET /evolucao] Erro:`, err.message);
    return res.status(500).json({ erro: "Erro interno." });
  }
});

// ------------------------------------------------------------
// Rota raiz
// ------------------------------------------------------------
app.get("/", (req, res) => {
  res.sendFile(path.join(__dirname, "public", "index2.html"));
});

// ------------------------------------------------------------
// Inicia
// ------------------------------------------------------------
conectarBanco().then(() => {
  app.listen(PORT, () => {
    console.log(`[API] Rodando em http://localhost:${PORT}`);
  });
});