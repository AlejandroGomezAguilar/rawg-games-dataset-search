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

// Función para crear la tabla
HashTable* create_table(int size) {
    // 1. Reservar memoria para la estructura de la tabla
    HashTable* ht = (HashTable*)malloc(sizeof(HashTable));
    if (!ht) return NULL;

    // 2. Reservar memoria para el arreglo de punteros (buckets)
    ht->table = (Node**)malloc(sizeof(Node*) * size);
    if (!ht->table) {
        free(ht); // Limpiar si falló la segunda reserva
        return NULL;
    }

    // 3. Inicializar todos los punteros a NULL
    for (int i = 0; i < size; i++) {
        ht->table[i] = NULL;
    }
    return ht;
}

// Función hash simple
int hash_fn(int id) {
    return id % TABLE_SIZE;
}

// Función para insertar
void insert_id(HashTable* ht, int id, long offset) {
    int h = hash_fn(id);
    Node* new_node = malloc(sizeof(Node));
    new_node->id = id;
    new_node->offset = offset;
    new_node->next = ht->table[h];
    ht->table[h] = new_node;
}

// Función para buscar (Esta es la que te falta en el error)
Node* search_id(HashTable* ht, int id) {
    int h = hash_fn(id);
    Node* actual = ht->table[h];
    while (actual != NULL) {
        if (actual->id == id) return actual;
        actual = actual->next;
    }
    return NULL;
}

// ============================================== //
// =========== MOTOR DE PRE-PROCESO ============= //
// ============================================== //

// Parsea un campo CSV con soporte correcto de comillas dobles escapadas ("").
// Modifica el buffer in-place. Retorna puntero al contenido sin comillas externas.
char* obtener_campo(char **linea) {
    if (*linea == NULL || **linea == '\0') return "";

    char *inicio = *linea;

    if (*inicio == '"') {
        // Campo entre comillas
        inicio++;          // saltar comilla de apertura
        *linea = inicio;
        char *w = inicio;  // puntero de escritura

        while (**linea) {
            if (**linea == '"') {
                if (*(*linea + 1) == '"') {
                    // "" -> comilla literal
                    *w++ = '"';
                    *linea += 2;
                } else {
                    // comilla de cierre
                    *w = '\0';
                    (*linea)++;                      // saltar la "
                    if (**linea == ',') (*linea)++; // saltar la ,
                    break;
                }
            } else {
                *w++ = **linea;
                (*linea)++;
            }
        }
    } else {
        // Campo sin comillas: avanzar hasta coma o fin
        while (**linea && **linea != ',' && **linea != '\n' && **linea != '\r') {
            (*linea)++;
        }
        if (**linea == ',') {
            **linea = '\0';
            (*linea)++;
        } else {
            // fin de línea: terminar la cadena aquí
            if (**linea != '\0') **linea = '\0';
        }
    }
    return inicio;
}

// Header esperado del CSV RAWG (ajusta los índices si tu CSV difiere):
// Col 0:  id
// Col 1:  slug
// Col 2:  name
// Col 3:  released
// Col 4:  background_image
// Col 5:  rating
// Col 6:  rating_top
// Col 7:  ratings
// Col 8:  ratings_count
// Col 9:  reviews_text_count
// Col 10: added
// Col 11: added_by_status
// Col 12: metacritic
// Col 13: playtime
// Col 14: suggestions_count
// Col 15: updated
// Col 16: platforms
// Col 17: parent_platforms
// Col 18: developers
// Col 19: genres

