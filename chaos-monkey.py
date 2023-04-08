import random
import time
import argparse
import sys
import os
import signal

def chaos_monkey(p, ip):
    while(True):
        time.sleep(5)
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
    parser.add_argument("-ip", required=True, type=str, help="ip of the node to be partitioned")
    args = parser.parse_args()
    signal.signal(signal.SIGINT, lambda x,y: last_call())
    chaos_monkey(args.p, args.ip)
