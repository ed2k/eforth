import socket, sys

api = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
api_ep = ('192.168.0.11',27777)



def api_send(s):
  api.sendto(s+'\0', api_ep)

api_send('\0'+' '.join(sys.argv[1:]))
