import requests
import sys
import os

def send_file(dest_host, dest_port, file_path):
    with open(file_path, 'rb') as f:
        r = requests.post(
            'http://{}:{}/upload'.format(dest_host, dest_port), 
            files={os.path.basename(file_path): f}
        )

if __name__ == "__main__":
    if len(sys.argv)< 4:
        print("not enough args")
        exit(1)
    dest_host= sys.argv[1]
    dest_port = int(sys.argv[2])
    file_path = sys.argv[3]
    try:
        send_file(dest_host, dest_port, file_path)
    except Exception as e:
        print("Error while sending the file:", e)