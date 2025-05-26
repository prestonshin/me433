import matplotlib.pyplot as plt # for plotting
import numpy as np # for sine function
import csv


file = 'C:/Users/prest/Documents/GitHub/me433/HW10/sigA.csv'
X = 150

t = [] # column 0
data1 = [] # column 1
filtered_data = []

with open(file) as f:
    # open the csv file
    reader = csv.reader(f)
    i = 0
    for row in reader:
        # read the rows 1 one by one
        t.append(float(row[0])) # leftmost column
        data = float(row[1])
        data1.append(data) # second column
        if i < X:
            avg = sum(data1[0:i+1]) / (i+1)
        else:
            avg = sum(data1[i-X+1:i+1]) / X
        filtered_data.append(avg)
        i += 1


# FFT
Fs = (1.0*(len(t)))/t[-1] # sample rate
Ts = 1.0/Fs; # sampling interval
ts = t
y = data1 # the data to make the fft from
n = len(y) # length of the signal
k = np.arange(n)
T = n/Fs
frq = k/T # two sides frequency range
frq = frq[range(int(n/2))] # one side frequency range
Y = np.fft.fft(y)/n # fft computing and normalization
Y = Y[range(int(n/2))]


# Filtered data FFT
y_filtered = filtered_data
n_filtered = len(y_filtered)
k_filtered = np.arange(n_filtered)
T_filtered = n_filtered/Fs
frq_filtered = k_filtered/T_filtered
frq_filtered = frq_filtered[range(int(n_filtered/2))]
Y_filtered = np.fft.fft(y_filtered)/n_filtered
Y_filtered = Y_filtered[range(int(n_filtered/2))]


fig, (ax1, ax2) = plt.subplots(2, 1)
plt.suptitle(f'SigA MAF (X = {X} points)')
ax1.plot(t, data1, 'k', label='Unfiltered', linewidth=1)
ax1.plot(t, filtered_data, 'r', label='Filtered', linewidth=1)
ax1.set_xlabel('Time')
ax1.set_ylabel('Amplitude')
ax1.legend()
ax1.grid(True, alpha=0.3)
ax1.set_xlabel('Time')

ax1.set_ylabel('Amplitude')
ax2.loglog(frq, abs(Y), 'k', label='Unfiltered', linewidth=1)
ax2.loglog(frq_filtered, abs(Y_filtered), 'r', label='Filtered', linewidth=1)
ax2.set_xlabel('Freq (Hz)')
ax2.set_ylabel('|Y(freq)|')
ax2.legend()
ax2.grid(True, alpha=0.3)

ax2.set_xlabel('Freq (Hz)')
ax2.set_ylabel('|Y(freq)|')

plt.show()


