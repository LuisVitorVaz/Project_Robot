import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# ==========================================================
# CONFIGURAÇÕES
# ==========================================================

N_AMOSTRAS = 256
CORR_TAMANHO = (2 * N_AMOSTRAS) - 1  # 511 quando N_AMOSTRAS = 256

# ----------------------------------------------------------
# PARÂMETROS FÍSICOS DO ARRANJO (usados na conversão lag -> ângulo
# e na checagem de plausibilidade física do resultado)
# ----------------------------------------------------------
D_MICROFONES = 0.10   # distância entre os dois microfones [m]
L_FONTE = 1.30         # distância da fonte sonora ao centro do arranjo [m]
VELOCIDADE_SOM = 343.0  # velocidade do som no ar [m/s] (~20°C)

# ARQUIVO = "sinal_0graus_20260709_100059.xlsx"
# ARQUIVO = "sinal_45graus_20260709_100059.xlsx"
ARQUIVO = "sinal_90graus_20260709_100059.xlsx"
# ARQUIVO = "sinal_135graus_20260709_100059.xlsx"
# ARQUIVO = "sinal_180graus_20260709_100059.xlsx"

# DADOS DOS MIC COM A CAIXA DE SOM
# ARQUIVO = "sinal_0graus_20260709_122645.xlsx"
# ARQUIVO = "sinal_45graus_20260709_122645.xlsx"
# ESSE E O NOVO 45 GRAUS
# ARQUIVO = "sinal_0graus_20260709_135036.xlsx"  
# ARQUIVO = "sinal_0graus_20260709_135613.xlsx"
# MELHOR RESPOSTA A 45 GRAUS
# ARQUIVO = "sinal_0graus_20260709_140622.xlsx"    
# melhor arquivo a 45 graus
# ARQUIVO = "sinal_0graus_20260709_152458.xlsx"   
# ARQUIVO = "sinal_0graus_20260709_153206.xlsx"
# ARQUIVO = "sinal_0graus_20260709_154041.xlsx"

# ARQUIVO = "sinal_90graus_20260709_140622.xlsx"
# ARQUIVO = "sinal_135graus_20260709_122645.xlsx"  

# ARQUIVO = "sinal_45graus_20260709_135613.xlsx"

# ARQUIVO = "sinal_90graus_20260709_122645.xlsx"
# ARQUIVO = "sinal_135graus_20260709_122645.xlsx"
# ARQUIVO = "sinal_180graus_20260709_122645.xlsx"


# ARQUIVO = "senoides_-1_a_1_defasagem_100.xlsx"
# ARQUIVO = "senoides_-1_a_1_defasagem_50.xlsx"
# ARQUIVO = "senoides_-1_a_1_defasagem_50 (1).xlsx"
# ARQUIVO = "senoides_-1_a_1_defasagem_30.xlsx"
# ARQUIVO = "senoides_-1_a_1_defasagem_12.xlsx"
ARQUIVO = "sinal nao defasado.xlsx"


# ARQUIVOS SEM ORELHA
# angulo 45 grau :ângulo 0° salvo em: sinal_0graus_20260709_154603.xlsx
# angulo de 0 graus :bloco do ângulo 0° salvo em: sinal_0graus_20260709_154941.xlsx
# angulo de 90 graus : Melhor bloco do ângulo 45° salvo em: sinal_45graus_20260709_154941.xlsx (pico de correlação = 5.3532)
# angulo de 135 graus :Melhor bloco do ângulo 90° salvo em: sinal_90graus_20260709_154941.xlsx (pico de correlação = 7.4064)
# angulo de 180 graus :Melhor bloco do ângulo 135° salvo em: sinal_135graus_20260709_154941.xlsx (pico de correlação = 5.6523)

# zero graus
# ARQUIVO = "sinal_0graus_20260709_154941.xlsx"  
# 45 graus
# ARQUIVO = "sinal_0graus_20260709_154603.xlsx"
# 90 graus
# ARQUIVO = "sinal_45graus_20260709_154941.xlsx"
# 135 graus
# ARQUIVO = "sinal_90graus_20260709_154941.xlsx"
# 180 graus
# ARQUIVO = "sinal_135graus_20260709_154941.xlsx"


