import socket
import sys

def send_file(source_ip, source_port, dest_ip, dest_port, file_path):
    with socket.socket() as sock, open(file_path, 'rb') as file:
        sock.bind((source_ip, source_port))
        sock.connect((dest_ip, dest_port))
        buff = file.read(1024)
        while buff :
            sock.send(buff)
            buff = file.read(1024)
        sock.shutdown(socket.SHUT_WR)

if __name__ == "__main__":
    source_ip = "0.0.0.0"
    if len(sys.argv)< 5:
        print("not enough args")
        exit(1)
    source_port = int(sys.argv[1])
    dest_ip = sys.argv[2]
    dest_port = int(sys.argv[3])
    file_path = sys.argv[4]
    try:
        send_file(source_ip, source_port, dest_ip, dest_port, file_path)
    except Exception as e:
        print("Error while sending the file:", e)