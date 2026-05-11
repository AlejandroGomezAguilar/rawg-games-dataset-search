#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include "common.h"

// ============================================== //
// ============ ESTRUCTURA DE DISCO ============= //
// ============================================== //

// Esta estructura debe coincidir exactamente con common.h
// pero la definimos aquí para el manejo de archivos.
typedef struct {
    int id;
    char nombre[STR_LEN];
    char fecha[12];
    float rating;
    int rating_count;
    char platforms[STR_EXT];
    char developer[STR_LEN];
    char genres[STR_EXT];
} JuegoRegistro;

// ============================================== //
// ============ TABLA HASH PARA IDs ============= //
// ============================================== //
#define TABLE_SIZE 10000

typedef struct Node {
    int id;
    long offset; // Posición en el archivo binario
    struct Node* next;
} Node;

typedef struct {
    Node** table;
} HashTable;

// Función Hash simple para el ID
int hash(int id) { return id % TABLE_SIZE; }

void insert(HashTable* ht, int id, long offset) {
    int h = hash(id);
    Node* new_node = malloc(sizeof(Node));
    new_node->id = id;
    new_node->offset = offset;
    new_node->next = ht->table[h];
    ht->table[h] = new_node;
}

// ============================================== //
// =========== MOTOR DE PRE-PROCESO ============= //
// ============================================== //

// Función auxiliar para saltar comillas y manejar comas correctamente
char* obtener_campo(char **linea) {
    if (*linea == NULL || **linea == '\0') return "";
    char *inicio = *linea;
    int en_comillas = 0;

    while (**linea) {
        if (**linea == '"') en_comillas = !en_comillas;
        else if (**linea == ',' && !en_comillas) {
            **linea = '\0';
            (*linea)++;
            break;
        }
        (*linea)++;
    }
    // Limpiar comillas iniciales/finales si existen
    if (*inicio == '"') {
        inicio++;
        inicio[strlen(inicio)-1] = '\0';
    }
    return inicio;
}

void convertir_csv_a_binario(const char* csv_path, HashTable* ht) {
    FILE *csv = fopen(csv_path, "r");
    FILE *bin = fopen("juegos.bin", "wb");
    char buffer[10000]; // Buffer grande para líneas de 2GB

    // Saltar cabecera
    fgets(buffer, sizeof(buffer), csv);

    while (fgets(buffer, sizeof(buffer), csv)) {
        char *ptr = buffer;
        JuegoRegistro reg;
        memset(&reg, 0, sizeof(JuegoRegistro));

        // 1. ID (Columna 0)
        reg.id = atoi(obtener_campo(&ptr));
        
        // 2. Slug (Saltar Columna 1)
        obtener_campo(&ptr);

        // 3. Name (Columna 2)
        strncpy(reg.nombre, obtener_campo(&ptr), STR_LEN - 1);

        // 4. Released (Columna 3)
        strncpy(reg.fecha, obtener_campo(&ptr), 11);

        // ... Saltar columnas intermedias hasta Rating (Columna 5)
        obtener_campo(&ptr); // skip background_image
        reg.rating = atof(obtener_campo(&ptr)); // rating

        // ... Localizar Platforms (Columna 16), Developers (18), Genres (19)
        // (Nota: Tendrás que contar las comas exactas según el header que pasaste)
        
        long offset = ftell(bin);
        fwrite(&reg, sizeof(JuegoRegistro), 1, bin);
        insert(ht, reg.id, offset); // Indexamos el ID para búsqueda rápida
    }
    fclose(csv);
    fclose(bin);
}

// ============================================== //
// =========== MOTOR DE BÚSQUEDA ================ //
// ============================================== //

void ejecutar_busqueda(CriteriosBusqueda *c, ResultadosBusqueda *res, FILE *bin) {
    res->num_resultados = 0;
    JuegoRegistro reg;
    rewind(bin);

    while (fread(&reg, sizeof(JuegoRegistro), 1, bin) == 1 && res->num_resultados < MAX_RESULTADOS) {
        int coincide = 0;

        switch (c->modo) {
            case MODO_NOMBRE:
                if (strcasestr(reg.nombre, c->texto)) coincide = 1;
                break;
            case MODO_PLATAFORMA:
                if (strcasestr(reg.platforms, c->texto)) coincide = 1;
                break;
            case MODO_DEVELOPER:
                if (strcasestr(reg.developer, c->texto)) coincide = 1;
                break;
            case MODO_GENERO:
                if (strcasestr(reg.genres, c->texto)) coincide = 1;
                break;
            case MODO_RATING:
                if (reg.rating >= c->rating_min) coincide = 1;
                break;
            case MODO_FECHA:
                // Lógica para comparar el año en la cadena fecha "YYYY-MM-DD"
                if (strstr(reg.fecha, "2020")) coincide = 1; // Simplificado
                break;
        }

        if (coincide) {
            // Copiar los datos al struct de resultados para la tubería
            Juego *j = &res->juegos[res->num_resultados];
            j->id = reg.id;
            strcpy(j->nombre, reg.nombre);
            j->rating = reg.rating;
            j->rating_count = reg.rating_count;
            strcpy(j->platforms, reg.platforms);
            strcpy(j->developer, reg.developer);
            strcpy(j->genres, reg.genres);
            res->num_resultados++;
        }
    }
}

// El main seguiría la lógica de abrir FIFOs y esperar consultas...

// ============================================== //
// ==================== MAIN ==================== //
// ============================================== //

int main() {
    HashTable* ht = create_table(TABLE_SIZE);
    
    printf("📦 Cargando e indexando dataset de 2GB...\n");
    // Esto genera el juegos.bin y llena la tabla hash en RAM
    convertir_csv_a_binario("RAWG_Games_Dataset.csv", ht);
    
    FILE *bin = fopen("juegos.bin", "rb");
    if (!bin) { perror("Error al abrir binario"); return 1; }

    printf("✅ Sistema listo. Esperando peticiones...\n");

    while (1) {
        CriteriosBusqueda criterios;
        ResultadosBusqueda resultados;
        memset(&resultados, 0, sizeof(resultados));

        // 1. LEER de la tubería de envío
        int fd_env = open(FIFO_ENVIO, O_RDONLY);
        if (fd_env == -1) continue;
        read(fd_env, &criterios, sizeof(criterios));
        close(fd_env);

        // 2. EJECUTAR BÚSQUEDA
        if (criterios.modo == MODO_ID) {
            // Búsqueda instantánea por Tabla Hash
            Node* nodo = search_id(ht, criterios.id);
            if (nodo) {
                fseek(bin, nodo->offset, SEEK_SET);
                JuegoRegistro reg;
                fread(&reg, sizeof(JuegoRegistro), 1, bin);
                // Copiar a resultados...
                resultados.num_resultados = 1;
            }
        } else {
            // Búsqueda por texto (Barrido de disco)
            // Aquí recorremos el archivo binario comparando strings
            ejecutar_busqueda(&criterios, &resultados, bin);
        }

        // 3. ENVIAR por la tubería de respuesta
        int fd_resp = open(FIFO_RESPUESTA, O_WRONLY);
        if (fd_resp != -1) {
            write(fd_resp, &resultados, sizeof(resultados));
            close(fd_resp);
        }
    }

    return 0;
}