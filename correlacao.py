import serial
import matplotlib.pyplot as plt
import re
import time
import numpy as np

# Porta serial (modifique conforme necessário)
porta = 'COM3'
baudrate = 9600

# Abre a porta serial
ser = serial.Serial(porta, baudrate, timeout=1)
time.sleep(2)  # Aguarda a inicialização da comunicação

correlacao = []

try:
    print("Lendo dados da porta serial...")
    while True:
        linha = ser.readline().decode('utf-8').strip()

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

    # Gera o gráfico suavizado
    plt.figure(figsize=(10, 5))
    plt.plot(range(len(correlacao_suavizada)), correlacao_suavizada, marker='o', markersize=4)
    plt.title('Correlação Cruzada (mic1 x mic2)')
    plt.xlabel('Deslocamento (k)')
    plt.ylabel('Valor da Correlação')
    plt.grid(True)
    plt.tight_layout()
    plt.show()

except KeyboardInterrupt:
    ser.close()
    print("Interrompido pelo usuário.")
