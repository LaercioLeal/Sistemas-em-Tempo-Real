"""
gerar_coef_fir.py
Projeta o filtro FIR passa-baixas do laboratório (seção 5 do README)
e grava os coeficientes em um arquivo .txt: versão float (referência)
e versão Q15 (inteira), que é a usada no ESP32.
"""

from scipy.signal import firwin
import numpy as np

# ---------------- Configuração do projeto ----------------
FS        = 16000        # Taxa de amostragem (Hz)
FC        = 4000         # Frequência de corte (Hz)
NUM_TAPS  = 10           # Número de coeficientes
JANELA    = "hamming"    # Método: janelamento (windowed-sinc)
Q         = 15           # Formato Q15 usado no ESP32
ARQUIVO   = "coeficientes_fir.txt"
# ----------------------------------------------------------

# Projeto do filtro
b = firwin(NUM_TAPS, FC, fs=FS, window=JANELA)

# Quantização Q15 (int16). firwin retorna |b| < 1, então cabe em int16.
bq = np.round(b * 2**Q).astype(int)

# Ajuste: a soma deve ser 2^Q (ganho DC = 1). Corrige o resíduo nos taps centrais.
resto = 2**Q - bq.sum()
meio = NUM_TAPS // 2
while resto != 0:
    passo = 1 if resto > 0 else -1
    bq[meio] += passo; resto -= passo
    if resto != 0 and NUM_TAPS % 2 == 0:       # mantém simetria (nº par de taps)
        bq[meio - 1] += passo; resto -= passo

assert bq.min() >= -32768 and bq.max() <= 32767
assert abs(bq).sum() * 32768 < 2**31, "risco de overflow no acumulador int32"

with open(ARQUIVO, "w") as f:
    f.write("// float\n{" + ", ".join(f"{c:.10f}" for c in b) + "}\n")
    f.write(f"// Q{Q} (int16)\n{{" + ", ".join(str(int(c)) for c in bq) + "}\n")

print(f"Arquivo gerado : {ARQUIVO}")
print("Q15:", bq.tolist(), "soma =", bq.sum())