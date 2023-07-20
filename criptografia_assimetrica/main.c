#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#define PRIME_MIN_DIGITS 10000
#define PRIME_MAX_DIGITS 99999

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

void generatePrimesToFile(int count, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Erro ao abrir o arquivo.\n");
        return;
    }

    srand(time(NULL));

    for (int i = 0; i < count; ++i) {
        int p = generateRandomPrime(PRIME_MIN_DIGITS, PRIME_MAX_DIGITS);
        int q = generateRandomPrime(PRIME_MIN_DIGITS, PRIME_MAX_DIGITS);

        fprintf(file, "%d#%d\n", p, q);
    }

    fclose(file);
    printf("Números primos gerados e salvos no arquivo '%s'.\n", filename);
}

int generateRSAKeys(const char* primesFile, const char* publicKeyFile, const char* privateKeyFile) {
    FILE* file = fopen(primesFile, "r");
    if (file == NULL) {
        printf("Erro ao abrir o arquivo de números primos.\n");
        return 0;
    }

    char line[256];
    RSA* rsa = RSA_new();
    BIGNUM* pBN = BN_new();
    BIGNUM* qBN = BN_new();

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;

        char* pStr = strtok(line, "#");
        char* qStr = strtok(NULL, "#");

        if (pStr == NULL || qStr == NULL) {
            printf("Erro na leitura dos números primos do arquivo.\n");
            fclose(file);
            RSA_free(rsa);
            BN_free(pBN);
            BN_free(qBN);
            return 0;
        }

        int p = atoi(pStr);
        int q = atoi(qStr);

        // Gera as chaves RSA a partir dos números primos p e q
        BN_set_word(pBN, p);
        BN_set_word(qBN, q);

        if (RSA_generate_key_ex(rsa, 2048, pBN, qBN) != 1) {
            printf("Erro ao gerar as chaves RSA.\n");
            fclose(file);
            RSA_free(rsa);
            BN_free(pBN);
            BN_free(qBN);
            return 0;
        }

        FILE* publicKey = fopen(publicKeyFile, "w");
        if (publicKey == NULL) {
            printf("Erro ao abrir o arquivo de chave pública.\n");
            fclose(file);
            RSA_free(rsa);
            BN_free(pBN);
            BN_free(qBN);
            return 0;
        }

        FILE* privateKey = fopen(privateKeyFile, "w");
        if (privateKey == NULL) {
            printf("Erro ao abrir o arquivo de chave privada.\n");
            fclose(publicKey);
            fclose(file);
            RSA_free(rsa);
            BN_free(pBN);
            BN_free(qBN);
            return 0;
        }

        if (PEM_write_RSAPublicKey(publicKey, rsa) != 1) {
            printf("Erro ao escrever a chave pública.\n");
            fclose(publicKey);
            fclose(privateKey);
            fclose(file);
            RSA_free(rsa);
            BN_free(pBN);
            BN_free(qBN);
            return 0;
        }

        if (PEM_write_RSAPrivateKey(privateKey, rsa, NULL, NULL, 0, NULL, NULL) != 1) {
            printf("Erro ao escrever a chave privada.\n");
            fclose(publicKey);
            fclose(privateKey);
            fclose(file);
            RSA_free(rsa);
            BN_free(pBN);
            BN_free(qBN);
            return 0;
        }

        fclose(publicKey);
        fclose(privateKey);
    }

    fclose(file);
    RSA_free(rsa);
    BN_free(pBN);
    BN_free(qBN);
    printf("Chaves pública e privada geradas e salvas nos arquivos '%s' e '%s'.\n", publicKeyFile, privateKeyFile);
    return 1;
}

void encryptBMP(const char* inputFile, const char* outputFile, const char* publicKeyFile) {
    FILE* inputFilePtr = fopen(inputFile, "rb");
    if (inputFilePtr == NULL) {
        printf("Erro ao abrir o arquivo de entrada BMP.\n");
        return;
    }

    FILE* outputFilePtr = fopen(outputFile, "wb");
    if (outputFilePtr == NULL) {
        printf("Erro ao abrir o arquivo de saída BMP encriptado.\n");
        fclose(inputFilePtr);
        return;
    }

    FILE* publicKeyFilePtr = fopen(publicKeyFile, "r");
    if (publicKeyFilePtr == NULL) {
        printf("Erro ao abrir o arquivo de chave pública.\n");
        fclose(inputFilePtr);
        fclose(outputFilePtr);
        return;
    }

    RSA* rsa = RSA_new();
    PEM_read_RSAPublicKey(publicKeyFilePtr, &rsa, NULL, NULL);

    int bufferSize = RSA_size(rsa);
    unsigned char* buffer = (unsigned char*)malloc(bufferSize);

    int bytesRead = 0;
    while ((bytesRead = fread(buffer, sizeof(unsigned char), bufferSize, inputFilePtr)) > 0) {
        int encryptedSize = RSA_public_encrypt(bytesRead, buffer, buffer, rsa, RSA_PKCS1_OAEP_PADDING);
        fwrite(buffer, sizeof(unsigned char), encryptedSize, outputFilePtr);
    }

    free(buffer);
    fclose(inputFilePtr);
    fclose(outputFilePtr);
    fclose(publicKeyFilePtr);
    RSA_free(rsa);
    printf("Arquivo BMP encriptado salvo em '%s'.\n", outputFile);
}

void decryptBMP(const char* inputFile, const char* outputFile, const char* privateKeyFile) {
    FILE* inputFilePtr = fopen(inputFile, "rb");
    if (inputFilePtr == NULL) {
        printf("Erro ao abrir o arquivo BMP encriptado.\n");
        return;
    }

    FILE* outputFilePtr = fopen(outputFile, "wb");
    if (outputFilePtr == NULL) {
        printf("Erro ao abrir o arquivo BMP desencriptado.\n");
        fclose(inputFilePtr);
        return;
    }

    FILE* privateKeyFilePtr = fopen(privateKeyFile, "r");
    if (privateKeyFilePtr == NULL) {
        printf("Erro ao abrir o arquivo de chave privada.\n");
        fclose(inputFilePtr);
        fclose(outputFilePtr);
        return;
    }

    RSA* rsa = RSA_new();
    PEM_read_RSAPrivateKey(privateKeyFilePtr, &rsa, NULL, NULL);

    int bufferSize = RSA_size(rsa);
    unsigned char* buffer = (unsigned char*)malloc(bufferSize);

    int bytesRead = 0;
    while ((bytesRead = fread(buffer, sizeof(unsigned char), bufferSize, inputFilePtr)) > 0) {
        int decryptedSize = RSA_private_decrypt(bytesRead, buffer, buffer, rsa, RSA_PKCS1_OAEP_PADDING);
        fwrite(buffer, sizeof(unsigned char), decryptedSize, outputFilePtr);
    }

    free(buffer);
    fclose(inputFilePtr);
    fclose(outputFilePtr);
    fclose(privateKeyFilePtr);
    RSA_free(rsa);
    printf("Arquivo BMP desencriptado salvo em '%s'.\n", outputFile);
}

int main() {
    generatePrimesToFile(10, "primos.txt");
    generateRSAKeys("primos.txt", "chave.pub", "chave.priv");
    encryptBMP("fractaljulia.bmp", "arquivo_encriptado.bmp", "chave.pub");
    decryptBMP("arquivo_encriptado.bmp", "arquivo_desencriptado.bmp", "chave.priv");

    // falta dialogo TCP 

    return 0;
}
