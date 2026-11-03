#!/usr/bin/env python3
"""Poll a game process's instructions and cycles alongside the retrace in its log.
usage: rate_poll.py PGREP_PATTERN LOGFILE OUT   (waits for the process, polls every 0.5 s)
Rows: wall_seconds retrace instructions cycles."""
import ctypes, subprocess, time, sys, re, os
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
    else: time.sleep(0.1)
out=open(outf,'w'); start=time.time(); pos=0; retr=0
while True:
    ru=RU()
    if lib.proc_pid_rusage(pid,4,ctypes.byref(ru))!=0: break
    try:
        with open(logf,'rb') as f:
            f.seek(pos); chunk=f.read(); pos+=len(chunk)
        for m in re.finditer(rb'frame-timing\] retrace=(\d+)',chunk): retr=int(m.group(1))
    except FileNotFoundError: pass
    out.write('%.2f %d %d %d\n'%(time.time()-start,retr,ru.f[29],ru.f[30])); out.flush()
    time.sleep(0.5)
