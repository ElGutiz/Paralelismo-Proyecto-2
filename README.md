# Paralelismo-Proyecto-2

Para compilar y ejecutar el código es necesario de correr las siguientes lineas en la terminal.

```
mpicc bruteforce_file.c -o bruteforce -lmpi -lcrypto

mpirun -np 4 bruteforce 123456L prueba.txt
```
