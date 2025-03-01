#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_BUFFER_SIZE 65535

int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char mode[20], *buffer;
    long total_bytes = 0, message_count = 0;
    int buffer_size;

    // Creare socket UDP
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configurare adresa server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Asociere socket-port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Serverul UDP asculta pe portul %d...\n", PORT);

    while (1) {
        printf("Astept date de la un client...\n");

        // Primire dimensiune buffer
        recvfrom(server_fd, &buffer_size, sizeof(buffer_size), 0, (struct sockaddr *)&client_addr, &client_len);
        if (buffer_size < 1 || buffer_size > MAX_BUFFER_SIZE) {
            printf("Dimensiune buffer invalida!\n");
            continue;
        }

        buffer = (char *)malloc(buffer_size);
        if (!buffer) {
            perror("Eroare la alocarea memoriei");
            continue;
        }

        // Primire mod de transfer de la client
        ssize_t bytes_received = recvfrom(server_fd, mode, sizeof(mode), 0,
                                         (struct sockaddr *)&client_addr, &client_len);
        if (bytes_received < 0) {
            perror("recvfrom failed");
            continue;
        }
        mode[bytes_received] = '\0'; // Null-terminate the string
        mode[strcspn(mode, "\n")] = 0; // Remove newline if present

        printf("Mod de transfer: %s | Buffer: %d bytes\n", mode, buffer_size);


        // Primire date de la client
        while (1) {
            bytes_received = recvfrom(server_fd, buffer, buffer_size, 0,
                                     (struct sockaddr *)&client_addr, &client_len);
            if (bytes_received <= 0) {
                perror("recvfrom failed");
                break;
            }

            if (strcmp(buffer, "FINAL_CONFIRMATION") == 0) {
                printf("Mesaj de confirmare finală primit.\n");
                sendto(server_fd, "ACK", 3, 0,
                       (struct sockaddr *)&client_addr, client_len);
                break;
            }

            total_bytes += bytes_received;
            message_count++;

            if (strcmp(mode, "STOP-AND-WAIT") == 0) {
                sendto(server_fd, "ACK", 3, 0,
                       (struct sockaddr *)&client_addr, client_len);
            }
        }

        printf("\n===== Statistici Transfer =====\n");
        printf("Protocol: UDP\n");
        printf("Mesaje primite: %ld\n", message_count);
        printf("Total bytes primiti: %ld\n", total_bytes);
        printf("===============================\n");

        printf("Client deconectat. Astept urmatorul client...\n\n");

        // Resetare contoare pentru următorul client
        total_bytes = 0;
        message_count = 0;
        free(buffer);
    }

    close(server_fd);
    return 0;
}