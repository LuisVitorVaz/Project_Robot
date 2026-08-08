import serial
import threading
import queue
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import time
import csv
from datetime import datetime

# ==========================================================
# CONFIGURAÇÕES GERAIS
# ==========================================================
SERIAL_PORT_MASTER = "COM4"   # ESP32 MASTER -> MIC1
SERIAL_PORT_SLAVE  = "COM6"   # ESP32 SLAVE  -> MIC2
BAUDRATE = 921600

N_AMOSTRAS = 256
# TAU_MAX_AMOSTRAS = 58
CORR_TAMANHO = (2 * N_AMOSTRAS) - 1
WINDOW = 250   # amostras exibidas no gráfico ao vivo

# ---- Configurações do experimento ----
ANGULOS = [0, 45, 90, 135, 180]      # ângulos que serão testados, nesta ordem
DURACAO_TESTE_SEGUNDOS = 120         # 2 minutos por ângulo


TIMESTAMP_EXECUCAO = datetime.now().strftime("%Y%m%d_%H%M%S")
ARQUIVO_TABELA = f"tabela_resultados_{TIMESTAMP_EXECUCAO}.csv"
ARQUIVO_BRUTO  = f"dados_brutos_{TIMESTAMP_EXECUCAO}.csv"
ARQUIVO_GRAFICO = f"grafico_correlacao_por_angulo_{TIMESTAMP_EXECUCAO}.png"

# ==========================================================
# ESTADO COMPARTILHADO (agora via filas, para não perder blocos)
# ==========================================================
# Cada bloco completo recebido (entre "MIC:" e "END") é colocado na fila
# correspondente. O loop principal só remove da fila quando vai processar,
# então nenhum bloco é sobrescrito antes de ser usado.
fila_mic1 = queue.Queue()
fila_mic2 = queue.Queue()

# ==========================================================
# FUNÇÕES MATEMÁTICAS
# ==========================================================

def normalizar(sinal):
    """Remove DC (subtrai a média) e normaliza pela amplitude (divide pelo desvio padrão)."""
    arr = np.asarray(sinal, dtype=np.float64)
    media = np.mean(arr)
    desvio = np.std(arr)
    if desvio == 0:
        return arr - media
    return (arr - media) / desvio

#     """Correlação cruzada entre dois sinais já normalizados (float)."""

def correlacao_cruzada(Mic1, Mic2):
    res = np.zeros(CORR_TAMANHO, dtype=np.float64)
    for i in range(CORR_TAMANHO):
        soma = 0.0
        k = N_AMOSTRAS - 1
        j = 0
        # 
        # if j > (N_AMOSTRAS - 1):   
        #     k -= (j - (N_AMOSTRAS - 1)) 
        #     j = N_AMOSTRAS - 1
        while j <= N_AMOSTRAS - 1 and k >= 0:
            soma += Mic2[k] * Mic1[j]
            j += 1
            k -= 1
            res[j] = soma /j
    return res

#     """Retorna (indice_do_pico, valor_do_pico) dentro da janela de busca ao redor do centro."""

# def encontrar_pico(corr):
#     inicio = (N_AMOSTRAS - 1) - TAU_MAX_AMOSTRAS
#     fim = (N_AMOSTRAS - 1) + TAU_MAX_AMOSTRAS

#     max_val = corr[inicio]
#     max_index = inicio

#     for i in range(inicio, fim + 1):
#         if corr[i] > max_val:
#             max_val = corr[i]
#             max_index = i

#     return max_index, max_val
# argmax funcao que procura o maior valor no vetor e retorna a posicao
def encontrar_pico(corr):
    max_index = np.argmax(corr)
    max_val = corr[max_index]
    return max_index, max_val


