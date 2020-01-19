import os
import sys
import socket
import threading

IP_ADDRESS = '140.109.21.160';
IP_PORT = 4000

frame_num = input('Input Request: ')
try:
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.connect((IP_ADDRESS, IP_PORT))
except socket.error as msg:
	sys.exit(1)

s.send(frame_num.encode())
frame_size = int(s.recv(1024).decode())
with open(frame_num, 'wb') as fp:
	recv_size = 0
	while recv_size < frame_size:
		partial = s.recv(1024)
		fp.write(partial)
		recv_size += len(partial)
s.close()