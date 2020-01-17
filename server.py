import os
import sys
import socket
import threading

IP_ADDRESS = '127.0.0.1';
IP_PORT = 3000

def sendToProxy(conn, addr):
	file_name = conn.recv(1024).decode()
	file_size = os.path.getsize(file_name)
	conn.send(str(file_size).encode())
	with open(file_name, 'rb') as fp:
		for partial in fp:
			conn.send(partial)
	conn.close()

try:
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	s.bind((IP_ADDRESS, IP_PORT))
	s.listen(10)
except socket.error as msg:
	sys.exit(1)

while True:
	conn, addr = s.accept()
	t = threading.Thread(target = sendToProxy, args = (conn, addr))
	t.start()