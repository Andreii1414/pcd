#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void run_server(const char *protocol) {
    const char *server_program = NULL;

    if (strcmp(protocol, "tcp") == 0) {
        server_program = "./tcp_server";
    } else if (strcmp(protocol, "udp") == 0) {
        server_program = "./udp_server";
    } else if (strcmp(protocol, "quic") == 0) {
        server_program = "./quic_server";
    } else {
        fprintf(stderr, "Invalid protocol: %s\n", protocol);
        exit(EXIT_FAILURE);
    }

    execl(server_program, server_program, (char *)NULL);
    perror("execl failed");
    exit(EXIT_FAILURE);
}

void run_client(const char *protocol, const char *mode, const char *size_mb, const char *msg_size) {
    const char *client_program = NULL;

    if (strcmp(protocol, "tcp") == 0) {
        client_program = "./tcp_client";
    } else if (strcmp(protocol, "udp") == 0) {
        client_program = "./udp_client";
    } else if (strcmp(protocol, "quic") == 0) {
        client_program = "./quic_client";
    } else {
        fprintf(stderr, "Invalid protocol: %s\n", protocol);
        exit(EXIT_FAILURE);
    }

    execl(client_program, client_program, mode, size_mb, msg_size, (char *)NULL);
    perror("execl failed");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server|client> <tcp|udp|quic> [mode size_mb msg_size]\n", argv[0]);
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return EXIT_FAILURE;
    } else if (pid == 0) {
        if (strcmp(argv[1], "server") == 0) {
            run_server(argv[2]);
        } else if (strcmp(argv[1], "client") == 0) {
            if (argc < 6) {
                fprintf(stderr, "Usage for client: %s client <tcp|udp|quic> <streaming|stop-and-wait> <size_mb> <msg_size>\n", argv[0]);
                exit(EXIT_FAILURE);
            }
            run_client(argv[2], argv[3], argv[4], argv[5]);
        } else {
            fprintf(stderr, "Invalid role: %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    } else {
        wait(NULL);
    }

    return 0;
}
