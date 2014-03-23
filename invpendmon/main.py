
__author__ = 'hamid'
import socket
import matplotlib.pyplot as plt  # http://matplotlib.org/
import numpy as np
import time
import sys

REC_LEN = 80000

HOST = '192.168.1.104'  # The remote host5
PORT = 23  # The same port as used by the server
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print "trying ..."
s.connect((HOST, PORT))
print "connected"
data = []
counts = [0]
while len(data) < REC_LEN:
    if counts[-1] != 501:
        time.sleep(0.1)
    s.send('H')
    tmpData = s.recv(10000)
    data.extend(map(ord, list(tmpData))[1:])
    counts.extend([len(list(tmpData))])
    sys.stdout.write("\r%d%%" %(float(len(data))/REC_LEN*100))    # or print >> sys.stdout, "\r%d%%" %i,
    sys.stdout.flush()

print "max count: ", max(counts)
print counts
plt.plot(counts)
plt.show()

print "done"
print(len(data))

data = data[:len(data)/20*20]
data2 = [data[i:i+4] for i in range(0,len(data)/4,4)]
print '\n'.join([ str(myelement) for myelement in data2[::5] ])
data = np.array(data)
data = data[0::4] + (data[1::4] << 8) + (data[2::4] << 16) + (data[3::4] << 24)
data = data.reshape(len(data)/5,5)

plt.plot(data)
plt.show()

s.send('X')
tmpData = s.recv(10000)
data.extend(map(ord, list(tmpData)))
while len(data) != MON_DATA_LEN:
    tmpData = s.recv(10000)
    data.extend(map(ord, list(tmpData))[1:])

s.close()

# plt.plot(data)
# plt.show()

