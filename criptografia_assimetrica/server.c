#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#pragma pack(1)  // Empacotamento de 1 byte
#define PRIME_MIN_DIGITS 10000
#define PRIME_MAX_DIGITS 99999

#define PORT 8080
#define HEADER_SIZE 54
#define MAX_BUFFER_SIZE 1024

void save_bmp_file(char *header, char *buffer, int size) {
    FILE *file = fopen("received_image.bmp", "wb");
    if (file != NULL) {
        fwrite(header, sizeof(char), HEADER_SIZE, file);
        fwrite(buffer, sizeof(char), size, file);
        fclose(file);
        printf("Arquivo .bmp recebido e salvo com sucesso!\n");
    } else {
        perror("Erro ao salvar o arquivo");
    }
}

typedef struct {
    unsigned char type[2];
    unsigned int size;
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned int offset;
} BMPHeader;

typedef struct {
    unsigned int size;
    int width;
    int height;
    unsigned short planes;
    unsigned short bpp;
    unsigned int compression;
    unsigned int imageSize;
    int xResolution;
    int yResolution;
    unsigned int colors;
    unsigned int importantColors;
} BMPInfoHeader;

int isPrime(int num) {
    if (num < 2) {
        return 0;
    }

    for (int i = 2; i <= sqrt(num); i++) {
        if (num % i == 0) {
            return 0;
        }
    }

    return 1;
}

void readPrimesFromFile(const char* filename, int* p, int* q) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Erro ao abrir o arquivo %s.\n", filename);
        return;
    }

    fscanf(file, "%d#%d", p, q);
    fclose(file);
}

void saveToFile(const char* filename, int num) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Erro ao abrir o arquivo.\n");
        return;
    }

    fprintf(file, "%d", num);
    fclose(file);
}

int gcd(int a, int b) {
    if (b == 0) {
        return a;
    }

    return gcd(b, a % b);
}

int modInverse(int a, int m) {
    a = a % m;

    for (int x = 1; x < m; x++) {
        if ((a * x) % m == 1) {
            return x;
        }
    }

    return 1;
}

void createKeys(int p, int q) {
    int n = p * q;
    int phi = (p - 1) * (q - 1);

    int e;
    for (e = 2; e < phi; e++) {
        if (gcd(e, phi) == 1) {
            break;
        }
    }

    int d = modInverse(e, phi);

    saveToFile("chave.pub", e);
    saveToFile("chave.priv", d);
}

void encryptFile(const char* inputFile, const char* outputFile, int key) {
    FILE* input = fopen(inputFile, "rb");
    FILE* output = fopen(outputFile, "wb");

    if (input == NULL || output == NULL) {
        printf("Erro ao abrir os arquivos.\n");
        return;
    }

    BMPHeader bmpHeader;
    BMPInfoHeader bmpInfoHeader;

    fread(&bmpHeader, sizeof(BMPHeader), 1, input);
    fread(&bmpInfoHeader, sizeof(BMPInfoHeader), 1, input);

    fwrite(&bmpHeader, sizeof(BMPHeader), 1, output);
    fwrite(&bmpInfoHeader, sizeof(BMPInfoHeader), 1, output);

    unsigned char pixel;
    while (fread(&pixel, sizeof(unsigned char), 1, input) == 1) {
        pixel = pixel ^ key;
        fwrite(&pixel, sizeof(unsigned char), 1, output);
    }

    fclose(input);
    fclose(output);
    printf("Arquivo salvo: %s\n", outputFile);
}

void decryptFile(const char* inputFile, const char* outputFile, int key) {
    encryptFile(inputFile, outputFile, key); // A desencriptação é o mesmo processo da encriptação
}

int generateRandomPrime(int minDigits, int maxDigits) {
    int prime = 0;

    while (1) {
        prime = (rand() % (maxDigits - minDigits + 1)) + minDigits;

        int isPrime = 1;
        for (int i = 2; i <= prime / 2; ++i) {
            if (prime % i == 0) {
                isPrime = 0;
                break;
            }
        }

        if (isPrime) {
            break;
        }
    }

    return prime;
}

void generatePrimesToFile(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Erro ao abrir o arquivo.\n");
        return;
    }

    srand(time(NULL));

    int p = generateRandomPrime(PRIME_MIN_DIGITS, PRIME_MAX_DIGITS);
    int q = generateRandomPrime(PRIME_MIN_DIGITS, PRIME_MAX_DIGITS);

    fprintf(file, "%d#%d", p, q);

    fclose(file);
    printf("Números primos gerados e salvos no arquivo '%s'.\n", filename);
}

int main(int argc, char *argv[]){
    int p, q;
    generatePrimesToFile("primos.txt");
    readPrimesFromFile("primos.txt", &p, &q);
    createKeys(p, q);

    int fd =0, confd = 0,b,tot;
    struct sockaddr_in serv_addr;

    char buff[1025];
    int num;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    printf("Socket created\n");

    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(buff, '0', sizeof(buff));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000);

    bind(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    listen(fd, 10);

    while(1){
        confd = accept(fd, (struct sockaddr*)NULL, NULL);
        if (confd==-1) {
            perror("Accept");
            continue;
        }
        FILE* fp = fopen( "fractal_received.bmp", "wb");
        tot=0;
        if(fp != NULL){
            while( (b = recv(confd, buff, 1024,0))> 0 ) {
                tot+=b;
                fwrite(buff, 1, b, fp);
            }

            printf("Received byte: %d\n",tot);
            if (b<0)
               perror("Receiving");

            fclose(fp);
        } else {
            perror("File");
        }
        close(confd);
        encryptFile("fractal_received.bmp", "encrypted.bmp", p * q); // Use a chave pública adequada
        decryptFile("encrypted.bmp", "decrypted.bmp", p * q); // Use a chave privada adequada
    }

    return 0;
}
