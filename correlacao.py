import serial
import threading
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

# ==========================
# CONFIG
# ==========================
SERIAL_PORT = "COM6"
BAUDRATE = 115200

WINDOW = 250

N_AMOSTRAS = 256

# ==========================
# ESTADO COMPARTILHADO
# ==========================
dados = {
    "mic1": [],
    "mic2": [],
    "lag": None,       # ORIGINAL_LAG recebido do ESP32
    "novo": False,
}

historico_pico    = deque(maxlen=100)
buffer_mediana    = deque(maxlen=10)

lock = threading.Lock()

# ==========================
# THREAD SERIAL
# ==========================
def receptor_serial():

    ser = serial.Serial(
        SERIAL_PORT,
        BAUDRATE,
        timeout=1
    )

    print(f"[receiver] Serial conectada em {SERIAL_PORT}")

    mic1_tmp = []
    mic2_tmp = []
    lag_tmp  = None
    modo     = None

    try:

        while True:

            linha = ser.readline().decode(
                errors="ignore"
            ).strip()

            if not linha:
                continue

            # ==========================
            # HEADERS
            # ==========================
            if linha == "MIC1:":
                modo     = "mic1"
                mic1_tmp = []
                continue

            elif linha == "MIC2:":
                modo     = "mic2"
                mic2_tmp = []
                continue

            # ==========================
            # LAG DO MICROCONTROLADOR
            # ==========================
            elif linha.startswith("ORIGINAL_LAG:"):
                try:
                    lag_tmp = int(linha.split(":")[1])
                except:
                    pass
                continue

            # GCC_LAG ignorado
            elif linha.startswith("GCC_LAG:"):
                continue

            # ==========================
            # FINAL PACOTE
            # ==========================
            elif linha == "END":

                with lock:
                    dados["mic1"] = mic1_tmp[:]
                    dados["mic2"] = mic2_tmp[:]
                    dados["lag"]  = lag_tmp
                    dados["novo"] = True

                print(
                    f"[RX] "
                    f"mic1={len(mic1_tmp)} "
                    f"mic2={len(mic2_tmp)} "
                    f"lag={lag_tmp}"
                )

                mic1_tmp = []
                mic2_tmp = []
                lag_tmp  = None
                modo     = None
                continue

            # ==========================
            # DADOS
            # ==========================
            try:
                val = int(linha)

                if modo == "mic1":
                    mic1_tmp.append(val)

                elif modo == "mic2":
                    mic2_tmp.append(val)

            except:
                pass

    except Exception as e:
        print(f"[receiver] erro: {e}")

    finally:
        ser.close()
        print("[receiver] serial desconectada")

# ==========================
# THREAD
# ==========================
thread = threading.Thread(
    target=receptor_serial,
    daemon=True
)

thread.start()

# ==========================
# FIGURA  — 2x2, mesmo layout original
# ==========================
fig, axes = plt.subplots(
    2,
    2,
    figsize=(14, 8)
)

ax_mic1      = axes[0, 0]
ax_mic2      = axes[0, 1]
ax_corr      = axes[1, 0]
ax_historico = axes[1, 1]

# ==========================
# LINHAS
# ==========================
line_mic1,      = ax_mic1.plot([], [], lw=1.5, color="#1f77b4")
line_mic2,      = ax_mic2.plot([], [], lw=1.5, color="#ff7f0e")
line_corr,      = ax_corr.plot([], [], lw=1.5, color="#2ca02c")
line_historico, = ax_historico.plot([], [], lw=1.5, color="#9467bd")

vline_corr = ax_corr.axvline(
    x=0,
    color="red",
    linestyle="--",
    lw=1
)

# ==========================
# CONFIG EIXOS
# ==========================
for ax, titulo in [
    (ax_mic1,      "MIC1 RAW"),
    (ax_mic2,      "MIC2 RAW"),
    (ax_corr,      "Correlação cruzada"),
    (ax_historico, "Histórico posição do pico (mediana)"),
]:
    ax.set_title(titulo)
    ax.set_xlabel("Amostra")
    ax.grid(True)

