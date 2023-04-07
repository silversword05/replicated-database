import matplotlib.pyplot as plt
from pathlib import Path
import pandas as pd
import numpy as np

col_start = "startTime (micro_seconds)"
col_end = "endTime (micro_seconds)"
col_lat = "latency (micro_seconds)"

def plotThrouhputOverTime(df):
    print(df[col_lat].median())
    print(df[col_lat].quantile(0.99))

def plotLatVsThrput():
    raftHome = Path(str(Path.home()) + "/raft-home/")
    allCSVs = Path.glob(raftHome, "*.csv")

    all_thrput = np.array([])
    all_median_lat = np.array([])
    all_99_quantile_lat = np.array([])

    for csv in allCSVs:
        thrput = float(str(csv)[str(csv).find("_")+1:str(csv).find(".csv")])
        all_thrput = np.append(all_thrput, thrput)

        df = pd.read_csv(str(csv))
        df[col_lat] = df[col_end] - df[col_start]
        
        all_median_lat = np.append(all_median_lat, df[col_lat].median() / 1000.0)
        all_99_quantile_lat = np.append(all_99_quantile_lat, df[col_lat].quantile(0.99) / 1000.0)
    
    # Plotting thrput vs median
    plt.clf()
    plt.plot(all_thrput, all_median_lat, 'ro') # ro=> red dot, bs => blue squares, g^ => green triangles
    plt.xlabel("Throughput [requests/sec]")
    plt.ylabel("Median Latency [ms]")
    plt.xticks(np.arange(0, 3001, step=300))
    plt.yticks(np.arange(0, 20, step=2))
    plt.savefig("Throughput vs Median Latency.png")

    # Plotting thrput vs 99 percentile latency
    plt.clf()
    plt.plot(all_thrput, all_99_quantile_lat, 'b^') # ro=> red dot, bs => blue squares, g^ => green triangles
    plt.xlabel("Throughput [requests/sec]")
    plt.ylabel("99%ile Latency [ms]")
    plt.xticks(np.arange(0, 3001, step=300))
    plt.yticks(np.arange(0, 20, step=2))
    plt.savefig("Throughput vs 99 Percentile Latency.png")



if __name__ == "__main__":
    plotLatVsThrput()
    