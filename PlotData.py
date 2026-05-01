import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker  

def plot_s11_comparison(file_paths, labels=None):
    """
    Plots multiple S11 datasets on the same graph for comparison.
    
    :param file_paths: List of strings containing paths to .s1p files
    :param labels: List of strings for the legend labels
    """
    fig, ax = plt.subplots(figsize=(12, 7))
    
    # If no labels provided, use filenames
    if labels is None:
        labels = file_paths

    # Loop through each file and plot
    for file, label in zip(file_paths, labels):
        # Load data, skipping comments
        data = np.loadtxt(file, comments=['#', '!'])
        
        # Extract Frequency (assume Hz) and convert to GHz
        freq_ghz = data[:, 0] / 1e9 
        
        # Calculate Magnitude in dB from Real (col 1) and Imaginary (col 2)
        s11_db = 20 * np.log10(np.abs(data[:, 1] + 1j * data[:, 2]))
        
        # Plot the line
        ax.plot(freq_ghz, s11_db, label=label)

    # --- Formatting the Graph ---
    
    # Detailed Y-axis ticks
    ax.yaxis.set_major_locator(ticker.MultipleLocator(1))
    
    # Highlight GNSS Bands
    ax.axvspan(1.164, 1.189, color='cyan', alpha=0.1, label='L5 Band')
    ax.axvspan(1.559, 1.591, color='magenta', alpha=0.1, label='L1 Band')

    # Add -10dB Reference Line
    ax.axhline(-10, color='red', linestyle='--', linewidth=1, label='-10dB Match')

    ax.set_title('S11 Return Loss Comparison')
    ax.set_xlabel('Frequency (GHz)')
    ax.set_ylabel('Magnitude (dB)')
    ax.grid(True, which='both', linestyle=':', alpha=0.5)
    
    # Place legend to the side or top to avoid covering data
    ax.legend(loc='upper right', fontsize='small')

    plt.tight_layout()
    plt.show()

# Example usage:
# Replace 'Dipol_2.s1p' with the name of your second data file
files_to_plot = ['Dipol.s1p', 'Egen_GNSS.s1p']
display_names = ['Dipole Antenna A', 'Egen GNSS Antenna B']

plot_s11_comparison(files_to_plot, labels=display_names)