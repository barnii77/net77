import socket
import time


def start_server():
    host = 'localhost'
    port = 12345

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((host, port))
    server_socket.listen(1)
    print(f"Server started at {host}:{port}")

    while True:
        conn, addr = server_socket.accept()
        print(f"Connection established with {addr}")

        try:
            # Read the incoming data from the client
            data = conn.recv(1024).decode('utf-8')
            timestamp = int(time.time_ns() // 1000)
            print(f"[{timestamp}] Received message: `{data[:5]}`...")

            # Verify the content
            if data == "hello\r\n" * 100:
                response = "hi\r\n" * 100
                conn.sendall(response.encode('utf-8'))
                timestamp = int(time.time_ns() // 1000)
                print(f"[{timestamp}] Sent response: `{response[:5]}`...")
            else:
                print("Invalid message received.")

        except Exception as e:
            print(f"Error: {e}")
        finally:
            conn.close()


if __name__ == "__main__":
    start_server()
