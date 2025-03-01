#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ngtcp2/ngtcp2.h>

#define PORT 8080
#define DEFAULT_BUFFER_SIZE 1200
//gcc -o test_ngtcp2 test_ngtcp2.c -lngtcp2

int main() {
    int server_fd;
    struct sockaddr_in6 address;
    int addrlen = sizeof(address);
    char *buffer;
    int buffer_size = DEFAULT_BUFFER_SIZE;
    char mode[20];
    long total_bytes = 0, message_count = 0;

    // Creare socket UDP pentru QUIC
    if ((server_fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configurare adresa server
    memset(&address, 0, sizeof(address));
    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_any;
    address.sin6_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Serverul QUIC asculta pe portul %d...\n", PORT);

    while (1) {
        printf("Astept conexiune de la un client...\n");

        // Primire mod de transfer
        ssize_t recv_size = recvfrom(server_fd, mode, sizeof(mode), 0, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (recv_size <= 0) continue;

        mode[strcspn(mode, "\n")] = 0; 
        printf("Mod de transfer: %s\n", mode);

        // Primire dimensiune buffer
        recvfrom(server_fd, &buffer_size, sizeof(buffer_size), 0, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        printf("Buffer setat la %d bytes\n", buffer_size);

        buffer = (char *)malloc(buffer_size);
        if (!buffer) {
            perror("Eroare la alocarea memoriei");
            exit(EXIT_FAILURE);
        }

        total_bytes = 0;
        message_count = 0;

        while (1) {
            ssize_t bytes_received = recvfrom(server_fd, buffer, buffer_size, 0, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            if (bytes_received <= 0) break;

            buffer[bytes_received] = '\0';  
            if (strcmp(buffer, "FINAL_CONFIRMATION") == 0) {
                printf("Mesaj de confirmare finala primit.\n");
                sendto(server_fd, "ACK", 3, 0, (struct sockaddr *)&address, addrlen);
                break;
            }            

            total_bytes += bytes_received;
            message_count++;

            if (strcmp(mode, "STOP-AND-WAIT") == 0) {
                sendto(server_fd, "ACK", 3, 0, (struct sockaddr *)&address, addrlen);
            }
        }

        printf("\n===== Statistici Transfer =====\n");
        printf("Protocol: QUIC (ngtcp2)\n");
        printf("Mesaje primite: %ld\n", message_count);
        printf("Total bytes primiti: %ld\n", total_bytes);
        printf("===============================\n");

        printf("Client deconectat. Astept urmatorul client...\n\n");
    }

    close(server_fd);
    return 0;
}