ax_mic1.set_ylabel("ADC")
ax_mic2.set_ylabel("ADC")
ax_corr.set_ylabel("Correlação")
ax_historico.set_ylabel("Posição do pico")

ax_mic1.set_ylim(0, 4200)
ax_mic2.set_ylim(0, 4200)
ax_corr.set_ylim(-1.1, 1.1)

# ==========================
# UPDATE
# ==========================
def update(_):

    with lock:

        if not dados["novo"]:
            return (
                line_mic1,
                line_mic2,
                line_corr,
                line_historico,
                vline_corr,
            )

        dados["novo"] = False

        mic1 = dados["mic1"][:]
        mic2 = dados["mic2"][:]
        lag  = dados["lag"]

    # ==========================
    # MIC1
    # ==========================
    if len(mic1) > 0:
        mic1_plot = mic1[-WINDOW:]
        line_mic1.set_data(
            np.arange(len(mic1_plot)),
            mic1_plot
        )
        ax_mic1.set_xlim(0, WINDOW)

    # ==========================
    # MIC2
    # ==========================
    if len(mic2) > 0:
        mic2_plot = mic2[-WINDOW:]
        line_mic2.set_data(
            np.arange(len(mic2_plot)),
            mic2_plot
        )
        ax_mic2.set_xlim(0, WINDOW)

    # ==========================
    # CORRELAÇÃO — calculada no Python com os
    # dados brutos recebidos, apenas para exibição.
    # O pico (vline) vem do ORIGINAL_LAG do ESP32.
    # ==========================
    if len(mic1) >= N_AMOSTRAS and len(mic2) >= N_AMOSTRAS:

        v1 = np.array(mic1[:N_AMOSTRAS], dtype=np.float32)
        v2 = np.array(mic2[:N_AMOSTRAS], dtype=np.float32)

        # Remove offset DC (igual ao ESP32)
        v1 -= np.mean(v1)
        v2 -= np.mean(v2)

        # Normaliza (espelha a normalização do ESP32)
        m1 = np.max(np.abs(v1)) or 1
        m2 = np.max(np.abs(v2)) or 1
        v1 /= m1
        v2 /= m2

        corr = np.correlate(v1, v2, mode="full")

        corr_norm = corr / (np.max(np.abs(corr)) or 1)

        line_corr.set_data(
            np.arange(len(corr_norm)),
            corr_norm
        )

        ax_corr.set_xlim(0, len(corr_norm))

        # =====================================================
        # PICO = max_index_corr recebido do ESP32
        # O ESP32 armazena em max_index_corr o índice
        # absoluto no array de correlação (0..CORR_TAMANHO-1).
        # ORIGINAL_LAG = max_index_corr - (N_AMOSTRAS - 1)
        # Portanto: max_index_corr = lag + (N_AMOSTRAS - 1)
        # =====================================================
        if lag is not None:
            max_index_corr = lag + (N_AMOSTRAS - 1)
            vline_corr.set_xdata([max_index_corr])

    # ==========================
    # MEDIANA + HISTÓRICO
    # ==========================
    if lag is not None:

        # Converte lag para índice absoluto igual ao ESP32
        max_index_corr = lag + (N_AMOSTRAS - 1)

        buffer_mediana.append(max_index_corr)
        pico_med = int(np.median(buffer_mediana))
        historico_pico.append(pico_med)

        print(
            f"[PLOT] "
            f"lag={lag} "
            f"max_index={max_index_corr} "
            f"mediana={pico_med}"
        )

    # ==========================
    # GRÁFICO HISTÓRICO
    # ==========================
    if len(historico_pico) > 0:
        xs = np.arange(len(historico_pico))
        ys = list(historico_pico)
        line_historico.set_data(xs, ys)
        ax_historico.set_xlim(0, max(100, len(historico_pico)))
        margin = 5
        ax_historico.set_ylim(
            min(ys) - margin,
            max(ys) + margin
        )

    return (
        line_mic1,
        line_mic2,
        line_corr,
        line_historico,
        vline_corr,
    )

# ==========================
# ANIMAÇÃO
# ==========================
ani = animation.FuncAnimation(
    fig,
    update,
    interval=50,
    blit=False,
    cache_frame_data=False,
)

plt.tight_layout()
plt.show()