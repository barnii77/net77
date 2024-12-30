import socket
import time


def start_client():
    host = 'localhost'
    port = 12345

    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((host, port))

    message = "hello\r\n" * 100
    timestamp = int(time.time_ns() // 1000)
    print(f"[{timestamp}] Sending message: {message[:5]}...")

    client_socket.sendall(message.encode('utf-8'))

    # Wait for the server's response
    response = client_socket.recv(1024).decode('utf-8')
    timestamp = int(time.time_ns() // 1000)
    print(f"[{timestamp}] Received response: {response[:5]}...")
    time.sleep(0.01)
    print(f"Now it is [{int(time.time_ns() // 1000)}]")

    if response == "hi\r\n" * 100:
        print("Test passed: Server responded correctly.")
    else:
        print("Test failed: Server response incorrect.")

    client_socket.close()


if __name__ == "__main__":
    start_client()
