#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 12343
#define MAX_CLIENTS 10
#define MAX_Q_LEN 256
#define MAX_OPT_LEN 100
#define MAX_RETRIES 3

typedef struct {
    int socket;
    char name[50];
    int score;
    int active;
    int question_index;
    int retries;
} Client;

typedef struct {
    char question[MAX_Q_LEN];
    char options[4][MAX_OPT_LEN];
    int correct_index;
} Quiz;

Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t lock;

Quiz questions[] = {
    {"Who was the first elected Prime Minister of Nepal?", {"King Tribhuvan", "B. P. Koirala", "Matrika Prasad Koirala", "Tanka Prasad Acharya"}, 1},
    {"Which article of the Nepal Constitution defines fundamental rights?", {"Article 17", "Article 18", "Article 16", "Article 19"}, 0},
    {"Which is the deepest lake in Nepal?", {"Rara Lake", "Shey Phoksundo Lake", "Tilicho Lake", "Phewa Lake"}, 1},
    {"When was the Gorkha Kingdom unified to form modern Nepal?", {"1743 AD", "1768 AD", "1795 AD", "1801 AD"}, 1},
    {"Which national park of Nepal is listed as a UNESCO World Heritage Site?", {"Makalu-Barun National Park", "Shivapuri-Nagarjun National Park", "Sagarmatha National Park", "Banke National Park"}, 2}
};

int total_questions = sizeof(questions) / sizeof(questions[0]);

void send_question(int idx) {
    char buffer[1024];
    int q = clients[idx].question_index;

    if (q >= total_questions) {
        snprintf(buffer, sizeof(buffer), "END|Quiz Over! Final Score: %d\n", clients[idx].score);
        send(clients[idx].socket, buffer, strlen(buffer), 0);

        clients[idx].retries++;
        if (clients[idx].retries < MAX_RETRIES) {
            char retry_msg[128];
            int remaining = MAX_RETRIES - clients[idx].retries;
            snprintf(retry_msg, sizeof(retry_msg), "RETRY|You can retry %d more time(s). Send 'R' to restart or 'Q' to quit.\n", remaining);
            send(clients[idx].socket, retry_msg, strlen(retry_msg), 0);
        } else {
            clients[idx].active = 0;
        }
        return;
    }

    snprintf(buffer, sizeof(buffer), "Q%d: %s\nA. %s\nB. %s\nC. %s\nD. %s\n",
             q + 1,
             questions[q].question,
             questions[q].options[0],
             questions[q].options[1],
             questions[q].options[2],
             questions[q].options[3]);

    send(clients[idx].socket, buffer, strlen(buffer), 0);
}

void *client_handler(void *arg) {
    int idx = *(int *)arg;
    free(arg);

    clients[idx].retries = 0;
    clients[idx].active = 1;

    send_question(idx);

    char buffer[1024];

    while (clients[idx].active) {
        memset(buffer, 0, sizeof(buffer));
        int val = recv(clients[idx].socket, buffer, sizeof(buffer), 0);
        if (val <= 0) {
            printf("Client %s disconnected.\n", clients[idx].name);
            clients[idx].active = 0;
            break;
        }

        buffer[val] = '\0';

        if (clients[idx].retries > 0 && clients[idx].retries < MAX_RETRIES) {
            if (toupper(buffer[0]) == 'R') {
                clients[idx].score = 0;
                clients[idx].question_index = 0;
                printf("Client %s chose to retry the quiz.\n", clients[idx].name);
                send_question(idx);
                continue;
            } else if (toupper(buffer[0]) == 'Q') {
                printf("Client %s chose to quit.\n", clients[idx].name);
                clients[idx].active = 0;
                break;
            }
        }

        if (!clients[idx].active) break;

        char ans = toupper(buffer[0]);
        int q = clients[idx].question_index;

        if (ans >= 'A' && ans <= 'D') {
            int chosen = ans - 'A';
            if (chosen == questions[q].correct_index) {
                clients[idx].score++;
            }
            clients[idx].question_index++;
            send_question(idx);
        }
    }

    close(clients[idx].socket);
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    pthread_mutex_init(&lock, NULL);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d...\n", PORT);

    while (1) {
        int new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0) continue;

        char name[50];
        int recv_len = recv(new_socket, name, sizeof(name) - 1, 0);
        if (recv_len <= 0) {
            close(new_socket);
            continue;
        }
        name[recv_len] = '\0';

        pthread_mutex_lock(&lock);
        if (client_count >= MAX_CLIENTS) {
            pthread_mutex_unlock(&lock);
            char *msg = "Server full. Try again later.\n";
            send(new_socket, msg, strlen(msg), 0);
            close(new_socket);
            continue;
        }

        clients[client_count].socket = new_socket;
        strncpy(clients[client_count].name, name, sizeof(clients[client_count].name) - 1);
        clients[client_count].score = 0;
        clients[client_count].active = 1;
        clients[client_count].question_index = 0;

        int *idx = malloc(sizeof(int));
        *idx = client_count;
        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, idx);
        pthread_detach(tid);

        printf("Client %s connected.\n", clients[client_count].name);
        client_count++;
        pthread_mutex_unlock(&lock);
    }

    close(server_fd);
    return 0;
}
