// ============================================================
// sensor.js
// ============================================================

const API_BASE = "http://localhost:3000";

let intervaloSensor = null;
let quartoAtualId   = null;

// Consulta /status a cada 1s
setInterval(async () => {
  try {
    const res    = await fetch(`${API_BASE}/status`, { cache: "no-store" });
    const status = await res.json();
    const novoQuarto = status.quarto_id ?? null;

    // Só recarrega o front quando o quarto MUDAR
    if (novoQuarto !== quartoAtualId) {
      quartoAtualId = novoQuarto;

      if (quartoAtualId) {
        console.log(`[Status] Mudou para quarto ${quartoAtualId}`);
        const pacienteId = await buscarPaciente(quartoAtualId);
        if (pacienteId) await buscarEvolucoes(pacienteId);
        iniciarPolling(quartoAtualId);
      } else {
        pararPolling();
        limparTela();
      }
    }

  } catch (err) {
    console.error("[Status] Erro:", err);
  }
}, 1000);

// ------------------------------------------------------------
// Busca paciente e preenche os cards
// ------------------------------------------------------------
async function buscarPaciente(quartoId) {
  try {
    const res = await fetch(`${API_BASE}/paciente/${quartoId}`, { cache: "no-store" });
    if (!res.ok) return null;

    const dados = await res.json();
    const p = dados.paciente;
    const l = dados.leito;
    const m = dados.medico;

    setText("paciente-nome",                p.nome              ?? "-");
    setText("paciente-idade",               calcularIdade(p.data_nascimento));
    setText("paciente-sangue",              p.tipo_sanguineo    ?? "-");
    setText("paciente-alergias",            p.alergias          ?? "Nenhuma");
    setText("paciente-internado",           formatarData(p.internado_em));
    setText("paciente-contato-emergencia",  p.contato_emergencia  ?? "-");
    setText("paciente-telefone-emergencia", p.telefone_emergencia ?? "-");

    setText("leito-numero", l.numero ?? "-");
    setText("leito-ala",    l.ala    ?? "-");
    setText("leito-andar",  l.andar  ?? "-");

    setText("medico-nome",          m.nome          ?? "-");
    setText("medico-especialidade", m.especialidade ?? "-");
    setText("medico-telefone",      m.telefone      ?? "-");

    return p.id;

  } catch (err) {
    console.error("[Paciente] Erro:", err);
    return null;
  }
}

// ------------------------------------------------------------
// Busca evoluções
// ------------------------------------------------------------
async function buscarEvolucoes(pacienteId) {
  const container = document.getElementById("evolucoes-container");
  if (!container) return;

  try {
    const res = await fetch(`${API_BASE}/evolucao/${pacienteId}`, { cache: "no-store" });

    if (!res.ok) {
      container.innerHTML = `<p class="content-text" style="opacity:0.5;padding:1rem;">Nenhuma evolução encontrada.</p>`;
      return;
    }

    const dados = await res.json();
    container.innerHTML = "";

    dados.evolucoes.forEach((ev, i) => {
      const hora = new Date(ev.registrado_em).toLocaleTimeString("pt-BR", { hour: "2-digit", minute: "2-digit" });
      const data = new Date(ev.registrado_em).toLocaleDateString("pt-BR");

      const card = document.createElement("div");
      card.className = "pacient-evolution-history-card";
      card.innerHTML = `
        <div class="pacient-evolution-history-header">
          <h4>Evolução #${i + 1} : ${hora}</h4>
          <p>Data: ${data}</p>
        </div>
        <div class="pacient-evolution-history-body">
          <h5>Médico: ${ev.medico ?? "Não informado"}</h5>
          <hr style="border: 1px solid rgb(186,186,186)" />
          <p>${ev.observacao}</p>
        </div>
      `;
      container.appendChild(card);

      if (i < dados.evolucoes.length - 1) {
        const spacer = document.createElement("div");
        spacer.className = "spacer-md";
        container.appendChild(spacer);
      }
    });

  } catch (err) {
    console.error("[Evoluções] Erro:", err);
  }
}

// ------------------------------------------------------------
// Polling de sensores a cada 5s
// ------------------------------------------------------------
function iniciarPolling(quartoId) {
  pararPolling();
  buscarSensores(quartoId);
  intervaloSensor = setInterval(() => buscarSensores(quartoId), 5000);
}

function pararPolling() {
  if (intervaloSensor) {
    clearInterval(intervaloSensor);
    intervaloSensor = null;
  }
}

async function buscarSensores(quartoId) {
  try {
    const res = await fetch(`${API_BASE}/sensor/${quartoId}`, { cache: "no-store" });
    if (!res.ok) return;

    const dados  = await res.json();
    const ultima = dados.leituras[0];

    setText("temperatura",  ultima.temperatura  != null ? `${ultima.temperatura}°C` : "-");
    setText("umidade",      ultima.umidade       != null ? `${ultima.umidade}%`      : "-");
    setText("luminosidade", ultima.luminosidade  != null ? `${ultima.luminosidade}%` : "-");
    setText("atualizado",   formatarData(ultima.registrado_em));

  } catch (err) {
    console.error("[Sensor] Erro:", err);
  }
}

// ------------------------------------------------------------
// Limpa tela
// ------------------------------------------------------------
function limparTela() {
  const ids = [
    "paciente-nome", "paciente-idade", "paciente-sangue",
    "paciente-alergias", "paciente-internado",
    "paciente-contato-emergencia", "paciente-telefone-emergencia",
    "leito-numero", "leito-ala", "leito-andar",
    "medico-nome", "medico-especialidade", "medico-telefone",
    "temperatura", "umidade", "luminosidade", "atualizado",
  ];
  ids.forEach(id => setText(id, "-"));

  const container = document.getElementById("evolucoes-container");
  if (container) {
    container.innerHTML = `<p class="content-text" style="opacity:0.5;padding:1rem;">Aguardando comando da Alexa...</p>`;
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
  return new Date(dataStr).toLocaleString("pt-BR");
}

function calcularIdade(dataNasc) {
  if (!dataNasc) return "-";
  const hoje = new Date();
  const nasc = new Date(dataNasc);
  let idade  = hoje.getFullYear() - nasc.getFullYear();
  const m    = hoje.getMonth() - nasc.getMonth();
  if (m < 0 || (m === 0 && hoje.getDate() < nasc.getDate())) idade--;
  return `${idade} anos`;
}