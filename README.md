# Trabalho Final - Criptografia

## Criptografia simétrica

```bash
sudo apt-get install libssl-dev
gcc bob_server.c -o bob_server -lssl -lcrypto
./bob_server
gcc alice_client.c  -o alice_client -lssl -lcrypto
./alice_client
```