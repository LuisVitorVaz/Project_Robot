"""
receiver_realtime.py
────────────────────
Servidor TCP que recebe sinais do ESP32 e plota senoides suavizadas.
"""

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
# HOST = "10.241.8.71"
HOST = "0.0.0.0"
PORT = 5005

# ==========================
# ESTADO COMPARTILHADO
# ==========================
dados = {
    "mic1":       [],
    "mic2":       [],
    "correlacao": [],
    "max_index":  None,
    "max_val":    None,
    "angulo":     None,
    "novo":       False,
}
historico_angulo = deque(maxlen=100)
lock = threading.Lock()

# ==========================
# Reconstruir Sinal
# ==========================
def reconstruct_signal(sig, expected_len):
    """Resample to expected length using sinc interpolation"""
    if len(sig) == expected_len:
        return np.array(sig, dtype=float)
    # scipy resample uses FFT-based sinc — optimal for bandlimited signals
    return resample(np.array(sig, dtype=float), expected_len)

# ==========================
# FILTRO PASSA-BAIXA
# ==========================
def butter_lowpass(cutoff, fs, order=3):
    nyquist = 0.5 * fs
    normal_cutoff = cutoff / nyquist
    b, a = butter(order, normal_cutoff, btype='low', analog=False)
    return b, a

def lowpass_filter(data, cutoff, fs, order=3):
    if len(data) < 2 * order:
        return data
    b, a = butter_lowpass(cutoff, fs, order=order)
    y = lfilter(b, a, data)
    return y

# ==========================
# THREAD — servidor TCP
# ==========================
def servidor_tcp():
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((HOST, PORT))
    srv.listen(1)

    print(f"[receiver] Servidor TCP ouvindo em {HOST}:{PORT}")
    print(f"[receiver] Aguardando conexao da ESP32...\n")

    while True:
        try:
            conn, addr = srv.accept()
        except Exception as e:
            print(f"[receiver] Erro: {e}")
            break

        print(f"[receiver] ESP32 conectada: {addr}")

        buf = ""
        mic1_tmp, mic2_tmp, corr_tmp = [], [], []
        midx, mval, ang = None, None, None

        modo = None

        while True:
            try:
                chunk = conn.recv(4096).decode(errors="ignore")
            except Exception:
                break
            if not chunk:
                break

            buf += chunk

            while "\n" in buf:
                linha, buf = buf.split("\n", 1)
                linha = linha.strip()
                if not linha:
                    continue

                if linha == "MIC1:":
                    modo = "mic1"
                    continue
                elif linha == "MIC2:":
                    modo = "mic2"
                    continue
                elif linha == "CORR:":
                    modo = "corr"
                    continue

                if linha == "END":
                    with lock:
                        dados["mic1"]       = mic1_tmp[:]
                        dados["mic2"]       = mic2_tmp[:]
                        dados["correlacao"] = corr_tmp[:]
                        dados["max_index"]  = midx
                        dados["max_val"]    = mval
                        dados["angulo"]     = ang
                        dados["novo"]       = True

                        if ang is not None:
                            historico_angulo.append(ang)

                    print(f"[receiver] angulo={ang}  max_index={midx}  "
                          f"mic1={len(mic1_tmp)}pts  mic2={len(mic2_tmp)}pts")

                    buf = ""
                    break

                if re.fullmatch(r"-?\d+", linha):
                    val = int(linha)
                    if modo == "mic1":        mic1_tmp.append(val)
                    elif modo == "mic2":      mic2_tmp.append(val)
                    elif modo == "corr":      corr_tmp.append(val)
                    continue

                m = re.search(r"= (-?\d+)", linha)
                if not m:
                    continue

                val = int(m.group(1))
                if   linha.startswith("max_index"):    midx = val
                elif linha.startswith("max_val"):      mval = val
                elif linha.startswith("angulo_theta") or linha.startswith("angulo"):
                    ang = val

        conn.close()

thread = threading.Thread(target=servidor_tcp, daemon=True)
thread.start()

