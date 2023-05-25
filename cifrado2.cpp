#include <iostream>
#include <stdio.h>      // printf
#include <cstdlib>      // rand
#include <cmath>        // pow
#include <ctime>        // time_t, clock, difftime
#include <fstream>      // file processing
#include <mpi.h>

using namespace std;

// Variables globales --------------------------------------------------------------------------------------------------------------------------------------------------
char* PLAIN;
char KEY[16];
int SIZE = 16;
const int ROUNDS = 4;
char ALLKEYS[16*ROUNDS];
int MODULE;

void escribir(char chars[], int init) {
    ofstream archivo1;
    archivo1.open("cipher.bin", ios_base::app); // Abriendo archivo en modo escritura
    if (archivo1.fail()) {
        cout << "No se pudo abrir el archivo";
        exit(1);
    }
    for (int i = init; i < (SIZE + init); i++) {
        archivo1 << chars[i];
    }
    archivo1.close();
}

// Función para cifrar un bloque asignado
void cifrar(char* temp, int tID) {
    int cont;
    // Encriptar bloque asignado
    for (int i = 0; i < ROUNDS; i++) {
        cont = 0;
        for (int j = tID; j < (tID + SIZE); j++) {
            temp[j] = char((int)PLAIN[j] + (int)ALLKEYS[cont + (i * 16)]);
            cont++;
        }
    }
}

// Función para descifrar un bloque asignado
void descifrar(char* temp, int tID) {
    int cont;
    // Desencriptar bloque asignado
    for (int i = 0; i < ROUNDS; i++) {
        cont = 0;
        for (int j = tID; j < (tID + SIZE); j++) {
            temp[j] = char(int(PLAIN[j]) + int(ALLKEYS[cont - (i * 16)]));
            cont++;
        }
    }
}

// Funciones generales -------------------------------------------------------------------------------------------------------------------------------------------------
void rotateKeyCipher() {
    int shift = (int)(SIZE / ROUNDS);
    char temp[SIZE];
    for (int i = 0; i < SIZE; i++) {
        temp[i] = KEY[i];
    }
    for (int i = 0; i < SIZE - 4; i++) {
        KEY[i + 4] = temp[i];
    }
    for (int i = 0; i < 4; i++) {
        KEY[i] = temp[i + 12];
    }
}

void generateKey() {
    string my_key = "";
    srand(time(NULL));
    for (int i = 0; i < SIZE; i++) {
        // Unicamente seleccionar asciis imprimibles
        KEY[i] = (char)((rand() % (127 - 33)) + 33);
        // KEY[i] = (char)(rand()%128);
    }
    // escribir(KEY);
}

void encriptar() {
    // Generar llave aleatoria
    /*
    generateKey();
    cout<<"Llave: ";
    for(int i=0; i<SIZE; i++){
        cout<<KEY[i];
    }cout<<endl;
    */
    
    string my_key;
    cout << "Ingrese la llave (16): ";
    cin >> my_key;
    cout << endl;

    for (int i = 0; i < ROUNDS; i++) {
        for (int j = 0; j < SIZE; j++) {
            ALLKEYS[j + (i * 16)] = KEY[j];
        }
        rotateKeyCipher();
    }
    cout << "\n" << endl;
    for (int i = 0; i < (SIZE * 4); i++) {
        cout << ALLKEYS[i];
    }
    cout << endl;

    // Crear objeto de lectura para el archivo
    ifstream lectura("plain.txt", ios::in);
    if (!lectura) {
        cerr << "Fail to read plain.txt" << endl;
        exit(EXIT_FAILURE);
    }

    // Determinar numero de caracteres
    lectura.seekg(0, ios_base::end);
    int nChars = lectura.tellg();
    PLAIN = new char[nChars];
    lectura.seekg(0, ios_base::beg);

    // Almacenar mensaje del archivo en arreglo
    char letra;
    int i = 0;
    while (lectura.get(letra)) {
        PLAIN[i] = letra;
        cout << PLAIN[i];
        i++;
    }
    cout << endl;

    // Determinar cantidad de procesos y su rango
    int numProcesses, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &numProcesses);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    // Start measuring the execution time
    double startTime = MPI_Wtime();

    // Determinar cantidad de bloques a procesar
    int n = nChars / (SIZE * numProcesses);
    int module = nChars % (SIZE * numProcesses);

    if (rank < module) {
        n++;
    }

    // Distribuir tareas entre los procesos
    char* temp = new char[n * SIZE];
    MPI_Scatter(PLAIN, n * SIZE, MPI_CHAR, temp, n * SIZE, MPI_CHAR, 0, MPI_COMM_WORLD);

    // Cifrar los bloques asignados
    for (int i = 0; i < n * SIZE; i += SIZE) {
        if (rank == 0) {
            cifrar(temp, i);
        }
        MPI_Bcast(temp + i, SIZE, MPI_CHAR, 0, MPI_COMM_WORLD);
    }

    // Reunir los bloques cifrados
    char* cipherText = NULL;
    if (rank == 0) {
        cipherText = new char[nChars];
    }
    MPI_Gather(temp, n * SIZE, MPI_CHAR, cipherText, n * SIZE, MPI_CHAR, 0, MPI_COMM_WORLD);

    // Escribir el texto cifrado en el archivo
    if (rank == 0) {
        ofstream archivo1;
        archivo1.open("cipher.bin", ios_base::out | ios_base::trunc); // Abriendo archivo en modo escritura
        if (archivo1.fail()) {
            cout << "No se pudo abrir el archivo";
            exit(1);
        }
        archivo1.write(cipherText, nChars);
        archivo1.close();
    }

    delete[] temp;
    if (rank == 0) {
        delete[] cipherText;
        
    // Stop measuring the execution time and calculate elapsed time
    double endTime = MPI_Wtime();
    double elapsedTime = endTime - startTime;

    // Print the elapsed time for each process
    cout << "Process " << rank << " took " << elapsedTime << " seconds for decryption." << endl;

    }
}

