# 🎮 RAWG Games Search Engine — Práctica 1
**Sistemas Operativos - Universidad Nacional de Colombia**

Este sistema permite realizar búsquedas eficientes sobre el dataset de RAWG Games (aprox. 2GB) utilizando una arquitectura de **proceso cliente-servidor** comunicados mediante tuberías nombradas (**FIFOs**).

## 👥 Integrantes
* Carlos Samuel Ariza Cano
* Jader Camilo Rodríguez
* Genaro Alejandro Gomez Aguilar

---

## 🏗️ Arquitectura del Sistema

El proyecto se divide en dos componentes principales que se ejecutan de forma paralela:

1.  **Motor de Búsqueda (`p1-dataProgram.c`):**
    * **Indexación:** Al iniciar, convierte el CSV original en un archivo binario de registros fijos (`juegos.bin`).
    * **Tabla Hash:** Almacena en RAM los pares `ID -> offset_en_disco`. Esto permite búsquedas de ID con complejidad $O(1)$.
    * **Servidor de Consultas:** Escucha peticiones en `FIFO_ENVIO`, procesa la búsqueda y devuelve los resultados por `FIFO_RESPUESTA`.

2.  **Interfaz de Usuario (`interface.c`):**
    * Presenta un menú interactivo para capturar los criterios (Nombre, Año, Rating, etc.).
    * Envía estructuras `CriteriosBusqueda` y recibe `ResultadosBusqueda` de forma síncrona.

---

## 🔍 Capacidades de Búsqueda
El sistema soporta 7 modos de consulta distintos:
1.  **Nombre del Juego:** Búsqueda parcial (case-insensitive).
2.  **Año de Lanzamiento:** Filtrado por los primeros 4 dígitos de la fecha.
3.  **Rating Mínimo:** Filtra juegos con calificación mayor o igual a la ingresada.
4.  **Plataforma:** Búsqueda por sistema (ej: PC, PlayStation).
5.  **Desarrollador:** Búsqueda por estudio de desarrollo.
6.  **Género:** Filtra por categorías (ej: Action, RPG).
7.  **ID Único:** Acceso instantáneo mediante la **Tabla Hash**.

---

## 🚀 Instrucciones de Uso

### 1. Preparación
Asegúrate de tener el archivo `rawg-games-dataset.csv` en la raíz del proyecto.
https://www.kaggle.com/datasets/atalaydenknalbant/rawg-games-dataset

### 2. Compilación
Utilice el comando `make` para generar los ejecutables:
```bash
make
