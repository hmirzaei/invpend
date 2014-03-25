
__author__ = 'hamid'
import socket
import matplotlib.pyplot as plt  # http://matplotlib.org/
import numpy as np
import time
import sys
import math
import pickle

REC_LEN = 40000

HOST = '128.195.55.252'  # The remote host5
PORT = 23  # The same port as used by the server
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print("trying ...")
s.connect((HOST, PORT))
print("connected")
data = []
counts = [0]
while len(data) < REC_LEN:
    if counts[-1] != 501:
        time.sleep(0.1)
    s.send(bytes("d", 'UTF-8'))
    tmpData = s.recv(10000)
    data.extend(list(tmpData)[1:])
    counts.extend([len(list(tmpData))])
    sys.stdout.write("\r%d%%" %(float(len(data))/REC_LEN*100))    # or print >> sys.stdout, "\r%d%%" %i,
    sys.stdout.flush()

print("max count: ", max(counts))
print(counts)
plt.plot(counts)
plt.show()

print("done")
print(len(data))

data = data[:math.floor(len(data)/20)*20]
data2 = [data[i:i+4] for i in range(0,math.floor(len(data)/4),4)]
print('\n'.join([ str(myelement) for myelement in data2[::5] ]))
data = np.array(data)
data = data[0::4] + (data[1::4] << 8) + (data[2::4] << 16) + (data[3::4] << 24)
data = data.reshape(len(data)/5,5)

pickle.dump(data, open('save.p', 'wb'), 2)
data = pickle.load(open("save.p", 'rb'))

data2 = np.zeros(data.shape, float)
for i in range(data.shape[0]):
    for j in range(data.shape[1]):
        if data[i,j]> (1<<31):
            data2[i,j] = data[i,j] - (1<<32)
        else:
            data2[i,j] = data[i,j]

print(data[:,0])
data2[:,0] = 50000000.0/data2[:,0]/4000/4
data2[:,1] = data2[:,1]/4000.0/4
data2[:,2] = 50000000.0/data2[:,2]/720
data2[:,3] = data2[:,3]/720.0
data2[:,4] = -data2[:,4]/3333.33

t = np.array(range(data2.shape[0]))*3e-3

plt.subplot(5,1,1)
plt.plot(t,data2[:,1]*360)
plt.title('Pendulum Angle')
plt.title('Pendulum Angular Speed')
plt.ylabel('deg/s')
plt.grid()
plt.ylabel('deg')

plt.subplot(5,1,2)
plt.plot(t,data2[:,0]*360)
plt.title('Pendulum Angular Speed')
plt.grid()
plt.ylabel('deg/s')

plt.subplot(5,1,3)
plt.plot(t,data2[:,3]*360)
plt.title('Motor Angle')
plt.grid()
plt.ylabel('deg')


plt.subplot(5,1,4)
plt.plot(t,data2[:,2]*360)
plt.title('Motor Angular Speed')
plt.grid()
plt.ylabel('deg/s')

plt.subplot(5,1,5)
plt.plot(t,data2[:,4]*100)
plt.title('PWM Command')
plt.grid()
plt.ylabel('%')

# a= 100*data2[:,1]-1.5*data2[:,0]
# a=(a>1).choose(a,1)
# a=(a<-1).choose(a,-1)
# plt.plot(t,a*100)

plt.show()