void convertir_csv_a_binario(const char* csv_path, HashTable* ht) {
    FILE *csv = fopen(csv_path, "r");
    if (!csv) {
        perror("❌ Error al abrir el CSV");
        return;
    }
    FILE *bin = fopen("juegos.bin", "wb");
    if (!bin) {
        perror("❌ Error al crear el binario");
        fclose(csv);
        return;
    }
    char buffer[10000];

    // Leer la cabecera para detectar automáticamente las columnas
    fgets(buffer, sizeof(buffer), csv);

    // Detectar índices de columnas desde el encabezado
    int col_id = -1, col_name = -1, col_released = -1, col_rating = -1;
    int col_rating_count = -1, col_platforms = -1, col_developers = -1, col_genres = -1;
    {
        char header_copy[10000];
        strncpy(header_copy, buffer, sizeof(header_copy) - 1);
        char *ptr = header_copy;
        int col = 0;
        char *campo;
        while (*(campo = obtener_campo(&ptr)) != '\0' || ptr > header_copy) {
            if      (strcmp(campo, "id")            == 0) col_id           = col;
            else if (strcmp(campo, "name")          == 0) col_name         = col;
            else if (strcmp(campo, "released")      == 0) col_released     = col;
            else if (strcmp(campo, "rating")        == 0) col_rating       = col;
            else if (strcmp(campo, "ratings_count") == 0) col_rating_count = col;
            else if (strcmp(campo, "platforms")     == 0) col_platforms    = col;
            else if (strcmp(campo, "developers")    == 0) col_developers   = col;
            else if (strcmp(campo, "genres")        == 0) col_genres       = col;
            col++;
            if (*ptr == '\0' && campo[0] == '\0') break;
        }
        // Fallback: indices confirmados con el CSV real de RAWG:
        // col 0=id, 1=slug, 2=name, 3=released, 4=background_image,
        // col 5=rating, 6=rating_top, 7=ratings_count, 8=reviews_text_count,
        // col 16=platforms, 18=developers, 19=genres
        if (col_id           < 0) col_id           = 0;
        if (col_name         < 0) col_name         = 2;
        if (col_released     < 0) col_released     = 3;
        if (col_rating       < 0) col_rating       = 5;
        if (col_rating_count < 0) col_rating_count = 7;
        if (col_platforms    < 0) col_platforms    = 16;
        if (col_developers   < 0) col_developers   = 18;
        if (col_genres       < 0) col_genres       = 19;

        printf("📋 Columnas detectadas: id=%d name=%d released=%d rating=%d "
               "rating_count=%d platforms=%d developers=%d genres=%d\n",
               col_id, col_name, col_released, col_rating,
               col_rating_count, col_platforms, col_developers, col_genres);
    }

    int max_col = col_id;
    if (col_name         > max_col) max_col = col_name;
    if (col_released     > max_col) max_col = col_released;
    if (col_rating       > max_col) max_col = col_rating;
    if (col_rating_count > max_col) max_col = col_rating_count;
    if (col_platforms    > max_col) max_col = col_platforms;
    if (col_developers   > max_col) max_col = col_developers;
    if (col_genres       > max_col) max_col = col_genres;

    long registros = 0;
    while (fgets(buffer, sizeof(buffer), csv)) {
        char *ptr = buffer;
        JuegoRegistro reg;
        memset(&reg, 0, sizeof(JuegoRegistro));

        // Recorrer columna por columna hasta la última que necesitamos
        for (int col = 0; col <= max_col; col++) {
            char *campo = obtener_campo(&ptr);

            if      (col == col_id)           reg.id           = atoi(campo);
            else if (col == col_name)         strncpy(reg.nombre,    campo, STR_LEN - 1);
            else if (col == col_released)     strncpy(reg.fecha,     campo, 11);
            else if (col == col_rating)       reg.rating       = atof(campo);
            else if (col == col_rating_count) reg.rating_count = atoi(campo);
            else if (col == col_platforms)    strncpy(reg.platforms, campo, STR_EXT - 1);
            else if (col == col_developers)   strncpy(reg.developer, campo, STR_LEN - 1);
            else if (col == col_genres)       strncpy(reg.genres,    campo, STR_EXT - 1);
        }

        if (reg.id == 0) continue; // Saltar filas inválidas

        long offset = ftell(bin);
        fwrite(&reg, sizeof(JuegoRegistro), 1, bin);
        insert_id(ht, reg.id, offset);
        registros++;
    }

    printf("✅ %ld registros convertidos a juegos.bin\n", registros);
    fclose(csv);
    fclose(bin);
}

void liberar_tabla(HashTable* ht) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        Node* actual = ht->table[i];
        while (actual) {
            Node* temp = actual;
            actual = actual->next;
            free(temp);
        }
    }
    free(ht->table);
    free(ht);
}

// ============================================== //
// =========== MOTOR DE BÚSQUEDA ================ //
// ============================================== //