# ==========================================================
# THREAD SERIAL (agora coloca cada bloco completo na fila)
# ==========================================================
def receptor_serial(porta, fila_destino, nome_log):
    try:
        ser = serial.Serial(porta, BAUDRATE, timeout=1)
        ser.reset_input_buffer()
        print(f"[receiver:{nome_log}] Serial conectada em {porta}")
    except Exception as e:
        print(f"Erro ao abrir {porta}: {e}")
        return

    mic_tmp = []
    modo    = None

    try:
        while True:
            linha = ser.readline().decode(errors="ignore").strip()

            if not linha:
                continue

            if linha.startswith("DEVICE:") or linha.startswith("FS:"):
                continue

            elif linha == "MIC:":
                modo    = "mic"
                mic_tmp = []
                continue

            elif linha == "END":
                # Coloca o bloco completo na fila (não sobrescreve nada,
                # cada bloco fica esperando até ser consumido no loop principal)
                fila_destino.put(mic_tmp[:])
                mic_tmp = []
                modo    = None
                continue

            try:
                val = int(linha)
                if modo == "mic":
                    mic_tmp.append(val)
            except:
                pass

    except Exception as e:
        print(f"[receiver:{nome_log}] erro: {e}")

    finally:
        ser.close()
        print(f"[receiver:{nome_log}] serial desconectada")

thread_master = threading.Thread(target=receptor_serial, args=(SERIAL_PORT_MASTER, fila_mic1, "mic1"), daemon=True)
thread_slave  = threading.Thread(target=receptor_serial, args=(SERIAL_PORT_SLAVE, fila_mic2, "mic2"), daemon=True)
thread_master.start()
thread_slave.start()

# ==========================================================
# JANELA DE MONITORAMENTO AO VIVO (mantida)
# ==========================================================
plt.ion()
fig, (ax_mic1, ax_mic2) = plt.subplots(1, 2, figsize=(11, 4))
line_mic1, = ax_mic1.plot([], [], lw=1.2, color="#1f77b4")
line_mic2, = ax_mic2.plot([], [], lw=1.2, color="#ff7f0e")

for ax, titulo in [(ax_mic1, "MIC1 (MASTER)"), (ax_mic2, "MIC2 (SLAVE)")]:
    ax.set_title(titulo)
    ax.set_xlabel("Amostra")
    ax.set_ylabel("ADC")
    ax.set_ylim(-2100, 2100)
    ax.set_xlim(0, WINDOW)
    ax.grid(True)

fig.tight_layout()
plt.show(block=False)
fig.canvas.draw()
fig.canvas.flush_events()

def atualizar_grafico_ao_vivo(mic1, mic2, angulo_atual, segundos_restantes):
    if len(mic1) > 0:
        m1 = mic1[-WINDOW:]
        line_mic1.set_data(np.arange(len(m1)), m1)
    if len(mic2) > 0:
        m2 = mic2[-WINDOW:]
        line_mic2.set_data(np.arange(len(m2)), m2)
    fig.suptitle(f"Ângulo atual: {angulo_atual}°  |  Tempo restante: {segundos_restantes:.0f}s")
    fig.canvas.draw_idle()
    plt.pause(0.001)

# ==========================================================
# ARQUIVO DE DADOS BRUTOS (uma linha por bloco processado)
# ==========================================================
arquivo_bruto = open(ARQUIVO_BRUTO, mode="w", newline="", encoding="utf-8")
escritor_bruto = csv.writer(arquivo_bruto)
escritor_bruto.writerow(["angulo", "timestamp", "posicao_correlacao_maxima"])

# ==========================================================
# EXPORTAÇÃO DO MELHOR BLOCO DE CADA ÂNGULO PARA .XLSX
# (formato compatível com o script de validação: colunas "Mic1"/"Mic2",
#  256 linhas = N_AMOSTRAS, sem índice)
# ==========================================================
def salvar_melhor_bloco_xlsx(angulo, mic1_raw, mic2_raw, pico_valor):
    """Salva o par de blocos brutos (256 amostras cada) que produziu o maior
    pico de correlação naquele ângulo, no formato Mic1/Mic2 esperado pelo
    script de validação/teste (senoides_...xlsx)."""
    nome_arquivo = f"sinal_{angulo}graus_{TIMESTAMP_EXECUCAO}.xlsx"
    df = pd.DataFrame({
        "Mic1": mic1_raw,
        "Mic2": mic2_raw,
    })
    df.to_excel(nome_arquivo, index=False)
    print(f"[xlsx] Melhor bloco do ângulo {angulo}° salvo em: {nome_arquivo} "
          f"(pico de correlação = {pico_valor:.4f})")
    return nome_arquivo

