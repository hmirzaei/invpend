__author__ = 'hamid'
import socket
#import matplotlib.pyplot as plt  # http://matplotlib.org/
#import numpy as np
import time

HOST = '128.195.55.205'    # The remote host
PORT = 23             # The same port as used by the server
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))

data=[]
while len(data) < 15000:
    s.send('H')
    tmpData = s.recv(4000)
    counter = (tmpData[-4]+tmpData[-3]<<8+tmpData[-2]<<16+tmpData[-1]<<24)*4
    data.extend(map(ord,list(tmpData))[:counter])

s.send('H')
tmpData = s.recv(4000)
s.close()

# plt.plot(data)
# plt.show()