# ==========================================================
# FUNÇÕES
# ==========================================================

def normalizar_sinal(sinal):
    sinal = ((sinal - np.min(sinal)) / (np.max(sinal) - np.min(sinal)))
    # maior = np.max(np.abs(sinal))
    # if maior == 0:
    #     return sinal  # evita divisão por zero
    # return sinal / maior
    return sinal

def correlacao_cruzada(Mic1 ,Mic2):
    correlacao = [0.0] * CORR_TAMANHO

    for l in range(CORR_TAMANHO):
        soma = 0.0

        if l < N_AMOSTRAS:
            j = l
            k = N_AMOSTRAS - 1
        else:
            j = N_AMOSTRAS - 1
            k = (CORR_TAMANHO - 1) - l

        while j >= 0 and k >= 0:
            # Mesma ordem do np.correlate(Mic1, Mic2, ...): Mic1[j] * Mic2[k]
            soma += Mic1[j] * Mic2[k]
            j -= 1
            k -= 1

        correlacao[l] = soma

    return correlacao


def encontrar_maior(corr):
    maior = corr[0]
    posicao = 0  # <-- inicializado ANTES do loop (bug corrigido)

    for i in range(1, len(corr)):
        if corr[i] > maior:
            maior = corr[i]
            posicao = i

    return posicao


# ==========================================================
# NOVA FUNÇÃO: LAG -> ÂNGULO + CHECAGEM DE PLAUSIBILIDADE FÍSICA
#
# Modelo far-field (Fig. 1 e Eq. 1 do artigo): como L_FONTE >> D_MICROFONES
# (razão 13:1 no seu experimento), a diferença de caminho entre os dois
# microfones pode ser aproximada por:
#
#       ΔL = d * sin(theta)
#
# e como ΔL = c * (lag / fs), isolando theta:
#
#       theta = arcsin( c * lag / (fs * d) )
#
# Além disso, o maior atraso fisicamente possível ocorre com a fonte a
# 90° (na lateral do arranjo, ΔL = d):
#
#       lag_max = d * fs / c
#
# Qualquer |lag| medido acima desse valor é impossível para essa
# geometria e indica erro de estimação (ruído, reflexão/reverberação,
# pico espúrio), não um atraso real da onda direta.
# ==========================================================

def lag_max_amostras(d=D_MICROFONES, fs=16000, c=VELOCIDADE_SOM):
    """Maior lag (em amostras) fisicamente possível para a geometria dada."""
    return d * fs / c


def lag_para_angulo(lag_amostras, d=D_MICROFONES, fs=16000, c=VELOCIDADE_SOM):
    """
    Converte um lag (em amostras) no ângulo estimado da fonte (graus),
    usando o modelo far-field. Também retorna se o lag é fisicamente
    plausível para a geometria do arranjo.

    Retorna
    -------
    angulo_graus : float ou None (None se o lag for fisicamente impossível)
    plausivel    : bool
    lag_max      : float (limite teórico em amostras, para referência)
    """
    lag_max = lag_max_amostras(d, fs, c)
    razao = (c * lag_amostras) / (fs * d)

    plausivel = abs(razao) <= 1.0  # arcsin só é definido em [-1, 1]

    if not plausivel:
        return None, False, lag_max

    angulo_graus = np.degrees(np.arcsin(razao))
    return angulo_graus, True, lag_max


