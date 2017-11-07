#!/bin/bash

python plot.py fifo/sort_results.csv "FIFO Sort"
python plot.py fifo/scan_results.csv "FIFO Scan"
python plot.py fifo/focus_results.csv "FIFO Focus"

python plot.py random/sort_results.csv "Random Sort"
python plot.py random/scan_results.csv "Random Scan"
python plot.py random/focus_results.csv "Random Focus"

python plot.py custom/sort_results.csv "Random Sort"
python plot.py custom/scan_results.csv "Random Scan"
python plot.py custom/focus_results.csv "Random Focus"
