#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 12343

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[1024];
    char name[50];
    char answer[5];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Enter your name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0;
    send(sock, name, strlen(name), 0);
    printf("Client has been connected.\n");

    int quiz_over = 0;
    int waiting_retry = 0;

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int val = recv(sock, buffer, sizeof(buffer), 0);
        if (val <= 0) {
            printf("Disconnected from server.\n");
            break;
        }

        buffer[val] = '\0';

        if (strncmp(buffer, "END|", 4) == 0) {
            printf("\n%s\n", buffer + 4);
            quiz_over = 1;
            continue;
        }

        if (strncmp(buffer, "RETRY|", 6) == 0) {
            printf("%s\n", buffer + 6);
            waiting_retry = 1;

            while (1) {
                printf("Enter your choice (R to retry, Q to quit): ");
                fgets(answer, sizeof(answer), stdin);
                answer[strcspn(answer, "\n")] = 0;

                if (strlen(answer) == 1 && (toupper(answer[0]) == 'R' || toupper(answer[0]) == 'Q')) {
                    send(sock, answer, 1, 0);
                    if (toupper(answer[0]) == 'R') {
                        waiting_retry = 0;
                        quiz_over = 0;
                    } else if (toupper(answer[0]) == 'Q') {
                        printf("Quitting...\n");
                        close(sock);
                        exit(0);
                    }
                    break;
                } else {
                    printf("Invalid input. Please enter 'R' or 'Q'.\n");
                }
            }
            continue;
        }

        if (quiz_over) continue;

        printf("\n%s\n", buffer);

        if (buffer[0] == 'Q') {
            printf("Your answer (A/B/C/D): ");
            fgets(answer, sizeof(answer), stdin);
            answer[strcspn(answer, "\n")] = 0;
            if (strlen(answer) == 0) strcpy(answer, "X");
            send(sock, answer, strlen(answer), 0);
        }
    }

    close(sock);
    return 0;
}