void desencriptar() {
    string my_key;
    cout << "Ingrese la llave: ";
    cin >> my_key;
    cout << endl;

    my_key.copy(KEY, SIZE + 1);

    for (int i = 0; i < ROUNDS; i++) {
        for (int j = 0; j < SIZE; j++) {
            ALLKEYS[j + (i * 16)] = KEY[j];
        }
        rotateKeyCipher();
    }
    cout << "\n" << endl;
    for (int i = 0; i < (SIZE * 4); i++) {
        cout << ALLKEYS[i];
    }
    cout << endl;

    // Crear objeto de lectura para el archivo
    ifstream lectura("cipher.bin", ios::in);
    if (!lectura) {
        cerr << "Fail to read cipher.bin" << endl;
        exit(EXIT_FAILURE);
    }

    // Determinar numero de caracteres
    lectura.seekg(0, ios_base::end);
    int nChars = lectura.tellg();
    PLAIN = new char[nChars];
    lectura.seekg(0, ios_base::beg);

    // Almacenar mensaje del archivo en arreglo
    char letra;
    int i = 0;
    while (lectura.get(letra)) {
        PLAIN[i] << letra;
        i++;
    }

    // Determinar cantidad de procesos y su rango
    int numProcesses, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &numProcesses);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // Start measuring the execution time
    double startTime = MPI_Wtime();

    // Determinar cantidad de bloques a procesar
    int n = nChars / (SIZE * numProcesses);
    int module = nChars % (SIZE * numProcesses);

    if (rank < module) {
        n++;
    }

    // Distribuir tareas entre los procesos
    char* temp = new char[n * SIZE];
    MPI_Scatter(PLAIN, n * SIZE, MPI_CHAR, temp, n * SIZE, MPI_CHAR, 0, MPI_COMM_WORLD);

    // Descifrar los bloques asignados
    for (int i = 0; i < n * SIZE; i += SIZE) {
        if (rank == 0) {
            descifrar(temp, i);
        }
        MPI_Bcast(temp + i, SIZE, MPI_CHAR, 0, MPI_COMM_WORLD);
    }

    // Reunir los bloques descifrados
    char* plainText = NULL;
    if (rank == 0) {
        plainText = new char[nChars];
    }
    MPI_Gather(temp, n * SIZE, MPI_CHAR, plainText, n * SIZE, MPI_CHAR, 0, MPI_COMM_WORLD);

    // Escribir el texto descifrado en el archivo
    if (rank == 0) {
        ofstream archivo1;
        archivo1.open("plain_decrypted.txt", ios_base::out | ios_base::trunc); // Abriendo archivo en modo escritura
        if (archivo1.fail()) {
            cout << "No se pudo abrir el archivo";
            exit(1);
        }
        archivo1.write(plainText, nChars);
        archivo1.close();
    }

    delete[] temp;
    if (rank == 0) {
        delete[] plainText;
    }
    
    // Stop measuring the execution time and calculate elapsed time
    double endTime = MPI_Wtime();
    double elapsedTime = endTime - startTime;

    // Print the elapsed time for each process
    cout << "Process " << rank << " took " << elapsedTime << " seconds for encryption." << endl;


}

int main(int argc, char** argv) {
    // Inicializar MPI
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    // Desplegar opciones
    int opc = 0;
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        cout << "Ingrese una opcion: \n\t(1) Encriptar  \n\t(2) Desencriptar " << endl;
        cin >> opc;
    }

    // Broadcast de la opción seleccionada
    MPI_Bcast(&opc, 1, MPI_INT, 0, MPI_COMM_WORLD);

    switch (opc) {
        case 1:
            if (rank == 0) {
                desencriptar();
            }
            break;
        case 2:
            if (rank == 0) {
                encriptar();
            }
            break;
        default:
            if (rank == 0) {
                cout << "Opción no válida" << endl;
            }
            break;
    }

    // Finalizar MPI
    MPI_Finalize();

    return 0;
}





