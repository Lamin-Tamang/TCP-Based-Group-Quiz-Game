#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 12346

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

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int val = recv(sock, buffer, sizeof(buffer), 0);
        if (val <= 0) {
            printf("Disconnected from server.\n");
            break;
        }

        buffer[val] = '\0';

        // Check for end of quiz
        if (strncmp(buffer, "END|", 4) == 0) {
            printf("\n%s\n", buffer + 4);  // Skip "END|"
            break;
        }

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
