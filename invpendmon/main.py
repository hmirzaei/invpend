__author__ = 'hamid'
import socket
import matplotlib.pyplot as plt  # http://matplotlib.org/
import numpy as np
import time

HOST = '128.195.55.205'    # The remote host
PORT = 23             # The same port as used by the server
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))

data=[]
start = time.time()
for i in range(10000):
    s.send('H')
    tmpData = s.recv(1000)
    data.extend(map(ord,list(tmpData)))
end = time.time()
print end - start
s.close()

data = np.array(data)
data = data[0::2]+data[1::2]*256
# plt.plot(data)
# plt.show()




