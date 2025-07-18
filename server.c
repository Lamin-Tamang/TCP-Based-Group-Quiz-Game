#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 12322
#define MAX_CLIENTS 10
#define MIN_GROUP 2
#define MAX_Q_LEN 256
#define MAX_OPT_LEN 100
#define MAX_RETRIES 3

typedef struct {
    int socket;
    char name[50];
    int score;
    int active;
    int answer_received;
    char current_answer;
} Client;

typedef struct {
    char question[MAX_Q_LEN];
    char options[4][MAX_OPT_LEN];
    int correct_index;
} Quiz;

Client clients[MAX_CLIENTS];
int client_count = 0;
int group_question_index = 0;
int group_retries = 0;
int answers_collected = 0;
pthread_mutex_t lock;

Quiz questions[] = {
    {"Who was the first elected Prime Minister of Nepal?", {"King Tribhuvan", "B. P. Koirala", "Matrika Prasad Koirala", "Tanka Prasad Acharya"}, 1},
    {"Which article of the Nepal Constitution defines fundamental rights?", {"Article 17", "Article 18", "Article 16", "Article 19"}, 0},
    {"Which is the deepest lake in Nepal?", {"Rara Lake", "Shey Phoksundo Lake", "Tilicho Lake", "Phewa Lake"}, 1},
    {"When was the Gorkha Kingdom unified to form modern Nepal?", {"1743 AD", "1768 AD", "1795 AD", "1801 AD"}, 1},
    {"Which national park of Nepal is listed as a UNESCO World Heritage Site?", {"Makalu-Barun National Park", "Shivapuri-Nagarjun National Park", "Sagarmatha National Park", "Banke National Park"}, 2}
};

int total_questions = sizeof(questions) / sizeof(questions[0]);

void send_to_client(int idx, const char *msg) {
    send(clients[idx].socket, msg, strlen(msg), 0);
}

void send_question_to_all() {
    char buffer[1024];
    if (group_question_index >= total_questions) {
        snprintf(buffer, sizeof(buffer), "END|Quiz Over! Final Scores:\n");
        for (int i = 0; i < client_count; i++) {
            char score_line[128];
            snprintf(score_line, sizeof(score_line), "%s: %d\n", clients[i].name, clients[i].score);
            strcat(buffer, score_line);
        }
        for (int i = 0; i < client_count; i++) {
            send_to_client(i, buffer);
            if (group_retries < MAX_RETRIES) {
                char retry_msg[128];
                int remaining = MAX_RETRIES - group_retries;
                snprintf(retry_msg, sizeof(retry_msg), "RETRY|You can retry %d more time(s). Send 'R' to restart or 'Q' to quit.\n", remaining);
                send_to_client(i, retry_msg);
            } else {
                clients[i].active = 0;
            }
        }
        return;
    }
    snprintf(buffer, sizeof(buffer), "Q%d: %s\nA. %s\nB. %s\nC. %s\nD. %s\n",
             group_question_index + 1,
             questions[group_question_index].question,
             questions[group_question_index].options[0],
             questions[group_question_index].options[1],
             questions[group_question_index].options[2],
             questions[group_question_index].options[3]);
    for (int i = 0; i < client_count; i++) {
        send_to_client(i, buffer);
        clients[i].answer_received = 0;
        clients[i].current_answer = 0;
    }
    answers_collected = 0;
}

void reset_group_quiz() {
    group_question_index = 0;
    group_retries++;
    for (int i = 0; i < client_count; i++) {
        clients[i].score = 0;
        clients[i].active = 1;
        clients[i].answer_received = 0;
        clients[i].current_answer = 0;
    }
}

void *client_handler(void *arg) {
    int idx = *(int *)arg;
    free(arg);

    pthread_mutex_lock(&lock);
    pthread_mutex_unlock(&lock);

    while (clients[idx].active) {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        int val = recv(clients[idx].socket, buffer, sizeof(buffer), 0);
        if (val <= 0) {
            pthread_mutex_lock(&lock);
            printf("Client %s disconnected.\n", clients[idx].name);
            clients[idx].active = 0;
            pthread_mutex_unlock(&lock);
            break;
        }
        buffer[val] = '\0';

        pthread_mutex_lock(&lock);

        if (group_question_index >= total_questions) {
            if (toupper(buffer[0]) == 'R' && group_retries < MAX_RETRIES) {
                reset_group_quiz();
                printf("Group chose to retry the quiz.\n");
                send_question_to_all();
                pthread_mutex_unlock(&lock);
                continue;
            } else if (toupper(buffer[0]) == 'Q') {
                printf("Group chose to quit the quiz.\n");
                for (int i = 0; i < client_count; i++) {
                    clients[i].active = 0;
                }
                pthread_mutex_unlock(&lock);
                break;
            }
            pthread_mutex_unlock(&lock);
            continue;
        }

        if (!clients[idx].answer_received) {
            char ans = toupper(buffer[0]);
            if (ans >= 'A' && ans <= 'D') {
                clients[idx].current_answer = ans;
                clients[idx].answer_received = 1;
                answers_collected++;
            }
        }

        if (answers_collected == client_count) {
            for (int i = 0; i < client_count; i++) {
                if (clients[i].current_answer == 'A' + questions[group_question_index].correct_index) {
                    clients[i].score++;
                }
            }
            group_question_index++;
            send_question_to_all();
        }

        pthread_mutex_unlock(&lock);
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
    if (server_fd < 0) exit(EXIT_FAILURE);

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) exit(EXIT_FAILURE);
    if (listen(server_fd, MAX_CLIENTS) < 0) exit(EXIT_FAILURE);

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
        clients[client_count].answer_received = 0;
        clients[client_count].current_answer = 0;

        int *idx = malloc(sizeof(int));
        *idx = client_count;
        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, idx);
        pthread_detach(tid);

        printf("Client %s connected.\n", clients[client_count].name);

        client_count++;

        if (client_count >= MIN_GROUP) {
            send_question_to_all();
        }

        pthread_mutex_unlock(&lock);
    }

    close(server_fd);
    return 0;
}
