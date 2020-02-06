import os
import socket

# Packet structure:
# First byte: CODE (request code OR response code)
# Rest bytes: Message

# Request codes
REQCODE_DIR_LIST = 10 #0x0a



PCKT_LEN_MAX = 15000

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

def req_dir_list(dir_path):
    # Encode first byte (len = 1)
    # concat the dir_path
    message = REQCODE_DIR_LIST.to_bytes(1, 'big') + str.encode(dir_path)
    print(message)
    s.connect(("localhost", 9999))
    s.send(message)

    recv = s.recv(PCKT_LEN_MAX)
    print("Got: " +str(recv))

    print("\n\nDirectory list:")

    values = recv.decode('ascii').split('\x00')

    print(values)
    pass

dir_path = "/home/shlomi/"
req_dir_list(dir_path)