# ==========================================================
# LOOP PRINCIPAL DO EXPERIMENTO
# ==========================================================
resultados = []

print("\n===== INÍCIO DO EXPERIMENTO =====")
print(f"Ângulos a testar: {ANGULOS}")
print(f"Duração por ângulo: {DURACAO_TESTE_SEGUNDOS}s ({DURACAO_TESTE_SEGUNDOS/60:.1f} min)\n")

try:
    for angulo in ANGULOS:
        input(f">>> Posicione o microfone em {angulo}° e pressione ENTER para iniciar a coleta...")
        print(f"Coletando dados para {angulo}°...")

        posicoes_coletadas = []   # vetor com a posição (índice) onde ocorreu o pico da correlação, a cada bloco, nos 2 minutos
        ultimo_aviso = time.time()
        recebeu_algum_dado = False

        # ---- rastreio do "melhor" bloco deste ângulo (maior pico de correlação) ----
        melhor_pico_valor = -np.inf
        melhor_mic1_raw = None
        melhor_mic2_raw = None

        # Ao iniciar um novo ângulo, descarta blocos antigos que ainda
        # estejam nas filas (de antes do usuário posicionar o microfone),
        # para não misturar dados de ângulos diferentes.
        while not fila_mic1.empty():
            fila_mic1.get_nowait()
        while not fila_mic2.empty():
            fila_mic2.get_nowait()

        inicio = time.time()
        while True:
            decorrido = time.time() - inicio
            restante = DURACAO_TESTE_SEGUNDOS - decorrido
            if restante <= 0:
                break

            novo = False
            # Só retira das filas quando AMBAS já têm pelo menos um bloco
            # disponível. Isso garante que nenhum bloco é descartado: ele
            # fica esperando na fila até o outro microfone também ter dado
            # novo, em vez de ser sobrescrito.
            if not fila_mic1.empty() and not fila_mic2.empty():
                mic1 = fila_mic1.get()
                mic2 = fila_mic2.get()
                novo = True

            if novo:
                recebeu_algum_dado = True

                if len(mic1) >= N_AMOSTRAS and len(mic2) >= N_AMOSTRAS:
                    raw1 = mic1[:N_AMOSTRAS]
                    raw2 = mic2[:N_AMOSTRAS]

                    # ---- normalização do sinal antes da correlação ----
                    Mic1 = normalizar(raw1)
                    Mic2 = normalizar(raw2)

                    corr = correlacao_cruzada(Mic1, Mic2)
                    indice, valor_pico = encontrar_pico(corr)
                    posicoes_coletadas.append(indice)

                    # Bloco com maior pico de correlação até agora vira o
                    # candidato a ser exportado para o .xlsx deste ângulo.
                    if valor_pico > melhor_pico_valor:
                        melhor_pico_valor = valor_pico
                        melhor_mic1_raw = raw1
                        melhor_mic2_raw = raw2

                    escritor_bruto.writerow([
                        angulo,
                        datetime.now().isoformat(timespec="milliseconds"),
                        indice,
                    ])
                    arquivo_bruto.flush()
                else:
                    print(f"[aviso] pacote recebido mas curto demais: len(mic1)={len(mic1)}, len(mic2)={len(mic2)} (precisa >= {N_AMOSTRAS})")

                atualizar_grafico_ao_vivo(mic1, mic2, angulo, restante)
            else:
                plt.pause(0.01)

            # aviso a cada 5s se nenhum dado novo chegou nesse intervalo
            if time.time() - ultimo_aviso >= 5:
                ultimo_aviso = time.time()
                if not recebeu_algum_dado:
                    print(f"[aviso] {restante:.0f}s restantes e ainda nenhum dado recebido de MIC1/MIC2 nesse ângulo. Verifique as portas seriais.")
                print(f"[status] blocos processados até agora: {len(posicoes_coletadas)} "
                      f"(pendentes na fila -> mic1: {fila_mic1.qsize()}, mic2: {fila_mic2.qsize()})")

        # ---- Estatísticas consolidadas deste ângulo: apenas média e desvio padrão da posição do pico ----
        media_pos = float(np.mean(posicoes_coletadas)) if posicoes_coletadas else 0.0
        desvio_pos = float(np.std(posicoes_coletadas)) if posicoes_coletadas else 0.0

        resultados.append({
            "angulo_graus": angulo,
            "n_blocos": len(posicoes_coletadas),
            "media_posicao": media_pos,
            "desvio_padrao_posicao": desvio_pos,
        })

        print(
            f"Concluído {angulo}° -> "
            f"média da posição do pico = {media_pos:.4f} | "
            f"desvio padrão = {desvio_pos:.4f} "
            f"(n={len(posicoes_coletadas)} blocos)"
        )

        # ---- Exporta o melhor bloco deste ângulo em .xlsx (formato do teste) ----
        if melhor_mic1_raw is not None and melhor_mic2_raw is not None:
            salvar_melhor_bloco_xlsx(angulo, melhor_mic1_raw, melhor_mic2_raw, melhor_pico_valor)
        else:
            print(f"[aviso] Nenhum bloco válido coletado em {angulo}° — .xlsx não gerado para este ângulo.")

