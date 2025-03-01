#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_BUFFER_SIZE 65535

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char mode[20];
    long total_bytes = 0, message_count = 0;
    int buffer_size;

    // Creare socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Configurare adresa server
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Asociere socket-port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // "Ascultare" conexiuni
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Serverul TCP asculta pe portul %d...\n", PORT);

    while (1) {
        printf("Astept conexiune de la un client...\n");

        // Acceptare conexiune de la client
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client conectat!\n");

        recv(new_socket, &buffer_size, sizeof(buffer_size), 0);
        if (buffer_size < 1 || buffer_size > MAX_BUFFER_SIZE) {
            printf("Dimensiune buffer invalida!\n");
            close(new_socket);
            continue;
        }
        printf("Dimensiune buffer primita: %d bytes\n", buffer_size);
        
        char *buffer = (char *)malloc(buffer_size);
        if (!buffer) {
            perror("Eroare alocare memorie");
            close(new_socket);
            continue;
        }

        recv(new_socket, mode, sizeof(mode), 0);
        mode[strcspn(mode, "\n")] = 0; 

        printf("Mod de transfer: %s\n", mode);

        // Primire date de la client
        ssize_t bytes_received;
        while (1) {

            bytes_received = recv(new_socket, buffer, buffer_size, 0);
            if (bytes_received <= 0) break;
        
            total_bytes += bytes_received;
            message_count++;
        
            if (strcmp(mode, "STOP-AND-WAIT") == 0) {
                send(new_socket, "ACK", 3, 0);
            }
        }

        if (strcmp(buffer, "END") == 0) {
            printf("Transfer complet.\n");
        }

        printf("\n===== Statistici Transfer =====\n");
        printf("Protocol: TCP\n");
        printf("Mesaje primite: %ld\n", message_count);
        printf("Total bytes primiti: %ld\n", total_bytes);
        printf("===============================\n");

        total_bytes = 0;
        message_count = 0;
        free(buffer);
        close(new_socket);
        printf("Client deconectat. Astept urmatorul client...\n\n");
    }

    close(new_socket);
    close(server_fd);

    return 0;
}
