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

# ==========================
# ARQUIVO DE LOG
# ==========================
arquivo = open("dados_serial.txt", "w", encoding="utf-8")

sinal1 = []
sinal2 = []
correlacao = []

print("Capturando dados...")

inicio = time.time()
timeout = 10

# ==========================
# CAPTURA
# ==========================
while True:

    if time.time() - inicio > timeout:
        print("⚠️ Timeout geral")
        arquivo.write("⚠️ Timeout geral\n")
        break

    try:
        linha = ser.readline().decode(errors='ignore').strip()
    except:
        continue

    if not linha:
        continue

    print(linha)
    arquivo.write(linha + "\n")

    if linha == "END":
        print("Fim da transmissão")
        break

    if linha.startswith("mic1["):
        m = re.search(r"= (-?\d+)", linha)
        if m:
            sinal1.append(int(m.group(1)))

    elif linha.startswith("mic2["):
        m = re.search(r"= (-?\d+)", linha)
        if m:
            sinal2.append(int(m.group(1)))

    elif linha.startswith("correlacao["):
        m = re.search(r"= (-?\d+)", linha)
        if m:
            correlacao.append(int(m.group(1)))

ser.close()

# ==========================
# RESUMO
# ==========================
print("\nResumo da captura:")
print(f"mic1: {len(sinal1)}")
print(f"mic2: {len(sinal2)}")
print(f"correlacao: {len(correlacao)}")

arquivo.write("\n===== RESUMO =====\n")
arquivo.write(f"mic1: {len(sinal1)}\n")
arquivo.write(f"mic2: {len(sinal2)}\n")
arquivo.write(f"correlacao: {len(correlacao)}\n")

# ==========================
# SALVAR ORGANIZADO
# ==========================
arquivo.write("\n--- mic1 ---\n")
for i, v in enumerate(sinal1):
    arquivo.write(f"{i}: {v}\n")

arquivo.write("\n--- mic2 ---\n")
for i, v in enumerate(sinal2):
    arquivo.write(f"{i}: {v}\n")

arquivo.write("\n--- correlacao ---\n")
for i, v in enumerate(correlacao):
    arquivo.write(f"{i}: {v}\n")

arquivo.close()

print("✅ Dados salvos em dados_serial.txt")

# ==========================
# VALIDAÇÃO
# ==========================
if len(sinal1) == 0 or len(sinal2) == 0:
    print("❌ Erro: sinais não capturados")
    exit()

# ==========================
# FUNÇÃO: INTERPOLAR ZEROS
# ==========================
def interpolar_zeros(dados):
    dados = np.array(dados, dtype=float)

    for i in range(1, len(dados) - 1):
        if dados[i] == 0:
            esquerda = dados[i - 1]
            direita = dados[i + 1]

            if esquerda != 0 and direita != 0:
                dados[i] = (esquerda + direita) / 2

    return dados

# ==========================
# SUAVIZAÇÃO
# ==========================
def media_movel(dados, janela=5):
    if len(dados) < janela:
        return dados
    return np.convolve(dados, np.ones(janela)/janela, mode='same')

# ==========================
# NORMALIZAÇÃO
# ==========================
def normalizar(dados):
    dados = np.array(dados)
    if np.max(np.abs(dados)) == 0:
        return dados
    return dados / np.max(np.abs(dados))

# ==========================
# PROCESSAMENTO
# ==========================
sinal1_n = interpolar_zeros(normalizar(sinal1))
sinal2_n = interpolar_zeros(normalizar(sinal2))

correlacao_interp = interpolar_zeros(correlacao)
correlacao_suavizada = media_movel(correlacao_interp)

# ==========================
# PLOTS
# ==========================
plt.figure(figsize=(12, 8))

plt.subplot(3, 1, 1)
plt.plot(sinal1_n)
plt.title("Sinal mic1")
plt.grid()

plt.subplot(3, 1, 2)
plt.plot(sinal2_n)
plt.title("Sinal mic2")
plt.grid()

plt.subplot(3, 1, 3)
min_len = min(len(sinal1_n), len(sinal2_n))
plt.plot(sinal1_n[:min_len], label="mic1")
plt.plot(sinal2_n[:min_len], linestyle='--', label="mic2")
plt.title("Sobreposição")
plt.legend()
plt.grid()

plt.tight_layout()
plt.show()

# ==========================
# CORRELAÇÃO
# ==========================
if correlacao:
    plt.figure(figsize=(10, 4))
    plt.plot(correlacao_interp, label="Original (corrigido)")
    plt.plot(correlacao_suavizada, linestyle='--', label="Suavizada")
    plt.title("Correlação Cruzada")
    plt.legend()
    plt.grid()
    plt.show()
else:
    print("⚠️ Correlação não capturada")