// ============================================================
// sensor.js
// Script do frontend para atualizar os dados na tela
// automaticamente quando a Alexa acionar um quarto
// ============================================================
// Como usar:
//   Inclua este script no seu HTML:
//   <script src="sensor.js"></script>
//
// Elementos HTML necessários (com esses IDs):
//   <span id="quarto-ativo"></span>
//   <span id="temperatura"></span>
//   <span id="umidade"></span>
//   <span id="luminosidade"></span>
//   <span id="atualizado"></span>
//   <span id="status-conexao"></span>
// ============================================================

const API_BASE = "http://192.168.1.100:3000";
const API_KEY  = "tPmAT5Ab3j7F9";

let intervaloSensor = null; // polling dos dados do sensor
let quartoAtualId   = null; // quarto que está sendo monitorado

// ------------------------------------------------------------
// Consulta o /status a cada 3s para saber qual quarto
// a Alexa ativou e inicia o polling de dados automaticamente
// ------------------------------------------------------------
setInterval(async () => {
  try {
    const res    = await fetch(`${API_BASE}/status`);
    const status = await res.json();

    // Só age se o quarto mudou
    if (status.quarto_id !== quartoAtualId) {
      quartoAtualId = status.quarto_id;

      if (quartoAtualId) {
        console.log(`[Status] Alexa ativou quarto ${quartoAtualId}`);
        atualizarStatusConexao(`Monitorando quarto ${quartoAtualId}`, "ativo");
        iniciarPolling(quartoAtualId);
      } else {
        console.log("[Status] Nenhum quarto ativo");
        atualizarStatusConexao("Aguardando comando da Alexa...", "inativo");
        pararPolling();
      }
    }

  } catch (err) {
    console.error("[Status] Erro ao consultar /status:", err);
    atualizarStatusConexao("Sem conexão com a API", "erro");
  }
}, 3000);

// ------------------------------------------------------------
// Inicia o polling de dados do sensor para o quarto ativo
// ------------------------------------------------------------
function iniciarPolling(quartoId) {
  // Cancela o polling anterior
  pararPolling();

  // Busca imediatamente e depois a cada 5s
  buscarDados(quartoId);
  intervaloSensor = setInterval(() => buscarDados(quartoId), 5000);
}

// ------------------------------------------------------------
// Para o polling de dados
// ------------------------------------------------------------
function pararPolling() {
  if (intervaloSensor) {
    clearInterval(intervaloSensor);
    intervaloSensor = null;
  }
}

// ------------------------------------------------------------
// Busca os dados do sensor na API e atualiza a tela
// ------------------------------------------------------------
async function buscarDados(quartoId) {
  try {
    const res = await fetch(`${API_BASE}/sensor/${quartoId}`, {
      headers: { "x-api-key": API_KEY }
    });

    if (!res.ok) {
      console.warn(`[Sensor] API retornou ${res.status} para quarto ${quartoId}`);
      return;
    }

    const dados  = await res.json();
    const ultima = dados.leituras[0]; // leitura mais recente

    // Atualiza os elementos na tela
    setText("quarto-ativo",   `Quarto ${quartoId}`);
    setText("temperatura",    `${ultima.temperatura}°C`);
    setText("umidade",        `${ultima.umidade}%`);
    setText("luminosidade",   `${ultima.luminosidade}%`);
    setText("atualizado",     formatarData(ultima.registrado_em));

    console.log(`[Sensor] Q${quartoId} → Temp: ${ultima.temperatura}°C | Umidade: ${ultima.umidade}% | Lum: ${ultima.luminosidade}%`);

  } catch (err) {
    console.error("[Sensor] Erro ao buscar dados:", err);
  }
}

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------
function setText(id, valor) {
  const el = document.getElementById(id);
  if (el) el.textContent = valor;
}

function formatarData(dataStr) {
  if (!dataStr) return "-";
  const d = new Date(dataStr);
  return d.toLocaleString("pt-BR");
}

function atualizarStatusConexao(mensagem, estado) {
  const el = document.getElementById("status-conexao");
  if (!el) return;
  el.textContent = mensagem;
  el.className   = `status-${estado}`; // aplique estilos via CSS: .status-ativo, .status-inativo, .status-erro
}