"""
receiver_realtime.py
────────────────────
Servidor TCP que fica ouvindo na porta 5005.
A ESP32 conecta, envia START...END, desconecta e repete.
Os gráficos atualizam automaticamente a cada captura.

Execute ANTES de ligar a ESP32:
    python receiver_realtime.py
"""

import socket
import re
import threading
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

# ==========================
# CONFIG
# ==========================
HOST = "10.69.69.71"   # escuta em todas as interfaces
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
# PROCESSAMENTO
# ==========================
def reconstruir(arr):
    arr = np.array(arr, dtype=float)
    arr[arr == 0] = np.nan
    for i in range(1, len(arr) - 1):
        prev = arr[i-1] if not np.isnan(arr[i-1]) else arr[i]
        if not np.isnan(arr[i]) and abs(arr[i] - prev) > 150:
            arr[i] = np.nan
    x = np.arange(len(arr))
    mask = ~np.isnan(arr)
    if mask.sum() >= 2:
        arr = np.interp(x, x[mask], arr[mask])
    arr = np.convolve(arr, np.ones(7)/7, mode="same")
    m = np.max(np.abs(arr))
    return arr / m if m != 0 else arr

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

                m = re.search(r"= (-?\d+)", linha)
                if not m:
                    continue
                val = int(m.group(1))

                if   linha.startswith("mic1["):        mic1_tmp.append(val)
                elif linha.startswith("mic2["):        mic2_tmp.append(val)
                elif linha.startswith("correlacao["):  corr_tmp.append(val)
                elif linha.startswith("max_index"):    midx = val
                elif linha.startswith("max_val"):      mval = val
                elif linha.startswith("angulo_theta"): ang  = val

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

line_mic1, = ax_mic1.plot([], [], color="steelblue",      lw=1.5)
line_mic2, = ax_mic2.plot([], [], color="darkorange",     lw=1.5)
line_corr, = ax_corr.plot([], [], color="mediumseagreen", lw=1.5)
vline      = ax_corr.axvline(x=0, color="red", linestyle="--", lw=1.2, label="max_index")
line_ang,  = ax_ang.plot([], [], color="mediumpurple", lw=1.5, marker="o", markersize=3)

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
    ax.set_xlim(0, 1)
    ax.set_ylim(-1.2, 1.2)

ax_ang.set_ylabel("Angulo (graus)")
ax_ang.set_ylim(-10, 190)
ax_corr.legend(loc="upper right")

txt_angulo = ax_ang.text(
    0.05, 0.90, "Aguardando ESP32...",
    transform=ax_ang.transAxes,
    fontsize=14, fontweight="bold", color="mediumpurple"
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

    if len(mic1) >= 5:
        s1 = reconstruir(mic1)
        line_mic1.set_data(np.arange(len(s1)), s1)
        ax_mic1.set_xlim(0, len(s1) - 1)
        ax_mic1.set_ylim(-1.2, 1.2)

    if len(mic2) >= 5:
        s2 = reconstruir(mic2)
        line_mic2.set_data(np.arange(len(s2)), s2)
        ax_mic2.set_xlim(0, len(s2) - 1)
        ax_mic2.set_ylim(-1.2, 1.2)

    if len(corr) >= 5:
        c = reconstruir(corr)
        line_corr.set_data(np.arange(len(c)), c)
        ax_corr.set_xlim(0, len(c) - 1)
        ax_corr.set_ylim(-1.2, 1.2)
        if midx is not None:
            vline.set_xdata([midx, midx])

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