except KeyboardInterrupt:
    print("\nExperimento interrompido pelo usuário.")

finally:
    arquivo_bruto.close()
    print(f"[csv] Dados brutos salvos em: {ARQUIVO_BRUTO}")

# ==========================================================
# TABELA FINAL DE RESULTADOS
# ==========================================================
if resultados:
    campos = ["angulo_graus", "n_blocos", "media_posicao", "desvio_padrao_posicao"]

    with open(ARQUIVO_TABELA, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=campos)
        writer.writeheader()
        for r in resultados:
            writer.writerow(r)

    print(f"\n[csv] Tabela final salva em: {ARQUIVO_TABELA}\n")

    # Exibe a tabela no console também
    cabecalho = "{:<10}{:<10}{:<18}{:<18}".format(
        "Ângulo", "N", "MédiaPosição", "DesvioPosição")
    print(cabecalho)
    print("-" * len(cabecalho))
    for r in resultados:
        print("{:<10}{:<10}{:<18.4f}{:<18.4f}".format(
            r["angulo_graus"], r["n_blocos"], r["media_posicao"], r["desvio_padrao_posicao"]))

    # ---- Gráfico final: posição média do pico x ângulo (com barra de erro) ----
    angulos_plot = [r["angulo_graus"] for r in resultados]
    medias_plot  = [r["media_posicao"] for r in resultados]
    desvios_plot = [r["desvio_padrao_posicao"] for r in resultados]

    plt.ioff()
    fig2, ax2 = plt.subplots(figsize=(8, 5))
    ax2.errorbar(angulos_plot, medias_plot, yerr=desvios_plot, fmt="o-", capsize=5, color="#1f77b4")
    ax2.set_xlabel("Ângulo (graus)")
    ax2.set_ylabel("Posição média do pico de correlação (índice)")
    ax2.set_title("Posição média do pico por ângulo (barras = desvio padrão)")
    ax2.grid(True)
    plt.tight_layout()
    plt.savefig(ARQUIVO_GRAFICO, dpi=150)
    print(f"[png] Gráfico salvo em: {ARQUIVO_GRAFICO}")
    plt.show()
else:
    print("Nenhum resultado coletado.")