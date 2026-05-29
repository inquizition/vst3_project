import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import firls, freqz

fs = 44100
nyq = fs / 2
numtaps = 153

bands = np.array([0, 50,
                  50, 80,
                  80, 150,
                  150, 1000,
                  1000, 2000,
                  2000, 3000,
                  3000, 4000,
                  4000, 5000,
                  5000, 6500,
                  6500, 8300,
                  8300, 9000,
                  9000, 9800,
                  9800, 11000,
                  11000, 15000,
                  15000, 20000])

desired = np.array([1, 1,
                    1, 1.1,
                    1.1, 1,
                    1, 1,
                    1, 0.8,
                    0.8, 1,
                    1, 1.2,
                    1.2, 1,
                    1, 0.5,
                    0.5, 1,
                    1, 1.3,
                    1.3, 1,
                    1, 0.7,
                    0.7, 1,
                    1, 1])

taps1 = firls(numtaps, bands, desired, fs=fs)

weights = [0.2,
           0.9,
           0.3,
           0.2,
           0.2,
           0.2,
           0.2,
           0.2,
           0.9,
           0.9,
           0.9,
           0.9,
           0.9,
           0.9,
           0.2]
taps2 = firls(numtaps, bands, desired, weight=weights, fs=fs)

worN = 4096
w, h1 = freqz(taps1, worN=worN, fs=fs)
_, h2 = freqz(taps2, worN=worN, fs=fs)

# Convert to dB — clip to avoid log(0)
def to_db(h, floor=-60):
    return np.maximum(20 * np.log10(np.abs(h) + 1e-12), floor)

h1_db = to_db(h1)
h2_db = to_db(h2)

def piecewise_desired(freqs, bands, desired):
    out = np.full(len(freqs), np.nan)
    for i in range(0, len(bands), 2):
        f1, f2 = bands[i], bands[i+1]
        d1, d2 = desired[i], desired[i+1]
        mask = (freqs >= f1) & (freqs <= f2)
        t = (freqs[mask] - f1) / (f2 - f1 + 1e-12)
        lin = d1 + t * (d2 - d1)
        out[mask] = to_db(lin)
    return out

desired_db = piecewise_desired(w, bands, desired)

fig, axes = plt.subplots(2, 1, figsize=(10, 7))
fig.suptitle("firls — Least-Squares FIR Filter Design", fontsize=14, fontweight="bold", y=0.98)

ax = axes[0]
ax.semilogx(w[1:], h1_db[1:], lw=2, label="Unweighted", color="#2176ae")
ax.semilogx(w[1:], h2_db[1:], lw=2, linestyle="--", label=f"Weighted {weights[0]} (all bands)", color="#d85a30")
ax.semilogx(w[1:], desired_db[1:], lw=1.5, linestyle=":", color="#666", label="Desired response")

band_colors = ["#2176ae22","#d85a3022","#2176ae22","#2dad9421","#2dad3a20",
               "#87ad2d1f","#ad552d1f","#ad2d9e1f","#ad552d1f","#ad552d1f",
               "#ad552d1f","#ad552d1f","#ad552d1f","#ad552d1f","#ad552d1f"]
band_labels = ["0–50","50–80","80–150","150–1k","1k–2k",
               "2k–3k","3k–4k","4k–5k","5k–6.5k","6.5k–8.3k",
               "8.3k–9k","9k–9.8k","9.8k–11k","11k–15k","15k–20k"]

for i in range(0, len(bands), 2):
    ax.axvspan(max(bands[i], 1), bands[i+1], alpha=0.1, color=band_colors[i//2], zorder=0)

# y-axis in dB
ax.set_xlim(20, nyq)
ax.set_ylim(-20, 5)
ax.set_ylabel("Magnitude (dB)", fontsize=11)
ax.set_title("Magnitude response", fontsize=11)
ax.axhline(0, color="#aaa", lw=0.8, linestyle="--")  # 0 dB reference
ax.legend(loc="lower left", fontsize=9)
ax.grid(True, alpha=0.3, which="both")

# x-axis: nice frequency labels
ax.set_xticks([20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000])
ax.set_xticklabels(["20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k"])

# --- Impulse response ---
ax2 = axes[1]
n = np.arange(numtaps)
ax2.stem(n, taps1, linefmt="#2176ae", markerfmt="o", basefmt="k-", label="Unweighted")
ax2.stem(n + 0.3, taps2, linefmt="#d85a30", markerfmt="s", basefmt="k-", label="Weighted")
ax2.axhline(0, color="black", lw=0.8)
ax2.set_xlabel("Tap index", fontsize=11)
ax2.set_ylabel("Amplitude", fontsize=11)
ax2.set_title("Impulse response (filter coefficients)", fontsize=11)
ax2.legend(loc="upper right", fontsize=9)
ax2.grid(True, alpha=0.3)

plt.tight_layout()
plt.show()
#plt.savefig("/mnt/user-data/outputs/firls_db_plot.png", dpi=150, bbox_inches="tight")
#print("done")