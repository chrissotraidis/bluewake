#!/usr/bin/env python3
"""Poll a process's memory footprint and resident size with the retrace its log has reached.
usage: mem_poll.py PGREP_PATTERN LOGFILE OUT   (rows: seconds retrace phys_footprint_MB resident_MB)"""
import ctypes, subprocess, time, sys, re
lib = ctypes.CDLL('/usr/lib/libproc.dylib')
class RU(ctypes.Structure):
    _fields_ = [('uuid', ctypes.c_uint8*16), ('f', ctypes.c_uint64*64)]
pat, logf, outf = sys.argv[1:4]
pid=None; t0=time.time()
while pid is None and time.time()-t0<300:
    # pgrep -f also matches the shell that launched the run (its command line
    # holds the pattern), so keep only processes that are not shells or python.
    o=[int(x) for x in subprocess.run(['pgrep','-f',pat],capture_output=True,text=True).stdout.split()]
    o=[x for x in o if subprocess.run(['ps','-o','comm=','-p',str(x)],capture_output=True,text=True).stdout.strip().rsplit('/',1)[-1] not in ('zsh','bash','sh','python3','Python')]
    if o: pid=o[0]
    else: time.sleep(0.2)
out=open(outf,'w'); start=time.time(); pos=0; retr=0
while True:
    ru=RU()
    if lib.proc_pid_rusage(pid,4,ctypes.byref(ru))!=0: break
    try:
        with open(logf,'rb') as f:
            f.seek(pos); chunk=f.read(); pos+=len(chunk)
        for m in re.finditer(rb'frame-timing\] retrace=(\d+)',chunk): retr=int(m.group(1))
    except FileNotFoundError: pass
    out.write('%.1f %d %.1f %.1f\n'%(time.time()-start,retr,ru.f[7]/2**20,ru.f[6]/2**20)); out.flush()
    time.sleep(5)
