//
// Created by utnso on 31/10/18.
//

#ifndef FM9_MEMORIA_H
#define FM9_MEMORIA_H

#include <commons/log.h>
#include <commons/collections/list.h>
#include <ensalada/config-types.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>

typedef struct {
    int tamanio;
    int cant_lineas;
    int tamanio_linea;
    int tamanio_pagina;
    int cant_lineas_pagina;
    int cant_paginas;
    char* inicio;
    t_log* logger;
    char* estado_lineas;
    char* estado_paginas;
}MemoriaReal;

typedef struct {
    MemoriaReal* storage;
    t_list* lista_tablas_de_segmentos;
    t_list* lista_tabla_de_paginas_invertida;
    modo_memoria modo;
    t_log* logger;
}Memoria;

typedef struct {
    int id_segmento;
    int inicio_segmento;
    int longitud_segmento;
}NodoTablaSegmentos;

typedef struct {
    int id_dtb;
    t_list* tabla_de_segmentos;
    int contador_segmentos;
}NodoListaTablasSegmentos;

typedef struct{
	int id_tabla;
	int id_dtb;
    int nro_pagina;
    int encadenamiento;
}NodoTablaPaginasInvertida;

MemoriaReal* inicializar_memoria_real(int, int, int);
void destruir_memoria_real(MemoriaReal*);
void destruir_tabla_segmentos(void*);
Memoria* inicializar_memoria(MemoriaReal*, int);
void destruir_memoria(Memoria*);
void escribir_linea(MemoriaReal*, char*, int, char);
void escribir_archivo_en_storage(MemoriaReal*, char*, int);
void modificar_linea_storage(MemoriaReal*, int, int, char*);
int encontrar_espacio_para_segmento(MemoriaReal*, int);
int obtener_cantidad_paginas_necesarias(MemoriaReal*, int);
int verificar_si_hay_cantidad_paginas_necesarias(MemoriaReal*, int);
int encontrar_marco_hash(MemoriaReal*, int, int);
NodoListaTablasSegmentos* encontrar_tabla_segmentos_por_id_dtb(t_list*, int);
int contar_lineas(char*);
int cargar_script(Memoria*, int, char*);
int traer_ultima_pagina_id_dtb(int, int);
int cargar_archivo(Memoria*, int, char*);
char* leer_linea(Memoria*, int, int);
int modificar_linea_archivo(Memoria*, int, int, char*);
char* flush_archivo(Memoria*, int, int);
int cerrar_archivo(Memoria*, int, int);
int desalojar_script(Memoria*, int);
void dump(Memoria*, int);

#endif //FM9_MEMORIA_H