# ==========================
# PLOT EM TEMPO REAL
# ==========================
fig, axes = plt.subplots(2, 2, figsize=(14, 8))
fig.suptitle("ESP32 — Osciloscópio em Tempo Real", fontsize=13, fontweight="bold")

ax_mic1 = axes[0, 0]
ax_mic2 = axes[0, 1]
ax_corr = axes[1, 0]
ax_ang  = axes[1, 1]

line_mic1, = ax_mic1.plot([], [], lw=1.5)
line_mic2, = ax_mic2.plot([], [], lw=1.5)
line_corr, = ax_corr.plot([], [], lw=1.5)
vline      = ax_corr.axvline(x=0, linestyle="--", lw=1.2)
line_ang,  = ax_ang.plot([], [], lw=1.5, marker="o", markersize=3)

for ax, titulo, xlabel in [
    (ax_mic1, "mic1",                "Amostra"),
    (ax_mic2, "mic2",                "Amostra"),
    (ax_corr, "Correlacao cruzada",  "Lag"),
    (ax_ang,  "Historico do angulo", "Captura"),
]:
    ax.set_title(titulo)
    ax.set_xlabel(xlabel)
    ax.set_ylabel("Amplitude")
    ax.grid(True)

txt_angulo = ax_ang.text(
    0.05, 0.90, "Aguardando ESP32...",
    transform=ax_ang.transAxes,
    fontsize=14
)

def update(_):
    with lock:
        if not dados["novo"]:
            return
        dados["novo"] = False

        mic1  = dados["mic1"][:]
        mic2  = dados["mic2"][:]
        corr  = dados["correlacao"][:]
        midx  = dados["max_index"]
        ang   = dados["angulo"]
        hist  = list(historico_angulo)

    # === PLOT MIC1 (SENÓIDE FILTRADA) ===
    if len(mic1) >= 1:
        s1 = reconstruct_signal(mic1, 500)
        m = np.max(np.abs(s1))
        if m != 0:
            s1 = s1 / m
        if len(s1) > 100:
            s1 = lowpass_filter(s1, cutoff=2000, fs=80000, order=3)
        line_mic1.set_data(np.arange(len(s1)), s1)
        ax_mic1.set_xlim(0, len(s1) - 1)
        ax_mic1.set_ylim(-1.2, 1.2)

    # === PLOT MIC2 (SENÓIDE FILTRADA) ===
    if len(mic2) >= 1:
        s2 = reconstruct_signal(mic2, 600)
        m = np.max(np.abs(s2))
        if m != 0:
            s2 = s2 / m
        if len(s2) > 100:
            s2 = lowpass_filter(s2, cutoff=2000, fs=80000, order=3)
        line_mic2.set_data(np.arange(len(s2)), s2)
        ax_mic2.set_xlim(0, len(s2) - 1)
        ax_mic2.set_ylim(-1.2, 1.2)

    # === PLOT CORRELAÇÃO (NORMALIZADA) ===
    if len(corr) >= 5:
        c = np.array(corr, dtype=float)
        mc = np.max(np.abs(c))
        if mc != 0:
            c = c / mc
        line_corr.set_data(np.arange(len(c)), c)
        ax_corr.set_xlim(0, len(c) - 1)
        ax_corr.set_ylim(-1.2, 1.2)
        if midx is not None:
            vline.set_xdata([midx, midx])

    # === PLOT HISTÓRICO DE ÂNGULO ===
    if hist:
        x = list(range(len(hist)))
        line_ang.set_data(x, hist)
        ax_ang.set_xlim(0, max(len(hist) - 1, 1))
        ax_ang.set_ylim(max(0, min(hist) - 15), min(180, max(hist) + 15))

    if ang is not None:
        txt_angulo.set_text(f"Angulo atual: {ang} graus")

    fig.canvas.draw_idle()

ani = animation.FuncAnimation(
    fig, update,
    interval=200,
    blit=False,
    cache_frame_data=False,
)

plt.tight_layout()
plt.show()