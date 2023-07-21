# Trabalho Final - Criptografia

## Criptografia Simétrica

```bash
sudo apt-get install libssl-dev
gcc bob_server.c -o bob_server -lssl -lcrypto
./bob_server
gcc alice_client.c  -o alice_client -lssl -lcrypto
./alice_client
```


## Criptografia Assimétrica

```bash
gcc -o server server.c -lm
gcc -o client client.c
./server
./client
```
