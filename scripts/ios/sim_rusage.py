#!/usr/bin/env python3
"""Whole-run CPU work of the simulator app, from the kernel's per-process counters.

usage: scripts/ios/sim_rusage.py &   (start it, then launch a run; it waits for the app)
Prints the app's total instructions and cycles (proc_pid_rusage, rusage_info_v4:
ri_instructions, ri_cycles) as last read before it exits. Wall-clock rates on a busy
Mac swing by 2x between identical runs; these counts do not, so A/B the app with them
(docs/archive/status/CURRENT.md, 2026-09-24).
"""
import ctypes, subprocess, time, sys
lib = ctypes.CDLL('/usr/lib/libproc.dylib')
class RU(ctypes.Structure):
    _fields_ = [('uuid', ctypes.c_uint8*16), ('f', ctypes.c_uint64*64)]
def read(pid):
    ru = RU()
    if lib.proc_pid_rusage(pid, 4, ctypes.byref(ru)) != 0: return None
    return ru.f[29], ru.f[30]   # ri_instructions, ri_cycles (rusage_info_v4)
pid=None
t0=time.time()
while pid is None and time.time()-t0<120:
    out=subprocess.run(['pgrep','-f','Bundle/Application/.*/BlueWake.app/BlueWake'],capture_output=True,text=True).stdout.split()
    if out: pid=int(out[0])
    else: time.sleep(0.05)
last=None
while True:
    v=read(pid)
    if v is None: break
    last=v; time.sleep(0.02)
print('pid',pid,'instructions %.3f G cycles %.3f G'%(last[0]/1e9,last[1]/1e9))

