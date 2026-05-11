// ===== IMPORTS ===== //
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include "common.h"

// ===== UTILS TERMINAL ===== //
static void limpiar_pantalla(void) { system("clear"); }

static void pausa(void) {
    printf("\nPresione Enter para continuar...");
    while (getchar() != '\n'); 
}

// ===== VALIDACIONES ===== //
static void leer_string(const char *etiqueta, char *destino, int max) {
    printf("    Ingrese %s: ", etiqueta);
    fgets(destino, max, stdin);
    destino[strcspn(destino, "\n")] = 0; // Limpiar el salto de línea
}

static float pedir_float(const char *etiqueta) {
    char buf[32];
    printf("    Ingrese %s (ej: 4.5): ", etiqueta);
    fgets(buf, sizeof(buf), stdin);
    return atof(buf);
}

// ===== COMUNICACIÓN ===== //
static int enviar_y_recibir(CriteriosBusqueda *c, ResultadosBusqueda *r) {
    int fd_env = open(FIFO_ENVIO, O_WRONLY);
    if (fd_env == -1) {
        perror("\n❌ Error: El proceso de búsqueda no está activo");
        return 0;
    }
    write(fd_env, c, sizeof(CriteriosBusqueda));
    close(fd_env);

    int fd_resp = open(FIFO_RESPUESTA, O_RDONLY);
    if (fd_resp == -1) { perror("open respuesta"); return 0; }
    read(fd_resp, r, sizeof(ResultadosBusqueda));
    close(fd_resp);
    return 1;
}

static void mostrar_resultados(ResultadosBusqueda *res) {
    limpiar_pantalla();
    printf("====================================================\n");
    printf("🎮  RESULTADOS ENCONTRADOS: %d\n", res->num_resultados);
    printf("====================================================\n");

    if (res->num_resultados == 0) {
        printf("\n   ⚠️  No se encontraron juegos con esos criterios.\n");
    } else {
        for (int i = 0; i < res->num_resultados; i++) {
            printf("[%d] %s\n", res->juegos[i].id, res->juegos[i].nombre);
            printf("    ⭐ Rating: %.2f (%d votos) | 📅 Fecha: %s\n", 
                    res->juegos[i].rating, res->juegos[i].rating_count, res->juegos[i].fecha);
            printf("    🏢 Dev: %s\n", res->juegos[i].developer);
            printf("    🕹️  Plataformas: %s\n", res->juegos[i].platforms);
            printf("    🏷️  Géneros: %s\n", res->juegos[i].genres);
            printf("----------------------------------------------------\n");
        }
    }
}

// ===== MAIN MENU ===== //
int main() {
    int opcion;
    CriteriosBusqueda criterios;
    ResultadosBusqueda resultados;

    // Crear FIFOs si no existen
    mkfifo(FIFO_ENVIO, 0666);
    mkfifo(FIFO_RESPUESTA, 0666);

    do {
        limpiar_pantalla();
        printf("╔════════════════════════════════════════╗\n");
        printf("║       RAWG GAMES SEARCH ENGINE         ║\n");
        printf("╠════════════════════════════════════════╣\n");
        printf("║ 1. Buscar por Nombre (ej: Half-Life)   ║\n");
        printf("║ 2. Buscar por ID único                 ║\n");
        printf("║ 3. Filtrar por Rating Mínimo           ║\n");
        printf("║ 4. Buscar por Plataforma (ej: PC)      ║\n");
        printf("║ 5. Buscar por Desarrollador            ║\n");
        printf("║ 6. Buscar por Género                   ║\n");
        printf("║ 7. Salir                               ║\n");
        printf("╚════════════════════════════════════════╝\n");
        printf("Seleccione una opción: ");
        
        char buf[10];
        fgets(buf, sizeof(buf), stdin);
        opcion = atoi(buf);

        if (opcion >= 1 && opcion <= 6) {
            memset(&criterios, 0, sizeof(criterios));
            
            if (opcion == 1) {
                criterios.modo = MODO_NOMBRE;
                leer_string("nombre del juego", criterios.texto, STR_LEN);
            } else if (opcion == 2) {
                criterios.modo = MODO_ID;
                printf("    Ingrese ID: ");
                char id_buf[20];
                fgets(id_buf, sizeof(id_buf), stdin);
                criterios.id = atoi(id_buf);
            } else if (opcion == 3) {
                criterios.modo = MODO_RATING;
                criterios.rating_min = pedir_float("rating mínimo");
            } else if (opcion == 4) {
                criterios.modo = MODO_PLATAFORMA;
                leer_string("plataforma", criterios.texto, STR_LEN);
            } else if (opcion == 5) {
                criterios.modo = MODO_DEVELOPER;
                leer_string("desarrollador", criterios.texto, STR_LEN);
            } else if (opcion == 6) {
                criterios.modo = MODO_GENERO;
                leer_string("género", criterios.texto, STR_LEN);
            }

            printf("\n🔍 Buscando en el dataset de 2GB...");
            if (enviar_y_recibir(&criterios, &resultados)) {
                mostrar_resultados(&resultados);
            }
            pausa();
        }

    } while (opcion != 7);

    printf("\nCerrando interfaz...\n");
    return 0;
}
