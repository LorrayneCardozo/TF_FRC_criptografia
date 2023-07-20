#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/des.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 8080
#define MAX_BUFFER_SIZE 1024
#define KEY_SIZE 8
#define BMP_HEADER_SIZE 54

// Função para receber o arquivo do cliente
void receiveFile(int sockfd, const char* filename) {
    // Definir o nome do arquivo para salvar o arquivo recebido com o nome "fractaljulia_received.bmp"
    const char* receivedFilename = "fractaljulia_received.bmp";

    FILE* file = fopen(receivedFilename, "wb");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo para escrita");
        exit(1);
    }

    unsigned char buffer[MAX_BUFFER_SIZE];
    int bytesRead;
    while ((bytesRead = read(sockfd, buffer, sizeof(buffer))) > 0) {
        fwrite(buffer, 1, bytesRead, file);
    }
    if (bytesRead < 0) {
        perror("Erro ao receber o arquivo");
        exit(1);
    }

    fclose(file);
    printf("Arquivo recebido com sucesso: %s\n", receivedFilename);
}

void encryptFile(const char* inputFilename, const char* outputFilename, const unsigned char* key) {
    FILE* inputFile = fopen(inputFilename, "rb");
    if (inputFile == NULL) {
        perror("Erro ao abrir o arquivo para leitura");
        exit(1);
    }

    FILE* outputFile = fopen(outputFilename, "wb");
    if (outputFile == NULL) {
        perror("Erro ao abrir o arquivo para escrita");
        fclose(inputFile);
        exit(1);
    }

    DES_cblock desKey;
    memcpy(desKey, key, sizeof(desKey));

    DES_key_schedule keySchedule;
    DES_set_key(&desKey, &keySchedule);

    unsigned char inputBuffer[MAX_BUFFER_SIZE];
    unsigned char outputBuffer[MAX_BUFFER_SIZE];

    // Copiar o cabeçalho BMP para o arquivo de saída
    fseek(inputFile, 0, SEEK_SET);
    fread(inputBuffer, 1, BMP_HEADER_SIZE, inputFile);
    fwrite(inputBuffer, 1, BMP_HEADER_SIZE, outputFile);

    // Inicializar o vetor de inicialização para CBC
    DES_cblock ivec;
    memset(&ivec, 0, sizeof(ivec));

    size_t bytesRead;
    while ((bytesRead = fread(inputBuffer, 1, sizeof(inputBuffer), inputFile)) > 0) {
        // Criptografar usando CBC (Cipher Block Chaining)
        DES_ncbc_encrypt(inputBuffer, outputBuffer, bytesRead, &keySchedule, &ivec, DES_ENCRYPT);
        fwrite(outputBuffer, 1, bytesRead, outputFile);
    }

    fclose(inputFile);
    fclose(outputFile);
    printf("Arquivo encriptografado salvo como: %s\n", outputFilename);
}

void decryptFile(const char* inputFilename, const char* outputFilename, const unsigned char* key) {
    FILE* inputFile = fopen(inputFilename, "rb");
    if (inputFile == NULL) {
        perror("Erro ao abrir o arquivo para leitura");
        exit(1);
    }

    FILE* outputFile = fopen(outputFilename, "wb");
    if (outputFile == NULL) {
        perror("Erro ao abrir o arquivo para escrita");
        fclose(inputFile);
        exit(1);
    }

    DES_cblock desKey;
    memcpy(desKey, key, sizeof(desKey));

    DES_key_schedule keySchedule;
    DES_set_key(&desKey, &keySchedule);

    unsigned char inputBuffer[MAX_BUFFER_SIZE];
    unsigned char outputBuffer[MAX_BUFFER_SIZE];

    // Ler e copiar o cabeçalho BMP para o arquivo de saída
    fseek(inputFile, 0, SEEK_SET);
    fread(outputBuffer, 1, BMP_HEADER_SIZE, inputFile);
    fwrite(outputBuffer, 1, BMP_HEADER_SIZE, outputFile);

    // Inicializar o vetor de inicialização para CBC
    DES_cblock ivec;
    memset(&ivec, 0, sizeof(ivec));

    size_t bytesRead;
    while ((bytesRead = fread(inputBuffer, 1, sizeof(inputBuffer), inputFile)) > 0) {
        // Descriptografar usando CBC (Cipher Block Chaining)
        DES_ncbc_encrypt(inputBuffer, outputBuffer, bytesRead, &keySchedule, &ivec, DES_DECRYPT);
        fwrite(outputBuffer, 1, bytesRead, outputFile);
    }

    fclose(inputFile);
    fclose(outputFile);
    printf("Arquivo desencriptado salvo como: %s\n", outputFilename);
}

int main() {
    int sockfd, newsockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    unsigned char desKey[KEY_SIZE];
    unsigned char buffer[MAX_BUFFER_SIZE];

    // Criação do socket TCP
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar o socket");
        exit(1);
    }
    
    // Configuração do endereço do servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Reutilização do endereço
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("Erro ao configurar a opção SO_REUSEADDR");
        exit(1);
    }

    // Associação do socket com o endereço do servidor
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao associar o socket com o endereço do servidor");
        exit(1);
    }

    // Aguardar por conexões
    listen(sockfd, 5);
    printf("Aguardando por conexões...\n");

    // Aceitar conexões
    client_len = sizeof(client_addr);
    newsockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
    if (newsockfd < 0) {
        perror("Erro ao aceitar a conexão");
        exit(1);
    }
    printf("Conexão estabelecida com o cliente.\n");

    // Recebimento da chave DES
    if (recv(newsockfd, desKey, KEY_SIZE, 0) < 0) {
        perror("Erro ao receber a chave DES");
        exit(1);
    }

    // Recebimento do arquivo "fractaljulia.bmp"
    const char* receivedFilename = "fractaljulia_received.bmp";
    receiveFile(newsockfd, receivedFilename);
    close(newsockfd);
    close(sockfd);
    printf("Arquivo 'fractaljulia.bmp' recebido e salvo no servidor!\n");

    // Criptografia do arquivo recebido
    const char* encryptedFilename = "fractaljulia_encriptado.bmp";
    encryptFile(receivedFilename, encryptedFilename, desKey);

    // Desencriptação do arquivo
    const char* decryptedFilename = "fractaljulia_desencriptado.bmp";
    decryptFile(encryptedFilename, decryptedFilename, desKey);

    return 0;
}
