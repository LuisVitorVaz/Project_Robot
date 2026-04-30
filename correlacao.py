import socket
import re
import threading
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
from scipy.signal import butter, lfilter
from scipy.signal import resample

# ==========================
# CONFIG
# ==========================
HOST = "0.0.0.0"   # 🔥 CORREÇÃO
PORT = 5005

# ==========================
# ESTADO COMPARTILHADO
# ==========================
dados = {
    "mic1": [],
    "mic2": [],
    "correlacao": [],
    "max_index": None,
    "max_val": None,
    "angulo": None,
    "novo": False,
}
historico_angulo = deque(maxlen=100)
lock = threading.Lock()

# ==========================
# THREAD — servidor TCP
# ==========================
def servidor_tcp():
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((HOST, PORT))
    srv.listen(1)

    print(f"[receiver] Servidor TCP ouvindo em {HOST}:{PORT}")

    while True:
        conn, addr = srv.accept()
        print(f"[receiver] ESP32 conectada: {addr}")

        buf = ""
        mic1_tmp, mic2_tmp, corr_tmp = [], [], []

        modo = None

        while True:
            chunk = conn.recv(4096)
            if not chunk:
                break

            buf += chunk.decode(errors="ignore")

            while "\n" in buf:
                linha, buf = buf.split("\n", 1)
                linha = linha.strip()

                if not linha:
                    continue

                if linha == "MIC1:":
                    modo = "mic1"
                    mic1_tmp = []  # 🔥 garante reset
                    continue

                elif linha == "MIC2:":
                    modo = "mic2"
                    mic2_tmp = []
                    continue

                elif linha == "END":
                    with lock:
                        dados["mic1"] = mic1_tmp[:]
                        dados["mic2"] = mic2_tmp[:]
                        dados["novo"] = True

                    print(f"[RX] mic1={len(mic1_tmp)} mic2={len(mic2_tmp)}")
                    break

                # 🔥 apenas números (RAW ADC)
                if linha.isdigit():
                    val = int(linha)

                    if modo == "mic1":
                        mic1_tmp.append(val)

                    elif modo == "mic2":
                        mic2_tmp.append(val)

        conn.close()
        print("[receiver] desconectado")

thread = threading.Thread(target=servidor_tcp, daemon=True)
thread.start()

# ==========================
# PLOT EM TEMPO REAL
# ==========================
fig, axes = plt.subplots(2, 2, figsize=(14, 8))

ax_mic1 = axes[0, 0]
ax_mic2 = axes[0, 1]
ax_corr = axes[1, 0]
ax_ang  = axes[1, 1]

line_mic1, = ax_mic1.plot([], [], lw=1.5)
line_mic2, = ax_mic2.plot([], [], lw=1.5)

for ax, titulo in [
    (ax_mic1, "mic1 RAW"),
    (ax_mic2, "mic2 RAW"),
]:
    ax.set_title(titulo)
    ax.set_xlabel("Amostra")
    ax.set_ylabel("ADC (0-4095)")
    ax.grid(True)
    ax.set_ylim(0, 4200)

def update(_):
    with lock:
        if not dados["novo"]:
            return
        dados["novo"] = False

        mic1 = dados["mic1"][:]
        mic2 = dados["mic2"][:]

    if len(mic1) > 0:
        line_mic1.set_data(np.arange(len(mic1)), mic1)
        ax_mic1.set_xlim(0, len(mic1))

    if len(mic2) > 0:
        line_mic2.set_data(np.arange(len(mic2)), mic2)
        ax_mic2.set_xlim(0, len(mic2))

    fig.canvas.draw_idle()

ani = animation.FuncAnimation(
    fig, update,
    interval=100,
    blit=False,
    cache_frame_data=False,
)

plt.tight_layout()
plt.show()