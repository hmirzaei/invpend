__author__ = 'hamid'
import socket
#import matplotlib.pyplot as plt  # http://matplotlib.org/
#import numpy as np
import time

MON_DATA_LEN = 700 * 4

HOST = '128.195.55.252'  # The remote host
PORT = 23  # The same port as used by the server
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print "trying ..."
s.connect((HOST, PORT))
print "connected"
data2 = []
while len(data2) < 150000:
    data = []
    s.send('H')
    tmpData = s.recv(10000)
    data.extend(map(ord, list(tmpData)))
    while len(data) != MON_DATA_LEN:
        tmpData = s.recv(10000)
        data.extend(map(ord, list(tmpData)))

    print len(data)
    count = data[-4]+data[-3] << 8+data[-2] << 16+data[-1] << 24
    print count
    data2.extend(data[:count])
    #counter = (tmpData[-4]+tmpData[-3]<<8+tmpData[-2]<<16+tmpData[-1]<<24)*4
    #data.extend(map(ord,list(tmpData))[:counter])

print "done"
print(len(data2))

s.send('X')
tmpData = s.recv(10000)
data.extend(map(ord, list(tmpData)))
while len(data) != MON_DATA_LEN:
    tmpData = s.recv(10000)
    data.extend(map(ord, list(tmpData)))

s.close()

# plt.plot(data)
# plt.show()

