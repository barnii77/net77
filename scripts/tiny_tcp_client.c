#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 12345
#define MESSAGE "hello\r\n"
#define MESSAGE_REPEAT 100
#define RESPONSE "hi\r\n"

void get_timestamp(char *buffer, size_t len) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    snprintf(buffer, len, "[%ld]", (tv.tv_sec * 1000000L + tv.tv_usec));
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
    int sock;
    struct sockaddr_in server_addr;
    char message[MESSAGE_REPEAT * strlen(MESSAGE) + 1];
    char response[1024];
    char timestamp[32];
    int send_recv_timeout_usec = 50000;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (set_send_recv_timeout(sock, send_recv_timeout_usec)) {
        perror("Failed to set send/receive timeout");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Prepare the message
    memset(message, 0, sizeof(message));
    for (int i = 0; i < MESSAGE_REPEAT; i++) {
        strcat(message, MESSAGE);
    }

    get_timestamp(timestamp, sizeof(timestamp));
    printf("%s Sending message: `%s`...\n", timestamp, MESSAGE);

    // Send the message
    send(sock, message, strlen(message), 0);

    // Receive the response
    memset(response, 0, sizeof(response));
    recv(sock, response, sizeof(response), 0);

    get_timestamp(timestamp, sizeof(timestamp));
    printf("%s Received response: `%s`...\n", timestamp, RESPONSE);

    // Simulate delay
    usleep(10000);
    get_timestamp(timestamp, sizeof(timestamp));
    printf("Now it is %s\n", timestamp);

    // Verify the response
    char expected_response[MESSAGE_REPEAT * strlen(RESPONSE) + 1];
    memset(expected_response, 0, sizeof(expected_response));
    for (int i = 0; i < MESSAGE_REPEAT; i++) {
        strcat(expected_response, RESPONSE);
    }

    if (strcmp(response, expected_response) == 0) {
        printf("Test passed: Server responded correctly.\n");
    } else {
        printf("Test failed: Server response incorrect.\n");
    }

    close(sock);
    return 0;
}