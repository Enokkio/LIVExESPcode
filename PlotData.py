import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker  

def plot_s11_refined(file_path):
    data = np.loadtxt(file_path, comments=['#', '!'])
    
    freq_ghz = data[:, 0] / 1e9 
    s11_db = 20 * np.log10(np.abs(data[:, 1] + 1j * data[:, 2]))

    fig, ax = plt.subplots(figsize=(12, 7))
    ax.plot(freq_ghz, s11_db, color='black', label='$S_{11}$')

    ax.yaxis.set_major_locator(ticker.MultipleLocator(1))
    
    ax.axvspan(1.164, 1.189, color='cyan', alpha=0.2, label='L5 Band')
    ax.axvspan(1.559, 1.591, color='magenta', alpha=0.2, label='L1 Band')

    ax.axhline(-10, color='red', linestyle='--', linewidth=1, label='-10dB Match')

    ax.set_title('S11 Return Loss (Detailed Y-Axis)')
    ax.set_xlabel('Frequency (GHz)')
    ax.set_ylabel('Magnitude (dB)')
    ax.grid(True, which='both', linestyle=':', alpha=0.5)
    ax.legend()

    plt.show()
plot_s11_refined('Egen_GNSS.s1p')