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

WINDOW = 500

# ==========================
# ESTADO COMPARTILHADO
# ==========================
dados = {
    "mic1": [],
    "mic2": [],
    "correlacao": [],
    "max_index": 0,
    "max_val": 0,
    "angulo": 0,
    "novo": False,
}

historico_angulo = deque(maxlen=100)

lock = threading.Lock()

# ==========================
# THREAD — SERIAL
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

    modo = None

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

                modo = "mic1"

                mic1_tmp = []

                continue

            elif linha == "MIC2:":

                modo = "mic2"

                mic2_tmp = []

                continue

            # ==========================
            # FINAL DO PACOTE
            # ==========================
            elif linha == "END":

                with lock:

                    dados["mic1"] = mic1_tmp[:]

                    dados["mic2"] = mic2_tmp[:]

                    dados["novo"] = True

                print(
                    f"[RX] mic1={len(mic1_tmp)} "
                    f"mic2={len(mic2_tmp)}"
                )

                mic1_tmp = []
                mic2_tmp = []

                modo = None

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
# INICIA THREAD
# ==========================
thread = threading.Thread(
    target=receptor_serial,
    daemon=True
)

thread.start()

# ==========================
# PLOT
# ==========================
fig, axes = plt.subplots(2, 2, figsize=(14, 8))

ax_mic1 = axes[0, 0]
ax_mic2 = axes[0, 1]
ax_corr = axes[1, 0]
ax_ang  = axes[1, 1]

# ==========================
# LINHAS
# ==========================
line_mic1, = ax_mic1.plot([], [], lw=1.5)

line_mic2, = ax_mic2.plot([], [], lw=1.5)

line_corr, = ax_corr.plot([], [], lw=1.5)

line_ang, = ax_ang.plot([], [], lw=1.5)

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
    (ax_mic1, "MIC1 RAW"),
    (ax_mic2, "MIC2 RAW"),
    (ax_corr, "Correlação cruzada"),
    (ax_ang,  "Histórico ângulo"),
]:

    ax.set_title(titulo)

    ax.set_xlabel("Amostra")

    ax.grid(True)

ax_mic1.set_ylabel("ADC")
ax_mic2.set_ylabel("ADC")
ax_corr.set_ylabel("Correlação")
ax_ang.set_ylabel("Ângulo")

ax_mic1.set_ylim(0, 4200)
ax_mic2.set_ylim(0, 4200)

ax_corr.set_ylim(-1.1, 1.1)

ax_ang.set_ylim(0, 180)

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
                line_ang,
                vline_corr,
            )

        dados["novo"] = False

        mic1 = dados["mic1"][:]

        mic2 = dados["mic2"][:]

    # ==========================
    # MIC1
    # ==========================
    if len(mic1) > 0:

        if len(mic1) > WINDOW:

            mic1_plot = mic1[-WINDOW:]

        else:

            mic1_plot = mic1

        line_mic1.set_data(
            np.arange(len(mic1_plot)),
            mic1_plot
        )

        ax_mic1.set_xlim(0, WINDOW)

    # ==========================
    # MIC2
    # ==========================
    if len(mic2) > 0:

        if len(mic2) > WINDOW:

            mic2_plot = mic2[-WINDOW:]

        else:

            mic2_plot = mic2

        line_mic2.set_data(
            np.arange(len(mic2_plot)),
            mic2_plot
        )

        ax_mic2.set_xlim(0, WINDOW)

    # ==========================
    # CORRELAÇÃO
    # ==========================
    if len(mic1) > 200 and len(mic2) > 200:

        v1 = np.array(mic1, dtype=np.float32)

        v2 = np.array(mic2, dtype=np.float32)

        # ==========================
        # REMOVE OFFSET DC
        # ==========================
        v1 = v1 - np.mean(v1)

        v2 = v2 - np.mean(v2)

        # ==========================
        # PASSA-ALTA SIMPLES
        # ==========================
        v1[1:] = v1[1:] - v1[:-1]

        v2[1:] = v2[1:] - v2[:-1]

        # ==========================
        # CORRELAÇÃO
        # ==========================
        corr = np.correlate(
            v1[:100],
            v2[:100],
            mode="full"
        )

        corr_norm = corr / (np.max(np.abs(corr)) or 1)

        pico = np.argmax(corr_norm)

        line_corr.set_data(
            np.arange(len(corr_norm)),
            corr_norm
        )

        ax_corr.set_xlim(
            0,
            len(corr_norm)
        )

        vline_corr.set_xdata([pico])

        # ==========================
        # AJUSTE COMPATÍVEL COM ESP32
        # ==========================
        max_index = pico + 1

        angulo = int(
            (-0.174 * (max_index ** 2)) +
            (10.6 * max_index) +
            0.122
        )

        angulo = max(0, min(180, angulo))

        historico_angulo.append(angulo)

        line_ang.set_data(
            np.arange(len(historico_angulo)),
            list(historico_angulo)
        )

        ax_ang.set_xlim(
            0,
            max(100, len(historico_angulo))
        )

        print(
            f"[PLOT] max_index={max_index} "
            f"angulo={angulo}°"
        )

    return (
        line_mic1,
        line_mic2,
        line_corr,
        line_ang,
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