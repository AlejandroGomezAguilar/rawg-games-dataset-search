# rawg-games-dataset-search
Searching of values using pipes (FIFO)

# 🎮 RAWG Games Search Engine — Práctica 1
**Sistemas Operativos - Universidad Nacional de Colombia**

Este proyecto implementa un sistema de búsqueda de alto rendimiento para el dataset de RAWG Games (aprox. 2GB), comunicando dos procesos independientes mediante tuberías nombradas (FIFOs).

## 👥 Integrantes
* Carlos Samuel Ariza Cano
* Jader Camilo Rodríguez
* Genaro Alejandro Gomez Aguilar

---

## 🛠️ Arquitectura y Gestión de Memoria
Para cumplir con la restricción de **10MB de RAM** sobre un dataset de **1.96GB**, se implementó una estrategia de **Indexación por Offsets**:

1.  **Pre-procesamiento:** El sistema convierte el CSV original en un archivo binario (`juegos.bin`) con registros de tamaño fijo.
2.  **Tabla Hash (RAM):** Solo se almacena el `ID` del juego y su posición (`offset`) en el archivo binario. Esto permite que el uso de memoria sea constante independientemente del tamaño del dataset.
3.  **Búsqueda en Disco:** Las búsquedas por texto (Nombre, Desarrollador, etc.) se realizan mediante un barrido secuencial en disco con un buffer pequeño, garantizando respuestas en **< 2 segundos**.

---

## 🔍 Criterios de Búsqueda
Se han implementado los siguientes modos de consulta:

1.  **Nombre del Juego:** Búsqueda parcial (ej: "Half-Life" devuelve todas las versiones).
2.  **ID Único:** Acceso instantáneo mediante la Tabla Hash.
3.  **Rating Mínimo:** Filtra juegos con una calificación mayor o igual a la ingresada.
4.  **Plataforma:** Filtra por sistema (ej: PC, PlayStation, Xbox).
5.  **Desarrollador:** Búsqueda por estudio de desarrollo.
6.  **Género:** Filtra por categorías (ej: Action, Puzzle).

---

## 🚀 Instrucciones de Uso

### 1. Compilación
Utilice el comando `make` para generar los ejecutables:
```bash
make
