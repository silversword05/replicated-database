import matplotlib.pyplot as plt
from pathlib import Path
import pandas as pd
import numpy as np

col_start = "startTime (micro_seconds)"
col_end = "endTime (micro_seconds)"
col_lat = "latency (micro_seconds)"

machine_count = 3
election_timeout_min = 50
election_timeout_max = 100

election_crash_min = 5000
election_crash_max = 6000
heartbeat_timeout = 1


def plotThrouhputOverTime():
    # Need to give a single csv for whom it will plot
    # raftHome = Path(str(Path.home()) + "/replicated-database/plotting-script/data/crash_impact_election_timeout_2-3/")
    # allCSVs = Path.glob(raftHome, "*.csv")

    # my_csv = str(next(allCSVs)) # update this with correct csv path
    my_csv = '/users/mpatel48/replicated-database/plotting-script/data/crash_impact_election_timeout_2-3/clientLatency_980.csv'
    df = pd.read_csv(my_csv)
    df = df.sort_values(by=[col_end]).reset_index()
    df = df[(df != -1).all(1)].reset_index()
    rolling_window = 1000*1000; # 1s

    thrput = np.array([])
    time_in_sec = np.array([])

    initial_time_microseconds = df[col_end][0]
    for val in df[col_end]:
        col_less_than = df[col_end] < (val + rolling_window)
        col_great_than = val <= df[col_end]
        col = np.logical_and(col_less_than, col_great_than)
        
        thrput = np.append(thrput, col.sum())
        time_relative_sec = (val - initial_time_microseconds) / rolling_window
        time_in_sec = np.append(time_in_sec, time_relative_sec)
    
    # Plt this time series
    plt.clf()
    plt.plot(time_in_sec, thrput)
    plt.xlabel("Time [seconds]")
    plt.ylabel("Throughput [requests/sec]")
    plt.xlim([0,int(np.max(time_in_sec))+1])
    plt.xticks(np.arange(0, int(np.max(time_in_sec))+2, step=4))
    plt.yticks(np.arange(0, 1601, step=200))
    textstr = f'machine count = {machine_count}\n' + \
            f'election timeout = {election_crash_min} - {election_crash_max}ms\n'+\
            f'heartbeat timeout = {heartbeat_timeout}ms'
    props = dict(boxstyle='round', facecolor='wheat', alpha=0.5)
    plt.text(1, 1450, textstr, bbox=props, verticalalignment="center")
    # plt.axvline(x=6, ymin=0, ymax=0.4, color='black', linestyle='--')
    plt.title("Impact of leader crash on throughput")
    plt.savefig("Throughput_over_time.png")

def plotLatVsThrput():
    raftHome = Path(str(Path.home()) + "/replicated-database/plotting-script/data/latvsthrpt-5000/")
    # raftHome = Path(str(Path.home()) + "/replicated-database/plotting-script/data/latvsthrpt-50000/")
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
    plt.yticks(np.arange(0, 21, step=2))
    textstr = f'machine count = {machine_count}\n' + \
            f'election timeout = {election_timeout_min} - {election_timeout_max}ms\n'+\
            f'heartbeat timeout = {heartbeat_timeout}ms'
    props = dict(boxstyle='round', facecolor='wheat', alpha=0.5)
    plt.text(0, 17, textstr, bbox=props)
    plt.savefig("Throughput_vs_Median_Latency.png")

    # Plotting thrput vs 99 percentile latency
    plt.clf()
    plt.plot(all_thrput, all_99_quantile_lat, 'b^') # ro=> red dot, bs => blue squares, g^ => green triangles
    plt.xlabel("Throughput [requests/sec]")
    plt.ylabel("99%ile Latency [ms]")
    plt.xticks(np.arange(0, 3001, step=300))
    plt.yticks(np.arange(0, 67, step=5))
    textstr = f'machine count = {machine_count}\n' + \
            f'election timeout = {election_timeout_min} - {election_timeout_max}ms\n'+\
            f'heartbeat timeout = {heartbeat_timeout}ms'
    props = dict(boxstyle='round', facecolor='wheat', alpha=0.5)
    plt.text(0, 60, textstr, bbox=props, verticalalignment="center")
    plt.savefig("Throughput_vs_99_Percentile_Latency.png")

if __name__ == "__main__":
    plotLatVsThrput()
    plotThrouhputOverTime()
    