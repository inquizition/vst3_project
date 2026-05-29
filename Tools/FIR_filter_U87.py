import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import firls, freqz
from scipy.interpolate import PchipInterpolator

fs = 44100
numtaps = 73

def db2lin(db): return 10**(db/20)
def to_db(x):   return 20*np.log10(np.clip(x, 1e-6, None))

# --- FIX 1: define separate control points for the spline ---
# These are the key inflection points of your mic curve, one per region
# (not paired band edges — just unique, strictly increasing frequencies)
ctrl_f = np.array([600,     2000, 2800, 4000,   5500, 9200, 14000,   20000])
ctrl_db = np.array([  0,    0,    0,    0,      0,    2.5,  0,       -10])

# FIX 2: build the spline from ctrl_f/ctrl_db, not from bands
interp = PchipInterpolator(np.log10(ctrl_f), ctrl_db)

f_dense = np.logspace(np.log10(600), np.log10(20000), 2000)
mic_db_smooth  = interp(np.log10(f_dense))
mic_lin_smooth = db2lin(mic_db_smooth)
corr_lin_smooth = np.clip(1.0 / mic_lin_smooth, 0, db2lin(6))

# Sample correction onto 60 log-spaced sub-bands
N_BANDS = 60
f_inner    = 600.0 * (20000.0/600.0)**np.linspace(0, 1, N_BANDS + 1)
f_inner[0] = 600.0;  f_inner[-1] = 20000.0

corr_at_edges = np.interp(f_inner, f_dense, corr_lin_smooth)

bands_list   = [0.0, 600.0];  desired_list = [1.0, 1.0]
for i in range(N_BANDS):
    bands_list.extend([f_inner[i], f_inner[i+1]])
    desired_list.extend([corr_at_edges[i], corr_at_edges[i+1]])

bands   = np.array(bands_list)
desired = np.array(desired_list)

assert bands[1] == bands[2], f"Join mismatch: {bands[1]} != {bands[2]}"
print(f"Bands: {len(bands)//2}, Correction range: {to_db(corr_at_edges.min()):.1f} to {to_db(corr_at_edges.max()):.1f} dB")

taps = firls(numtaps, bands, desired, fs=fs)

worN = 8192
w, H = freqz(taps, worN=worN, fs=fs)
H_db = to_db(np.abs(H))

mic_on_w = np.interp(w, f_dense, mic_lin_smooth, left=1.0, right=mic_lin_smooth[-1])
combined  = mic_on_w * np.abs(H)

fig, axes = plt.subplots(3, 1, figsize=(11, 9), sharex=True)
fig.suptitle("AT4040 correction filter — smooth PCHIP spline  (600 Hz – 20 kHz)",
             fontsize=13, fontweight="bold")

kw = dict(lw=2, alpha=0.9)

ax = axes[0]
ax.semilogx(f_dense, mic_db_smooth, color="#2176ae", **kw, label="Spline fit")
ax.plot(ctrl_f, ctrl_db, "o", color="#2176ae", ms=5, alpha=0.7, label="Control points")
ax.axhline(0, color="#aaa", lw=0.8, ls="--")
ax.set_ylim(-9, 8); ax.set_ylabel("dB", fontsize=10)
ax.set_title("Mic response — PCHIP spline through control points", fontsize=11)
ax.legend(fontsize=9); ax.grid(True, alpha=0.3, which="both")

ax2 = axes[1]
ax2.semilogx(f_dense, to_db(corr_lin_smooth), color="#888", lw=1.5, ls=":", label="Desired correction (smooth)")
ax2.semilogx(w[1:], H_db[1:], color="#d85a30", **kw, label=f"FIR filter  ({numtaps} taps)")
ax2.axhline(0, color="#aaa", lw=0.8, ls="--")
ax2.set_ylim(-8, 8); ax2.set_ylabel("dB", fontsize=10)
ax2.set_title("Correction filter", fontsize=11)
ax2.legend(fontsize=9, loc="upper left"); ax2.grid(True, alpha=0.3, which="both")

ax3 = axes[2]
ax3.semilogx(f_dense, mic_db_smooth, color="#2176ae", lw=1.5, ls="--", alpha=0.5, label="Original mic")
ax3.semilogx(w[1:], to_db(combined[1:]), color="#2dad94", lw=2.5, label="Corrected")
ax3.axhline(0, color="#aaa", lw=0.8, ls="--", label="0 dB target")
ax3.set_ylim(-9, 8); ax3.set_ylabel("dB", fontsize=10)
ax3.set_xlabel("Frequency (Hz)", fontsize=10)
ax3.set_title("Corrected microphone response", fontsize=11)
ax3.legend(fontsize=9, loc="lower left"); ax3.grid(True, alpha=0.3, which="both")

for a in axes:
    a.set_xlim(600, 20000)
    a.set_xticks([1000, 2000, 5000, 10000, 20000])
    a.set_xticklabels(["1k", "2k", "5k", "10k", "20k"], fontsize=9)

plt.tight_layout()
plt.show()
#plt.savefig("/mnt/user-data/outputs/at4040_user_fixed.png", dpi=150, bbox_inches="tight")
print("done")