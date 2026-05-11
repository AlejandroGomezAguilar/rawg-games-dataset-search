#ifndef COMMON_H
#define COMMON_H

// ===== CONSTANTES ===== //
#define MAX_RESULTADOS  50
#define DATASET_SIZE    80000
#define STR_LEN         80
#define STR_EXTRA       150

// ===== TIPOS DE BÚSQUEDA ===== //
typedef enum {
    MODO_ID = 1,
    MODO_NOMBRE = 2,
    MODO_RATING = 3,
    MODO_FECHA = 4,
    MODO_PLATAFORMA = 5,
    MODO_DEVELOPER = 6,
    MODO_GENERO = 7
} ModoBusqueda;

// ===== COMUNICACIÓN PROCESOS ===== //
#define FIFO_ENVIO      "tuberia_envio"
#define FIFO_RESPUESTA  "tuberia_respuesta"

typedef struct { // Criterios enviados de la interfaz al proceso de búsqueda
    ModoBusqueda modo;
    int id;
    char texto[STR_LEN];
    float rating_min;
    int anio;
} CriteriosBusqueda;

typedef struct { // Resultados enviados del proceso de búsqueda a la interfaz
    int id;
    char nombre[STR_LEN];
    char fecha[12];
    float rating;
    int rating_count;
    char platforms[STR_EXT];
    char developer[STR_LEN];
    char genres[STR_EXT];
} Juego;

typedef struct { 
    Juego juegos[MAX_RESULTADOS];
    int num_resultados;
} ResultadosBusqueda;

#endif