void ejecutar_busqueda(CriteriosBusqueda *c, ResultadosBusqueda *res, FILE *bin) {
    res->num_resultados = 0;
    JuegoRegistro reg;
    rewind(bin);

    while (fread(&reg, sizeof(JuegoRegistro), 1, bin) == 1) {
                                                      // && res->num_resultados < MAX_RESULTADOS) {  
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
                if (reg.rating == c->rating_min) coincide = 1;
                break;
            case MODO_FECHA: {
                // La fecha está en formato "YYYY-MM-DD": comparar los 4 primeros dígitos
                char anio_str[5];
                snprintf(anio_str, sizeof(anio_str), "%d", c->anio);
                if (strncmp(reg.fecha, anio_str, 4) == 0) coincide = 1;
                break;
            }
        }

        if (coincide) {
            if (res->num_resultados < MAX_RESULTADOS) {
                // Copiar los datos al struct de resultados para la tubería
                Juego *j = &res->juegos[res->num_resultados];
                j->id           = reg.id;
                j->rating       = reg.rating;
                j->rating_count = reg.rating_count;
                strncpy(j->nombre,    reg.nombre,    STR_LEN - 1);
                strncpy(j->fecha,     reg.fecha,     11);
                strncpy(j->platforms, reg.platforms, STR_EXT - 1);
                strncpy(j->developer, reg.developer, STR_LEN - 1);
                strncpy(j->genres,    reg.genres,    STR_EXT - 1);
                res->num_resultados++;
            }
            res->total_encontrados++;  // <-- siempre contar
            res->num_resultados = (res->total_encontrados < MAX_RESULTADOS) 
                              ? res->total_encontrados 
                              : MAX_RESULTADOS;
        }
    }
}

// El main seguiría la lógica de abrir FIFOs y esperar consultas...

// ============================================== //
// ==================== MAIN ==================== //
// ============================================== //

// Lee exactamente n bytes de un fd, reintentando si el SO devuelve menos
static ssize_t read_all(int fd, void *buf, size_t n) {
    size_t total = 0;
    while (total < n) {
        ssize_t r = read(fd, (char*)buf + total, n - total);
        if (r <= 0) return r;
        total += r;
    }
    return (ssize_t)total;
}

// Escribe exactamente n bytes en un fd
static ssize_t write_all(int fd, const void *buf, size_t n) {
    size_t total = 0;
    while (total < n) {
        ssize_t w = write(fd, (const char*)buf + total, n - total);
        if (w <= 0) return w;
        total += w;
    }
    return (ssize_t)total;
}

int main() {
    HashTable* ht = create_table(TABLE_SIZE);
    
    printf("📦 Cargando e indexando dataset...\n");
    // Esto genera el juegos.bin y llena la tabla hash en RAM
    convertir_csv_a_binario("rawg-games-dataset.csv", ht);
    
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
        if (read_all(fd_env, &criterios, sizeof(criterios)) != sizeof(criterios)) { close(fd_env); continue; }
        close(fd_env);

        // 2. EJECUTAR BÚSQUEDA
        if (criterios.modo == MODO_ID) {
            // Búsqueda instantánea por Tabla Hash
            Node* nodo = search_id(ht, criterios.id);
            if (nodo) {
                fseek(bin, nodo->offset, SEEK_SET);
                JuegoRegistro reg;
                fread(&reg, sizeof(JuegoRegistro), 1, bin);
                // Copiar todos los campos al struct de resultados
                Juego *j = &resultados.juegos[0];
                j->id           = reg.id;
                j->rating       = reg.rating;
                j->rating_count = reg.rating_count;
                strncpy(j->nombre,    reg.nombre,    STR_LEN - 1);
                strncpy(j->fecha,     reg.fecha,     11);
                strncpy(j->platforms, reg.platforms, STR_EXT - 1);
                strncpy(j->developer, reg.developer, STR_LEN - 1);
                strncpy(j->genres,    reg.genres,    STR_EXT - 1);
                resultados.num_resultados = 1;
                resultados.total_encontrados = 1;
            }
        } else {
            // Búsqueda por texto (Barrido de disco)
            // Aquí recorremos el archivo binario comparando strings
            ejecutar_busqueda(&criterios, &resultados, bin);
        }

        // 3. ENVIAR por la tubería de respuesta
        int fd_resp = open(FIFO_RESPUESTA, O_WRONLY);
        if (fd_resp != -1) {
            write_all(fd_resp, &resultados, sizeof(resultados));
            close(fd_resp);
        }
    }

    fclose(bin);
    liberar_tabla(ht);
    return 0;
}