# ==========================================================
# NOVA FUNÇÃO: GCC-PHAT
# (Generalized Cross-Correlation with Phase Transform)
#
# Técnica descrita no artigo (Seção 3.3, Eqs. 7-11): o SRP-PHAT usado
# no artigo é a versão em banda larga / múltiplas direções desta mesma
# ideia. Para o caso de apenas 2 microfones e o objetivo de achar o lag
# de maior correlação (equivalente ao que "correlacao_cruzada" já faz),
# isso se reduz ao GCC-PHAT clássico:
#
#   1) X1(f) = FFT(Mic1),  X2(f) = FFT(Mic2)
#   2) Espectro cruzado:      G12(f) = X1(f) * conj(X2(f))
#   3) Pesagem PHAT (Eq. 9/10 do artigo: G_m,b(k) = 1 / |F_m,b(k)|):
#         G12_phat(f) = G12(f) / |G12(f)|
#      -> mantém só a informação de FASE, remove informação de amplitude.
#   4) GCC-PHAT(l) = IFFT(G12_phat(f))   (Eq. 11 do artigo)
#   5) O lag que maximiza GCC-PHAT é a estimativa do atraso (TDOA)
#      entre os dois microfones.
#
# Essa função NÃO altera nada da implementação original; é apenas
# adicionada ao lado dela para fins de comparação.
# ==========================================================

def gcc_phat(Mic1, Mic2, fs=None):
    """
    Calcula o GCC-PHAT entre Mic1 e Mic2.

    Parâmetros
    ----------
    Mic1, Mic2 : arrays 1D de mesmo tamanho N_AMOSTRAS
    fs : (opcional) taxa de amostragem em Hz. Se fornecida, também
         retorna o atraso estimado em segundos.

    Retorna
    -------
    gcc      : array real com a função GCC-PHAT, já reordenada de forma
               que o índice central corresponde ao lag 0 (mesma convenção
               de "lags" usada no script original: -(N-1) ... (N-1)).
    lag_amostras : lag (em número de amostras) que maximiza o GCC-PHAT.
    atraso_seg   : lag_amostras / fs (None se fs não for informado).
    indice_pico  : posição (índice) do valor máximo dentro do vetor `gcc`,
                   equivalente ao "Índice do corr" que já é impresso para
                   a correlação cruzada normal.
    """
    n = len(Mic1)

    # Tamanho de FFT com zero-padding para N1 + N2 - 1 amostras,
    # arredondado para a próxima potência de 2 (mais eficiente e evita
    # aliasing circular na correlação).
    tamanho_fft = 1
    while tamanho_fft < (2 * n - 1):
        tamanho_fft *= 2

    X1 = np.fft.rfft(Mic1, n=tamanho_fft)
    X2 = np.fft.rfft(Mic2, n=tamanho_fft)

    # Espectro cruzado
    G12 = X1 * np.conj(X2)

    # Pesagem PHAT: normaliza pelo módulo, mantendo só a fase
    denom = np.abs(G12)
    denom[denom == 0] = 1e-12  # evita divisão por zero
    G12_phat = G12 / denom

    # Volta ao domínio do tempo
    gcc_bruto = np.fft.irfft(G12_phat, n=tamanho_fft)

    # np.fft.irfft entrega o resultado na ordem "circular":
    # [lag 0, lag 1, ..., lag (N-1), lag -(N-N), ..., lag -1]
    # Reordena para: [-(n-1), ..., -1, 0, 1, ..., (n-1)]
    max_lag = n - 1
    gcc = np.concatenate((gcc_bruto[-max_lag:], gcc_bruto[:max_lag + 1]))

    lags_gcc = np.arange(-max_lag, max_lag + 1)

    indice_pico = np.argmax(gcc)
    lag_amostras = lags_gcc[indice_pico]

    atraso_seg = (lag_amostras / fs) if fs is not None else None

    return gcc, lags_gcc, lag_amostras, atraso_seg, indice_pico


# ==========================================================
# LÊ O EXCEL
# ==========================================================

dados = pd.read_excel(ARQUIVO)

Mic1 = dados["Mic1"].to_numpy(dtype=np.float64)
Mic2 = dados["Mic2"].to_numpy(dtype=np.float64)

print("Número de amostras:", len(Mic1))

Mic1 = Mic1[:N_AMOSTRAS]
Mic2 = Mic2[:N_AMOSTRAS]

# Normalização para -1 até 1 (agora Mic1/Mic2 já existem e a função já foi definida)
Mic1_norm = normalizar_sinal(Mic1)
Mic2_norm = normalizar_sinal(Mic2)

