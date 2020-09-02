#!/usr/bin/env python

import subprocess
import time
import sys

def get_freq(fs_desired):
    freq_read = subprocess.check_output(["get.site","0", "SIG:CLK_MB:FREQ"])
    freq_read = float(freq_read[:-1].split(" ")[1])
    setpt_write = subprocess.check_output(["set.site","0", "SIG:CLK_MB:SET",str(fs_desired)])
    setpt_read = subprocess.check_output(["set.site","0", "SIG:CLK_MB:SET"])
    setpt_read = setpt_read[:-1].split(" ")[1]
    return freq_read,setpt_read


def set_clock():
    fs_desired = float(sys.argv[1]) # Always pass desired freq setpoint from sync_role call
    less_1pct = fs_desired - (0.01*fs_desired)
    plus_1pct = fs_desired + (0.01*fs_desired)

    freq_read,setpt_read = get_freq(fs_desired)


    for attempt in range(0, 10): # Only try 10 times to set the clock.
        if less_1pct <= freq_read <= plus_1pct :
            return True
        else:
            time.sleep(0.5)
            print "\nClock Monitor Catch :: Setpoint = {}, MB_CLK Output = {}".format(fs_desired,freq_read)
            print "Thresholds {} {}".format(less_1pct,plus_1pct)
            print "Resetting MB_CLK ",fs_desired
            freq_read,setpt_read = get_freq(fs_desired)
    return False

if not set_clock():
    print("Max retries met. Clock not set.")
