import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker  

def plot_s11_comparison(file_paths, labels=None):
 
    fig, ax = plt.subplots(figsize=(12, 7))
    
    if labels is None:
        labels = file_paths

    for file, label in zip(file_paths, labels):
        data = np.loadtxt(file, comments=['#', '!'])
        
        freq_ghz = data[:, 0] / 1e9 
        
        s11_db = 20 * np.log10(np.abs(data[:, 1] + 1j * data[:, 2]))
        
        ax.plot(freq_ghz, s11_db, label=label)

    ax.yaxis.set_major_locator(ticker.MultipleLocator(1))
    
    ax.axvspan(1.164, 1.189, color='cyan', alpha=0.1, label='L5 Band')
    ax.axvspan(1.559, 1.591, color='magenta', alpha=0.1, label='L1 Band')

    ax.axhline(-10, color='red', linestyle='--', linewidth=1, label='-10dB Match')

    ax.set_title('S11 Return Loss')
    ax.set_xlabel('Frequency (GHz)')
    ax.set_ylabel('Magnitude (dB)')
    ax.grid(True, which='both', linestyle=':', alpha=0.5)
    ax.legend(loc='upper right', fontsize='small')
    ax.set_xlim(1.1, 1.7)
    plt.tight_layout()
    plt.show()


files_to_plot = [ 'Egen_GNSS.s1p','Dipol.s1p']
display_names = ['Dipol Antenn A', 'Egen GNSS Antenna B']

plot_s11_comparison(files_to_plot, labels=display_names)