#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/des.h>

#define PORT 8080
#define MAX_BUFFER_SIZE 1024
#define KEY_SIZE 8

void sendFile(const char* filename, const char* ip, int port, const unsigned char* key) {
    int sockfd;
    struct sockaddr_in server_addr;

    // Criação do socket TCP
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar o socket");
        exit(1);
    }

    // Configuração do endereço do servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    // Conexão com o servidor
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao conectar-se ao servidor");
        exit(1);
    }

    // Envio da chave DES
    if (send(sockfd, key, KEY_SIZE, 0) < 0) {
        perror("Erro ao enviar a chave DES");
        exit(1);
    }

    // Abertura do arquivo para leitura
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo para leitura");
        exit(1);
    }

    // Envio do arquivo para o servidor
    unsigned char buffer[MAX_BUFFER_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(sockfd, buffer, bytesRead, 0) < 0) {
            perror("Erro ao enviar o arquivo");
            exit(1);
        }
    }

    fclose(file);
    close(sockfd);
    printf("Arquivo enviado com sucesso!\n");
}

int main() {
    const char* filename = "fractaljulia.bmp";
    const char* ip = "127.0.0.1"; 
    int port = 8080;
    unsigned char desKey[KEY_SIZE] = {'k', 'e', 'y', '1', '2', '3', '4', '5'};

    sendFile(filename, ip, port, desKey);

    return 0;
}
