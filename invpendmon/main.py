__author__ = 'hamid'
import socket

HOST = '128.195.55.205'    # The remote host
PORT = 23             # The same port as used by the server
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))
s.send('Hello, world')
data = s.recv(1024)
s.close()
print 'Received', repr(data)