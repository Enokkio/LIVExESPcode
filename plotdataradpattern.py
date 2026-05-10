import numpy as np
import matplotlib.pyplot as plt
import os

DATA_FOLDER = 'CP ANT20260507'

def load_data(filename):
    filepath = os.path.join(DATA_FOLDER, filename)
    numeric_data = []
    if not os.path.exists(filepath): return None
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            parts = line.split()
            if len(parts) >= 2:
                try:
                    numeric_data.append([float(parts[0].replace(',', '.')), float(parts[1].replace(',', '.'))])
                except ValueError: continue
    return np.array(numeric_data)

def get_total_power(fh, fv):
    dh, dv = load_data(fh), load_data(fv)
    if dh is None or dv is None: return None, None
    gain_v_interp = np.interp(dh[:, 0], dv[:, 0], dv[:, 1])
    p_sum_lin = 10**(dh[:, 1]/10) + 10**(gain_v_interp/10)
    return np.radians(dh[:, 0]), 10 * np.log10(p_sum_lin)

def plot_raw_component(ax, fh, fv, title):
    dh, dv = load_data(fh), load_data(fv)
    if dh is None or dv is None: return
    rads = np.radians(dh[:, 0])
    v_interp = np.interp(dh[:, 0], dv[:, 0], dv[:, 1])
    total_lin = 10**(dh[:, 1]/10) + 10**(v_interp/10)
    total_db = 10 * np.log10(total_lin)
    peak = np.max(total_db)

    ax.plot(rads, dh[:, 1] - peak, label='H-Pol (dB)', linestyle='--', alpha=0.6)
    ax.plot(rads, v_interp - peak, label='V-Pol (dB)', linestyle='--', alpha=0.6)
    ax.plot(rads, total_db - peak, label='TOTAL GAIN (dB)', color='black', linewidth=2)
    ax.set_title(title, fontweight='bold', pad=30, fontsize=10)
    ax.legend(loc='upper center', bbox_to_anchor=(0.5, -0.15), fontsize=8)

def format_polar_ax(ax):
    ax.set_theta_zero_location('N')
    ax.set_theta_direction(-1)
    ax.set_rlim([-35, 0])
    ax.set_rticks([-30, -20, -10, 0])
    ax.set_yticklabels(["-30dB", "-20dB", "-10dB", "0dB"], fontsize=7)


l5_0h, l5_0v = "Ant2_Hpol_1,176GHz0deg.txt.txt", "Ant2_Vpol_1,176GHz0deg.txt.txt"
l5_90h, l5_90v = "Ant2_Hpol_1,176GHz90deg.txt.txt", "Ant2_Vpol_1,176GHz90deg.txt.txt"
l1_0h, l1_0v = "Ant2_Hpol_1,575GHz0deg.txt.txt", "Ant2_Vpol_1,575GHz0deg.txt.txt.txt"
l1_90h, l1_90v = "Ant2_Hpol_1,575GHz90deg.txt.txt", "Ant2_Vpol_1,575GHz90deg.txt.txt"

fig1, axs5 = plt.subplots(1, 3, figsize=(18, 6), subplot_kw={'projection': 'polar'})
plot_raw_component(axs5[0], l5_0h, l5_0v, "L5 (1.176 GHz) - 0° Cut")
plot_raw_component(axs5[1], l5_90h, l5_90v, "L5 (1.176 GHz) - 90° Cut")
r0, g0 = get_total_power(l5_0h, l5_0v)
r90, g90 = get_total_power(l5_90h, l5_90v)
axs5[2].plot(r0, g0 - np.max(g0), label='Combined 0° (dB)', linewidth=2)
axs5[2].plot(r90, g90 - np.max(g90), label='Combined 90° (dB)', linewidth=2)
axs5[2].set_title("L5: NORMALIZED TOTAL GAIN (dB)", fontweight='bold', pad=30, color='darkred')
axs5[2].legend(loc='upper center', bbox_to_anchor=(0.5, -0.15))
for ax in axs5: format_polar_ax(ax)
fig1.tight_layout()


fig2, axs1 = plt.subplots(1, 3, figsize=(18, 6), subplot_kw={'projection': 'polar'})
plot_raw_component(axs1[0], l1_0h, l1_0v, "L1 (1.575 GHz) - 0° Cut")
plot_raw_component(axs1[1], l1_90h, l1_90v, "L1 (1.575 GHz) - 90° Cut")
r0, g0 = get_total_power(l1_0h, l1_0v)
r90, g90 = get_total_power(l1_90h, l1_90v)
axs1[2].plot(r0, g0 - np.max(g0), label='Combined 0° (dB)', linewidth=2)
axs1[2].plot(r90, g90 - np.max(g90), label='Combined 90° (dB)', linewidth=2)
axs1[2].set_title("L1: NORMALIZED TOTAL GAIN (dB)", fontweight='bold', pad=30, color='darkblue')
axs1[2].legend(loc='upper center', bbox_to_anchor=(0.5, -0.15))
for ax in axs1: format_polar_ax(ax)
fig2.tight_layout()

plt.show()