# --------- Sinais ---------
plt.figure(figsize=(12, 4))
plt.plot(Mic1_norm, label="Mic1_norm")
plt.plot(Mic2_norm, label="Mic2_norm")
plt.title("sinais normalizados")
plt.xlabel("Amostra")
plt.ylabel("Amplitude")
plt.grid(True)
plt.legend()

# --------- Correlação (implementação manual x numpy) ---------
plt.figure(figsize=(14, 5))

plt.plot(linewidth=2)



# ==========================================================
# CORRELAÇÃO CRUZADA (IMPLEMENTAÇÃO MANUAL)
# ==========================================================

corr = correlacao_cruzada(Mic1_norm,Mic2_norm)

maior_valor = encontrar_maior(corr)

indice = np.argmax(corr)
valor = corr[indice]

lags = np.arange(-(N_AMOSTRAS - 1), N_AMOSTRAS)
lag = lags[indice]

print("\n==============================")
print("RESULTADO DA CORRELAÇÃO")
print("==============================")
print(f"Índice do corr : {indice}")
print(f"Defasagem (lag) : {lag} amostras")
print(f"Posição do maior valor : {maior_valor}")

angulo_corr, plausivel_corr, lag_max_teorico = lag_para_angulo(lag, fs=16000)
print(f"Lag máximo fisicamente possível (d={D_MICROFONES} m) : ±{lag_max_teorico:.2f} amostras")
if plausivel_corr:
    print(f"Ângulo estimado (far-field) : {angulo_corr:.2f} graus")
else:
    print("AVISO: lag fora do limite físico do arranjo -> resultado provavelmente "
          "é ruído/reflexão, não o caminho direto da fonte.")
print("==============================")


# ==========================================================
# CORRELAÇÃO CRUZADA PYTHON (NUMPY) - PARA COMPARAÇÃO
# ==========================================================

corr_python = np.correlate(Mic1_norm, Mic2_norm, mode='full')

indice_python = np.argmax(corr_python)
valor_python = corr_python[indice_python]
lag_python = lags[indice_python]

print("\n==============================")
print("RESULTADO DA CORRELAÇÃO PYTHON")
print("==============================")
print(f"Índice do corr : {indice_python}")
print(f"Defasagem (lag) : {lag_python} amostras")
print("==============================")


# ==========================================================
# GCC-PHAT - NOVO MÉTODO (PARA COMPARAÇÃO)
# ==========================================================

FS = 16000  # taxa de amostragem usada no artigo (Seção 5.1). Ajuste se necessário.

gcc, lags_gcc, lag_gcc, atraso_gcc_seg, indice_pico_gcc = gcc_phat(Mic1, Mic2, fs=FS)
valor_gcc = gcc[np.argmax(gcc)]

print("\n==============================")
print("RESULTADO DO GCC-PHAT")
print("==============================")
print(f"Índice do vetor gcc : {indice_pico_gcc}  (de 0 a {len(gcc) - 1}, centro = {len(gcc) // 2})")
print(f"Defasagem (lag) : {lag_gcc} amostras")
if atraso_gcc_seg is not None:
    print(f"Atraso estimado : {atraso_gcc_seg * 1000:.4f} ms (fs = {FS} Hz)")
print(f"Valor do pico   : {valor_gcc:.4f}")

angulo_gcc, plausivel_gcc, _ = lag_para_angulo(lag_gcc, fs=FS)
print(f"Lag máximo fisicamente possível (d={D_MICROFONES} m) : ±{lag_max_teorico:.2f} amostras")
if plausivel_gcc:
    print(f"Ângulo estimado (far-field) : {angulo_gcc:.2f} graus")
else:
    print("AVISO: lag fora do limite físico do arranjo -> resultado provavelmente "
          "é ruído/reflexão, não o caminho direto da fonte.")

LIMIAR_CONFIANCA_PICO = 0.3
if valor_gcc < LIMIAR_CONFIANCA_PICO:
    print(f"AVISO: pico do GCC-PHAT baixo ({valor_gcc:.4f} < {LIMIAR_CONFIANCA_PICO}) "
          "-> resultado pouco confiável (sinal fraco/ruidoso ou pico ambíguo).")
