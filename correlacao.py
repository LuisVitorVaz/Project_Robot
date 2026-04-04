import serial
import matplotlib.pyplot as plt
import re
import time
import numpy as np

# Porta serial (modifique conforme necessário)
porta = 'COM5'
baudrate = 115200

# Abre a porta serial
ser = serial.Serial(porta, baudrate, timeout=1)
time.sleep(2)  # Aguarda a inicialização da comunicação

# 🔧 LIMPA BUFFER (evita começar no meio do lixo)
ser.reset_input_buffer()

correlacao = []
sinal1 = []
sinal2 = []

def capturar_sinais(ser, sinal1, sinal2, n_amostras=81):
    """
    Captura os dois sinais brutos do Arduino.
    Espera linhas no formato:
        mic1[i] = valor
        mic2[i] = valor
    """
    print("Capturando sinais brutos...")
    while len(sinal1) < n_amostras or len(sinal2) < n_amostras:
        try:
            linha = ser.readline().decode('utf-8', errors='ignore').strip()
        except:
            continue

        # 🔧 ignora linhas vazias ou lixo
        if not linha:
            continue

        # 🔧 ajustado para mic1
        if linha.startswith("mic1["):
            match = re.search(r"mic1\[\d+\] = ([\d\.\-]+)", linha)
            if match:
                sinal1.append(float(match.group(1)))

        # 🔧 ajustado para mic2
        elif linha.startswith("mic2["):
            match = re.search(r"mic2\[\d+\] = ([\d\.\-]+)", linha)
            if match:
                sinal2.append(float(match.group(1)))

        if len(sinal1) >= n_amostras and len(sinal2) >= n_amostras:
            break

def plotar_sinais(sinal1, sinal2):
    """
    Plota os dois sinais capturados em subplots para comparação visual.
    """
    fig, axs = plt.subplots(3, 1, figsize=(12, 9))

    # Sinal 1
    axs[0].plot(range(len(sinal1)), sinal1, marker='o', markersize=3, label='Sinal 1 (mic1)')
    axs[0].set_title('Sinal 1 - Microfone 1')
    axs[0].set_xlabel('Amostras')
    axs[0].set_ylabel('Amplitude')
    axs[0].legend()
    axs[0].grid(True)

    # Sinal 2
    axs[1].plot(range(len(sinal2)), sinal2, marker='o', markersize=3, label='Sinal 2 (mic2)')
    axs[1].set_title('Sinal 2 - Microfone 2')
    axs[1].set_xlabel('Amostras')
    axs[1].set_ylabel('Amplitude')
    axs[1].legend()
    axs[1].grid(True)

    # Sobreposição
    axs[2].plot(range(len(sinal1)), sinal1, marker='o', markersize=3, label='Sinal 1 (mic1)')
    axs[2].plot(range(len(sinal2)), sinal2, linestyle='--', marker='o', markersize=3, label='Sinal 2 (mic2)')
    axs[2].set_title('Sobreposição: Sinal 1 x Sinal 2 (comparação de fase)')
    axs[2].set_xlabel('Amostras')
    axs[2].set_ylabel('Amplitude')
    axs[2].legend()
    axs[2].grid(True)

    plt.tight_layout()
    plt.show()

try:
    print("Lendo dados da porta serial...")

    # Captura os sinais brutos primeiro
    capturar_sinais(ser, sinal1, sinal2, n_amostras=81)

    # Captura a correlação cruzada
    print("Capturando correlação cruzada...")
    while True:
        try:
            linha = ser.readline().decode('utf-8', errors='ignore').strip()
        except:
            continue

        if not linha:
            continue

        if linha.startswith("correlacao["):
            match = re.search(r"correlacao\[\d+\] = ([\d\.\-]+)", linha)
            if match:
                valor = float(match.group(1))
                correlacao.append(valor)

        if len(correlacao) >= 81:
            break

    ser.close()

    # Suavização: média móvel com janela de 5
    def media_movel(dados, janela=5):
        return np.convolve(dados, np.ones(janela)/janela, mode='same')

    correlacao_suavizada = media_movel(correlacao, janela=5)

    # Plota os sinais brutos
    plotar_sinais(sinal1, sinal2)

    # Gráfico da correlação
    plt.figure(figsize=(10, 5))
    plt.plot(range(len(correlacao_suavizada)), correlacao_suavizada, marker='o', markersize=4)
    plt.title('Correlação Cruzada (mic1 x mic2)')
    plt.xlabel('Deslocamento (k)')
    plt.ylabel('Valor da Correlação')
    plt.grid(True)
    plt.tight_layout()
    plt.show()

    # 🔵 NOVO GRÁFICO: correlação cruzada original (sem suavização)
    plt.figure(figsize=(10, 5))
    plt.plot(range(len(correlacao)), correlacao, marker='o', markersize=4)
    plt.title('Correlação Cruzada ORIGINAL (sem suavização)')
    plt.xlabel('Deslocamento (k)')
    plt.ylabel('Valor da Correlação')
    plt.grid(True)
    plt.tight_layout()
    plt.show()
    
except KeyboardInterrupt:
    ser.close()
    print("Interrompido pelo usuário.")