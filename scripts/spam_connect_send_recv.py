import socket
import time


def main():
    host = "127.0.0.1"
    port = 54321
    message = "hello\r\n"

    while True:
        try:
            # Create a TCP/IP socket
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
                print(f"Connecting to {host}:{port}...")
                sock.connect((host, port))

                # Send the message
                print(f"Sending: {message.strip()}")
                sock.sendall(message.encode())

                # Receive the response
                response = sock.recv(1024)  # Adjust buffer size as needed
                print(f"Received: {response.decode().strip()}")

        except ConnectionRefusedError:
            print(f"Connection to {host}:{port} refused. Retrying in 100ms...")
        except socket.timeout:
            print(f"Connection timed out. Retrying in 100ms...")
        except socket.error as e:
            print(f"Socket error: {e}. Retrying in 100ms...")
        except Exception as e:
            print(f"An unexpected error occurred: {e}. Retrying in 100ms...")

        time.sleep(0.1)  # Wait for 5 before trying again


if __name__ == "__main__":
    main()
