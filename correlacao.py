import serial
import matplotlib.pyplot as plt
import re
import time
import numpy as np

# ==========================
# CONFIG SERIAL
# ==========================
porta = 'COM5'
baudrate = 115200

ser = serial.Serial(porta, baudrate, timeout=1)
time.sleep(2)

ser.reset_input_buffer()

sinal1 = []
sinal2 = []
correlacao = []

# buffers brutos (bytes)
sinal1_raw = []
sinal2_raw = []

# ==========================
# ESPERA INÍCIO
# ==========================
print("Aguardando START...")

while True:
    linha = ser.readline().decode(errors='ignore').strip()
    if linha == "START":
        break

print("Capturando dados...")

inicio = time.time()
timeout = 10

# ==========================
# CAPTURA
# ==========================
while True:

    if time.time() - inicio > timeout:
        print("⚠️ Timeout geral")
        break

    try:
        linha = ser.readline().decode(errors='ignore').strip()
    except:
        continue

    if not linha:
        continue

    # DEBUG (mantém seus prints visíveis)
    print(linha)

    if linha == "END":
        print("Fim da transmissão")
        break

    # mic1
    if linha.startswith("mic1["):
        m = re.search(r"= (\d+)", linha)
        if m:
            sinal1_raw.append(int(m.group(1)))

    # mic2
    elif linha.startswith("mic2["):
        m = re.search(r"= (\d+)", linha)
        if m:
            sinal2_raw.append(int(m.group(1)))

    # correlação
    elif linha.startswith("correlacao["):
        m = re.search(r"= (\d+)", linha)
        if m:
            correlacao.append(int(m.group(1)))

ser.close()

# ==========================
# RECONSTRUÇÃO 16 BITS
# ==========================
def reconstruir_uint16(lista):
    resultado = []
    for i in range(0, len(lista) - 1, 2):
        valor = (lista[i] << 8) | lista[i+1]
        resultado.append(valor)
    return resultado

sinal1 = reconstruir_uint16(sinal1_raw)
sinal2 = reconstruir_uint16(sinal2_raw)

print("\nResumo da captura:")
print(f"mic1 (raw): {len(sinal1_raw)}")
print(f"mic1 (reconstruído): {len(sinal1)}")
print(f"mic2 (raw): {len(sinal2_raw)}")
print(f"mic2 (reconstruído): {len(sinal2)}")
print(f"correlacao: {len(correlacao)}")

# ==========================
# VALIDAÇÃO
# ==========================
if len(sinal1) == 0 or len(sinal2) == 0:
    print("❌ Erro: sinais não capturados")
    exit()

# ==========================
# SUAVIZAÇÃO
# ==========================
def media_movel(dados, janela=5):
    if len(dados) < janela:
        return dados
    return np.convolve(dados, np.ones(janela)/janela, mode='same')

correlacao_suavizada = media_movel(correlacao)

# ==========================
# PLOTS
# ==========================
plt.figure(figsize=(12, 8))

plt.subplot(3, 1, 1)
plt.plot(sinal1)
plt.title("Sinal mic1")
plt.grid()

plt.subplot(3, 1, 2)
plt.plot(sinal2)
plt.title("Sinal mic2")
plt.grid()

plt.subplot(3, 1, 3)
min_len = min(len(sinal1), len(sinal2))
plt.plot(sinal1[:min_len], label="mic1")
plt.plot(sinal2[:min_len], linestyle='--', label="mic2")
plt.title("Sobreposição")
plt.legend()
plt.grid()

plt.tight_layout()
plt.show()

# ==========================
# CORRELAÇÃO
# ==========================
if correlacao:
    plt.figure()
    plt.plot(correlacao, label="Original")
    plt.plot(correlacao_suavizada, linestyle='--', label="Suavizada")
    plt.title("Correlação Cruzada")
    plt.legend()
    plt.grid()
    plt.show()
else:
    print("⚠️ Correlação não capturada")