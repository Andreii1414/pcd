#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <ngtcp2/ngtcp2.h>

#define SERVER_IP "::1"
#define PORT 8080
#define MAX_RETRIES 10
#define ACK_TIMEOUT 1
long DATA_SIZE;

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Utilizare: %s <streaming|stop-and-wait> <500|1024> <buffer_size>\n", argv[0]);
        return 1;
    }

    int size_in_MB = atoi(argv[2]);
    if (size_in_MB != 500 && size_in_MB != 1024) {
        printf("Dimensiune invalida! Alegeti intre 500MB si 1024MB.\n");
        return 1;
    }

    int buffer_size = atoi(argv[3]);
    if (buffer_size <= 0) {
        printf("Dimensiune buffer invalida!\n");
        return 1;
    }

    DATA_SIZE = size_in_MB * 1024 * 1024;

    int sock;
    struct sockaddr_in6 server_addr;
    char *buffer, ack[4], mode[20];
    long total_bytes = 0, message_count = 0;
    time_t start_time, end_time;

    buffer = (char *)malloc(buffer_size);
    if (!buffer) {
        perror("Eroare la alocarea memoriei");
        exit(EXIT_FAILURE);
    }
    memset(buffer, 'A', buffer_size);

    // Creare socket UDP pentru QUIC
    if ((sock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(PORT);
    if (inet_pton(AF_INET6, SERVER_IP, &server_addr.sin6_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "streaming") == 0) {
        strcpy(mode, "STREAMING");
    } else if (strcmp(argv[1], "stop-and-wait") == 0) {
        strcpy(mode, "STOP-AND-WAIT");
    } else {
        printf("Mod invalid! Folositi 'streaming' sau 'stop-and-wait'.\n");
        return 1;
    }

    sendto(sock, mode, strlen(mode) + 1, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

     // Trimiterea dimensiunii bufferului
     sendto(sock, &buffer_size, sizeof(buffer_size), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("Conectat la server. Incepem transferul de %d MB...\n", DATA_SIZE / (1024 * 1024));

    time(&start_time);

    while (total_bytes < DATA_SIZE) {
        ssize_t bytes_sent = sendto(sock, buffer, buffer_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (bytes_sent <= 0) {
            perror("Send failed");
            break;
        }
        total_bytes += bytes_sent;
        message_count++;

        if (strcmp(mode, "STOP-AND-WAIT") == 0) {
            int retries = 0;
            while (retries < MAX_RETRIES) {
                struct timeval timeout;
                timeout.tv_sec = ACK_TIMEOUT;
                timeout.tv_usec = 0;
                setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
                socklen_t server_len = sizeof(server_addr);
                ssize_t ack_bytes = recvfrom(sock, ack, sizeof(ack) - 1, 0, (struct sockaddr *)&server_addr, &server_len);
                
                if (ack_bytes > 0) {
                    ack[ack_bytes] = '\0'; 
                    
                    if (strcmp(ack, "ACK") == 0) {
                        break;
                    } else {
                        printf("ACK invalid primit: %s. Retransmitere...\n", ack);
                    }
                } else {
                    printf("Timeout la primirea ACK-ului. Retransmitere...\n");
                }
        
                // Retransmitere mesaj
                sendto(sock, buffer, buffer_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                retries++;
            }
        
            if (retries == MAX_RETRIES) {
                printf("Eroare: Serverul nu a răspuns corect cu ACK. Oprire transfer.\n");
                break;
            }
        }
        
    }

    // Trimite mesajul de confirmare finala si asteapta ACK
    const char *final_confirmation = "FINAL_CONFIRMATION";
    int retries = 0;
    while (retries < MAX_RETRIES) {
        sendto(sock, final_confirmation, strlen(final_confirmation) + 1, 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr));

        // Setam un timeout pentru ACK
        struct timeval timeout;
        timeout.tv_sec = ACK_TIMEOUT;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        socklen_t server_len = sizeof(server_addr);
        ssize_t ack_bytes = recvfrom(sock, ack, 3, 0, (struct sockaddr *)&server_addr, &server_len);
        if (ack_bytes > 0 && strcmp(ack, "ACK") == 0) {
            printf("Confirmare finala primita de la server.\n");
            break;
        } else {
            retries++;
        }
    }

    if (retries == MAX_RETRIES) {
        printf("Eroare: Nu s-a putut confirma finalizarea transferului cu serverul.\n");
    }


    time(&end_time);
    double total_time = difftime(end_time, start_time);

    printf("\n===== Statistici Transfer =====\n");
    printf("Protocol: QUIC (ngtcp2)\n");
    printf("Mod de transfer: %s\n", mode);
    printf("Mesaje trimise: %ld\n", message_count);
    printf("Total bytes trimisi: %ld\n", total_bytes);
    printf("Timp total: %.2f secunde\n", total_time);
    printf("===============================\n");
    
    free(buffer);
    close(sock);
    return 0;
}
