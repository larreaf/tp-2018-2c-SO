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
    t_list* tabla_procesos; //Lo pide segmentacion paginada
    int tamanio_maximo_segmento;
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

// Estructuras para segmentacion paginada
typedef struct {
	int id_proceso;
	t_list* tabla_segmentos;
	int cantidad_segmentos_codigo;
}NodoProceso;
typedef struct {
	t_list* tabla_paginas;
}NodoSegmento;

typedef struct {
	int numero_marco;
	int lineas_usadas;
}NodoPagina;


MemoriaReal* inicializar_memoria_real(int, int, int);
void destruir_memoria_real(MemoriaReal*);
void destruir_tabla_segmentos(void*);
Memoria* inicializar_memoria(MemoriaReal*, int, int);
void destruir_memoria(Memoria*);
void escribir_linea(MemoriaReal*, char*, int, char);
void escribir_archivo_en_storage(MemoriaReal*, char*, int);
void modificar_linea_storage(MemoriaReal*, int, int, char*);
int encontrar_espacio_para_segmento(MemoriaReal*, int);
int obtener_cantidad_paginas_necesarias(MemoriaReal*, int);
int verificar_si_hay_cantidad_paginas_necesarias(MemoriaReal*, int);
int encontrar_marco(Memoria*, int, int);
int calcular_marco_hash(MemoriaReal*, int, int);
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
int encontrar_marco_libre(MemoriaReal* storage);
void escribir_archivo_seg_pag(Memoria* memoria,int pid , int seg_init ,bool inicializar_archivo, char* buffer);
int obtener_numero_linea_pagina(int numero_marco, int tamanio_marco);
/*
 * Puede darse el caso de que agregue mas de un segmento
 */
int crear_segmento_y_agregarlo_al_proceso(NodoProceso* un_proceso,Memoria* memoria, int cant_lineas, int id_dtb);

/*
 * Para separar un string por \n aunque solo contenga puros \n
 */
char** lineas_split_(char* lineas);
int cuenta_saltos_de_linea(char* string);
void liberar_memoria_matriz(char** matriz);
int longitud_matriz(char** matriz);
int marcos_libres(MemoriaReal* storage, int cantidad_requerida_marcos);
#endif //FM9_MEMORIA_H
