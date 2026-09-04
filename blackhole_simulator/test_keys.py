import subprocess
import time
import pty
import os

master, slave = pty.openpty()
p = subprocess.Popen(["./blackhole_sim"], stdin=slave, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
time.sleep(1)

# Send Right Arrow
os.write(master, b'\x1b[C')
time.sleep(0.5)
os.write(master, b'q')
time.sleep(0.5)

out, err = p.communicate()
print([line for line in out.split(b'\n') if b'FOV=' in line][-1])
