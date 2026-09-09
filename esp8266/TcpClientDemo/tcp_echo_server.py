import socket


HOST = "0.0.0.0"
PORT = 5000


with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen()
    print(f"TCP echo server listening on {HOST}:{PORT}")

    while True:
        connection, address = server.accept()
        print(f"client connected: {address[0]}:{address[1]}")

        with connection:
            while True:
                data = connection.recv(1024)
                if not data:
                    break

                print(f"received {len(data)} bytes: {data!r}")
                connection.sendall(b"ECHO: " + data)

        print("client disconnected")
