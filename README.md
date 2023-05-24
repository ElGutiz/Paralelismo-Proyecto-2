# Paralelismo-Proyecto-2

Para compilar y ejecutar el código es necesario de correr las siguientes lineas en la terminal.

```
mpicc bruteforce_file.c -o <nombre_ejecutable> -lmpi -lcrypto

mpirun -np 4 bruteforce <key> <archivo.txt>
```
Tambien es necesario el instalar las librearias siguientes:

```
sudo apt-get install libssl-dev

sudo apt-get install libtirpc-dev
```
