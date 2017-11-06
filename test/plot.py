import csv
import sys
import matplotlib.pyplot as plt
import numpy as np

columns = {}

filename = sys.argv[1]

with open(filename, newline='') as csvfile:
    reader = csv.reader(csvfile)
    headers = next(reader)
    for h in headers:
        columns[h.strip()] = []
    for row in reader:
        for h, v in zip(headers, row):
            columns[h.strip()].append(int(v))

fig, ax = plt.subplots()

f = ax.plot(columns["Frames"], columns["Faults"], 'o-')
w = ax.plot(columns["Frames"], columns["Writes"],'o-')
r = ax.plot(columns["Frames"], columns["Reads"],'o-')

ax.xaxis.set_ticks(np.arange(0,105,5))

plt.savefig(filename.split('.')[0] + '.png')
