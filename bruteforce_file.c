#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <time.h>
#include <openssl/des.h>

void decrypt(const_DES_cblock key, unsigned char *ciph, int len) {
    DES_key_schedule keySchedule;
    DES_set_key_unchecked(&key, &keySchedule);
    DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &keySchedule, DES_DECRYPT);
}

void encrypt(const_DES_cblock key, unsigned char *ciph, int len) {
    DES_key_schedule keySchedule;
    DES_set_key_unchecked(&key, &keySchedule);
    DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &keySchedule, DES_ENCRYPT);
}

char search[] = "es una prueba de";

int tryKey(const_DES_cblock key, unsigned char *ciph, int len) {
    unsigned char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = 0;
    decrypt(key, temp, len);
    return strstr((char *)temp, search) != NULL;
}

int main(int argc, char *argv[]) {
    // para almacenar el tiempo de ejecución del código
    double time_spent = 0.0;
 
    clock_t begin = clock();
    
    // Verificar que se proporcionen suficientes argumentos
    if (argc < 3) {
        printf("Uso: %s clave archivo_entrada\n", argv[0]);
        return 1;
    }
    
    long key = atol(argv[1]);
    
    // Leer el texto cifrado desde el archivo
    FILE *file_input = fopen(argv[2], "r");
    if (file_input == NULL) {
        printf("No se pudo abrir el archivo de entrada.\n");
        return 1;
    }
    
    fseek(file_input, 0, SEEK_END);
    long input_size = ftell(file_input);
    fseek(file_input, 0, SEEK_SET);
    unsigned char *input_text = (unsigned char *)malloc(input_size + 1);
    fread(input_text, 1, input_size, file_input);
    input_text[input_size] = '\0';
    fclose(file_input);
    
    unsigned char *cipher = input_text;
    int N, id;
    long upper = (1L << 56); //upper bound DES keys 2^56
    long mylower, myupper;
    MPI_Status st;
    MPI_Request req;
    int ciphlen = strlen(cipher);
    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    long range_per_node = upper / N;
    mylower = range_per_node * id;
    myupper = range_per_node * (id + 1) - 1;
    if (id == N - 1) {
        myupper = upper;
    }

    long found = 0;

    MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

    for (long i = mylower; i < myupper && (found == 0); ++i) {
        const_DES_cblock key;
        memcpy(key, &i, sizeof(key));
        if (tryKey(key, cipher, ciphlen)) {
            found = i;
            for (int node = 0; node < N; node++) {
                MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
            }
            break;
        }
        // Mensaje de depuración
        //printf("Proceso %d: Iteración %li\n", id, i);
    }
    free(input_text);
    
    if (id == 0) {
        MPI_Wait(&req, &st);
        const_DES_cblock key;
        memcpy(key, &found, sizeof(key));
        decrypt(key, cipher, ciphlen);
        printf("%li %s\n", found, cipher);
    }

    MPI_Finalize();
    clock_t end = clock();
    // calcula el tiempo transcurrido encontrando la diferencia (end - begin) y
    // dividiendo la diferencia por CLOCKS_PER_SEC para convertir a segundos
    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
 
    printf("The elapsed time is %f seconds", time_spent);
    
    return 0;
}
