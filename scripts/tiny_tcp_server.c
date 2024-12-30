#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 54321
#define BUFFER_SIZE 1024
#define MESSAGE "hello\r\n"
#define MESSAGE_REPEAT 100
#define RESPONSE "hi\r\n"

void get_timestamp(char *buffer, size_t len) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    snprintf(buffer, len, "[%ld]", (tv.tv_sec * 1000000L + tv.tv_usec) / 1000);
}

int set_send_recv_timeout(size_t fd, ssize_t timeout_usec) {
    if (timeout_usec >= 0) {
        struct timeval timeout;
        timeout.tv_sec = timeout_usec / 1000000;
        timeout.tv_usec = timeout_usec % 1000000;
        if (setsockopt((int) fd, SOL_SOCKET, SO_RCVTIMEO, (const void *) &timeout, sizeof(timeout))) {
            return 1;
        }
        if (setsockopt((int) fd, SOL_SOCKET, SO_SNDTIMEO, (const void *) &timeout, sizeof(timeout))) {
            return 1;
        }
    }
    return 0;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    char timestamp[32];
    int send_recv_timeout_usec = 50000;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", PORT);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *) &address, &addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }
        if (set_send_recv_timeout(new_socket, send_recv_timeout_usec)) {
            perror("Set send/recv timeout failed");
            close(new_socket);
            continue;
        }

        printf("Connection established\n");

        memset(buffer, 0, sizeof(buffer));
        read(new_socket, buffer, sizeof(buffer));

        get_timestamp(timestamp, sizeof(timestamp));
        printf("%s Received message: `%.*s`...\n", timestamp, 5, buffer);

        // Verify the content
        char expected_message[MESSAGE_REPEAT * strlen(MESSAGE) + 1];
        memset(expected_message, 0, sizeof(expected_message));
        for (int i = 0; i < MESSAGE_REPEAT; i++) {
            strcat(expected_message, MESSAGE);
        }

        if (strcmp(buffer, expected_message) == 0) {
            char response[MESSAGE_REPEAT * strlen(RESPONSE) + 1];
            memset(response, 0, sizeof(response));
            for (int i = 0; i < MESSAGE_REPEAT; i++) {
                strcat(response, RESPONSE);
            }

            send(new_socket, response, strlen(response), 0);
            get_timestamp(timestamp, sizeof(timestamp));
            printf("%s Sent response: `%.*s`...\n", timestamp, 5, response);
        } else {
            printf("Invalid message received.\n");
        }

        close(new_socket);
    }

    close(server_fd);
    return 0;
}