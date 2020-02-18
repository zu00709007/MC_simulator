import os
import socket
import threading

IP_ADDRESS = '140.109.21.158';
IP_PORT = 4000

frame_num = input('Input Request: ')
try:
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.connect((IP_ADDRESS, IP_PORT))
except socket.error as msg:
	sys.exit(1)

s.send(frame_num.encode())
uri = s.recv(1024).decode()
print(uri)
s.close()