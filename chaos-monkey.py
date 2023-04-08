import random
import time
import argparse
import sys
import os
import signal
import subprocess

def chaos_monkey(p: float, t: int):
    ip = subprocess.getoutput("ifconfig | grep '10.10' | xargs | cut -d ' ' -f2")
    while(True):
        time.sleep(t)
        r = random.uniform(0,1)
        if r < p:
            print("Performing partition")
            os.system("sudo iptables -I OUTPUT -s " + ip + " -j DROP")
            os.system("sudo iptables -I INPUT -s " + ip + " -j DROP")
        else:
            print("Withdrawing partitions")
            os.system("sudo iptables -F")

def last_call():
    os.system("sudo iptables -F")
    print("Withdrawing final partition")
    sys.exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-p", default=0.3, type=float, help="probability of partition")
    parser.add_argument("-t", default=5, type=int, help="Time for sleep")
    args = parser.parse_args()
    signal.signal(signal.SIGINT, lambda x,y: last_call())
    chaos_monkey(args.p, args.t)

# echo $(ifconfig | grep '10.10' | xargs | cut -d " " -f2)