print("==============================")


# ==========================================================
# PLOTS
# ==========================================================

# --------- Sinais ---------
plt.figure(figsize=(12, 4))
plt.plot(Mic1, label="Mic1")
plt.plot(Mic2, label="Mic2")
plt.title("Sinais lidos do Excel")
plt.xlabel("Amostra")
plt.ylabel("Amplitude")
plt.grid(True)
plt.legend()

# --------- Correlação (implementação manual x numpy) ---------
plt.figure(figsize=(14, 5))

plt.plot(lags, corr, linewidth=2, label="Correlação implementada")

plt.plot(
    lags,
    corr_python,
    linewidth=2,
    linestyle="--",
    label="Correlação Python (numpy.correlate)"
)


# Marca o pico encontrado (implementação manual)
plt.scatter(lag, valor, color='red', s=80, zorder=5)
plt.annotate(
    f"Lag = {lag}\nValor = {valor:.2f}",
    xy=(lag, valor),
    xytext=(lag + 15, valor),
    arrowprops=dict(arrowstyle="->", color="red"),
    fontsize=10
)

# Marca pico da correlação Python
plt.scatter(lag_python, valor_python, color='green', s=80, zorder=5)
plt.annotate(
    f"Lag Python = {lag_python}\nValor = {valor_python:.2f}",
    xy=(lag_python, valor_python),
    xytext=(lag_python + 15, valor_python),
    arrowprops=dict(arrowstyle="->", color="green"),
    fontsize=10
)

plt.title("Resultado da Correlação Cruzada")
plt.xlabel("Defasagem (Lag) [amostras]")
plt.ylabel("Correlação")
plt.grid(True)
plt.legend()

plt.xlim(-(N_AMOSTRAS - 1), N_AMOSTRAS - 1)

# --------- NOVO: GCC-PHAT sozinho ---------
plt.figure(figsize=(14, 5))

plt.plot(lags_gcc, gcc, linewidth=2, color="purple", label="GCC-PHAT")

plt.scatter(lag_gcc, valor_gcc, color='red', s=80, zorder=5)
plt.annotate(
    f"Lag GCC-PHAT = {lag_gcc}\nValor = {valor_gcc:.4f}",
    xy=(lag_gcc, valor_gcc),
    xytext=(lag_gcc + 15, valor_gcc),
    arrowprops=dict(arrowstyle="->", color="red"),
    fontsize=10
)

plt.title("Resultado do GCC-PHAT (Generalized Cross-Correlation - Phase Transform)")
plt.xlabel("Defasagem (Lag) [amostras]")
plt.ylabel("GCC-PHAT")
plt.grid(True)
plt.legend()

# --------- NOVO: Comparação direta Correlação Cruzada x GCC-PHAT ---------
# Normaliza cada curva pelo próprio máximo absoluto, só para poder
# comparar visualmente o formato/posição dos picos lado a lado
# (as escalas dos dois métodos são naturalmente diferentes).
corr_norm = np.array(corr) / np.max(np.abs(corr))
gcc_norm = gcc / np.max(np.abs(gcc))

plt.figure(figsize=(14, 5))

plt.plot(lags, corr_norm, linewidth=2, label="Correlação cruzada (normalizada)")
plt.plot(lags_gcc, gcc_norm, linewidth=2, linestyle="--", color="purple",
         label="GCC-PHAT (normalizado)")

plt.axvline(lag, color="blue", linestyle=":", alpha=0.7,
            label=f"Pico correlação = {lag}")
plt.axvline(lag_gcc, color="purple", linestyle=":", alpha=0.7,
            label=f"Pico GCC-PHAT = {lag_gcc}")

plt.title("Comparação: Correlação Cruzada x GCC-PHAT")
plt.xlabel("Defasagem (Lag) [amostras]")
plt.ylabel("Amplitude normalizada")
plt.grid(True)
plt.legend()
plt.xlim(-(N_AMOSTRAS - 1), N_AMOSTRAS - 1)

plt.show()