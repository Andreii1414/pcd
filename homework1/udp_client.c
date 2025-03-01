#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>


#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define MAX_BUFFER_SIZE 65535
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
    if (buffer_size < 1 || buffer_size > MAX_BUFFER_SIZE) {
        printf("Dimensiune buffer invalida! Introduceti o valoare intre 1 si 65535 bytes.\n");
        return 1;
    }
    DATA_SIZE = size_in_MB * 1024 * 1024;

    int sock;
    struct sockaddr_in server_addr;
    char *buffer = (char *)malloc(buffer_size);
    char ack[4], mode[20];
    long total_bytes = 0, message_count = 0;
    time_t start_time, end_time;

    // Umplem bufferul cu date fictive
    memset(buffer, 'A', buffer_size);

    // Creare socket UDP
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configurare adresa server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    // Trimite modul de transfer si dimensiunea buffer-ului
    sendto(sock, &buffer_size, sizeof(buffer_size), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (strcmp(argv[1], "streaming") == 0) {
        strcpy(mode, "STREAMING");
    } else if (strcmp(argv[1], "stop-and-wait") == 0) {
        strcpy(mode, "STOP-AND-WAIT");
    } else {
        printf("Mod invalid! Folositi 'streaming' sau 'stop-and-wait'.\n");
        return 1;
    }
    sendto(sock, mode, strlen(mode) + 1, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("Conectat la server. Incepem transferul de %d MB...\n", DATA_SIZE / (1024 * 1024));

    time(&start_time);

    // Trimitere buffer
    while (total_bytes < DATA_SIZE) {
        ssize_t bytes_sent = sendto(sock, buffer, buffer_size, 0,
                                   (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (bytes_sent <= 0) {
            perror("Send failed");
            break;
        }
        total_bytes += bytes_sent;
        message_count++;

        // Stop-and-Wait: Asteapta ACK dupa fiecare bloc
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
                
                sendto(sock, buffer, buffer_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                retries++;
            }
            
            if (retries == MAX_RETRIES) {
                printf("Eroare: Serverul nu a răspuns corect cu ACK. Oprire transfer.\n");
                break;
            }
        }
    }

    // Trimite mesajul de confirmare finala și așteapta ACK
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
            printf("Nu s-a primit ACK. Reincercare %d/%d...\n", retries, MAX_RETRIES);
        }
    }

    if (retries == MAX_RETRIES) {
        printf("Eroare: Nu s-a putut confirma finalizarea transferului cu serverul.\n");
    }

    time(&end_time);
    double total_time = difftime(end_time, start_time);

    printf("\n===== Statistici Transfer =====\n");
    printf("Protocol: UDP\n");
    printf("Mod de transfer: %s\n", mode);
    printf("Mesaje trimise: %ld\n", message_count);
    printf("Total bytes trimisi: %ld\n", total_bytes);
    printf("Timp total: %.2f secunde\n", total_time);
    printf("===============================\n");

    free(buffer);
    close(sock);

    return 0;
}