//
// Created by utnso on 31/10/18.
//

#include "memoria.h"

/*!
 * Inicializa la memoria real (storage)
 * @param tamanio Tamanio en bytes del storage
 * @param tamanio_linea Tamanio de linea en bytes
 * @return struct MemoriaReal*
 */

	MemoriaReal* inicializar_memoria_real(int tamanio, int tamanio_linea, int tamanio_pagina){
    MemoriaReal* memoria_real = malloc(sizeof(MemoriaReal));

    memoria_real->logger = log_create("fm9.log", "MemoriaReal", true, log_level_from_string("info"));

    if(tamanio % tamanio_linea)
        log_warning(memoria_real->logger, "Tamanio (%d) no divisible por tamanio de linea (%d)", tamanio, tamanio_linea);

    if(tamanio % tamanio_pagina)
                log_warning(memoria_real->logger, "Tamanio (%d) no divisible por tamanio de pagina (%d)", tamanio, tamanio_pagina);

    memoria_real->tamanio = tamanio;
    memoria_real->tamanio_linea = tamanio_linea;
    memoria_real->cant_lineas = tamanio / tamanio_linea;
    memoria_real->estado_lineas = (char*)calloc(sizeof(char), (size_t)memoria_real->cant_lineas);
    memoria_real->tamanio_pagina = tamanio_pagina;
    memoria_real->cant_lineas_pagina = tamanio_pagina / tamanio_linea;
    memoria_real->cant_paginas = tamanio / tamanio_pagina;
    memoria_real->estado_paginas = (char*)calloc(sizeof(char), (size_t)memoria_real->tamanio/memoria_real->tamanio_pagina);

    memoria_real->inicio = malloc((size_t)tamanio);

    if(memoria_real->inicio == NULL)
        return NULL;

    log_info(memoria_real->logger, "Storage inicializado correctamente: Tamanio %d lineas de %d bytes cada una"
                                   " (total %d bytes)",
            memoria_real->cant_lineas, memoria_real->tamanio_linea,
            memoria_real->cant_lineas*memoria_real->tamanio_linea);
    return memoria_real;
}

/*!
 * Destruye el storage y libera memoria
 * @param storage
 */
void destruir_memoria_real(MemoriaReal* storage){
    free(storage->inicio);
    log_destroy(storage->logger);
    free(storage);
}

/*!
 * Inicializa memoria sobre el storage con un modo de ejecucion
 * @param storage MemoriaReal* ya inicializado
 * @param modo Modo de ejecucion
 * @return Memoria* inicializado
 */
Memoria* inicializar_memoria(MemoriaReal* storage, int modo){
    Memoria* memoria = malloc(sizeof(Memoria));

    memoria->storage = storage;
    memoria->lista_tablas_de_segmentos = list_create();
    memoria->lista_tabla_de_paginas_invertida = list_create();
    memoria->tabla_procesos = list_create();
    memoria->modo = modo;

    if(modo == SEG)
        memoria->logger = log_create("fm9.log", "MemoriaSegmentada", true, log_level_from_string("info"));
    else if(modo == SPA)
        memoria->logger = log_create("fm9.log", "MemoriaSegmentacionPaginada", true, log_level_from_string("info"));
    else if(modo == TPI)
        memoria->logger = log_create("fm9.log", "MemoriaPaginacionInvertida", true, log_level_from_string("info"));

    log_info(memoria->logger, "Memoria inicializada correctamente");
    return memoria;
}

/*!
 * Destruye tabla de segmentos
 * @param arg
 */
void destruir_tabla_segmentos(void* arg){
    NodoListaTablasSegmentos* tabla = (NodoListaTablasSegmentos*)arg;
    list_destroy_and_destroy_elements(tabla->tabla_de_segmentos, free);
}

/*!
 * Destruye memoria y storage
 * @param memoria
 */
void destruir_memoria(Memoria* memoria){
    list_destroy_and_destroy_elements(memoria->lista_tablas_de_segmentos, destruir_tabla_segmentos);
    log_destroy(memoria->logger);
    destruir_memoria_real(memoria->storage);
    free(memoria);
}

/*!
 * Encuentra la tabla de segmentos de un DTB
 * @param lista Lista de tabla de segmentos
 * @param id_dtb ID del DTB
 * @return NodoListaTablasSegmentos* que contiene la tabla de segmentos del proceso y metadata
 */
NodoListaTablasSegmentos* encontrar_tabla_segmentos_por_id_dtb(t_list* lista, int id_dtb){

    int _is_the_one(NodoListaTablasSegmentos*nodo) {
        return (nodo->id_dtb == id_dtb);
    }

    return list_find(lista, (void*) _is_the_one);
}

/*!
 * Escribe linea de storage
 * @param storage
 * @param linea datos a escribir
 * @param numero_linea numero de linea a escribir
 */
void escribir_linea(MemoriaReal* storage, char* linea, int numero_linea, char sobreescribir){
    if(strlen(linea)>storage->tamanio_linea){
        log_error(storage->logger, "Se intento escribir una linea mas grande del tamanio maximo de linea"
                                   " (linea %d, datos %s)", numero_linea, linea);
        return;
    }

    if(!storage->estado_lineas[numero_linea] || sobreescribir) {
        strcpy(storage->inicio + (numero_linea * storage->tamanio_linea), linea);
        storage->estado_lineas[numero_linea] = 1;
    }
}

/*!
 * Lee una linea de storage y retorna el contenido
 * @param storage
 * @param base numero de linea base
 * @param offset
 * @return char* con contenido de la linea
 */
char* leer_linea_storage(MemoriaReal* storage, int base, int offset){
    char* linea = string_new();
    string_append(&linea, (storage->inicio+(base*storage->tamanio_linea)+(offset*storage->tamanio_linea)));

    return linea;
}

/*!
 * Modifica una linea del storage
 * @param storage
 * @param base numero de linea base
 * @param offset
 * @param datos datos a escribir
 */
void modificar_linea_storage(MemoriaReal* storage, int base, int offset, char* datos){
    strcpy(storage->inicio+(base*storage->tamanio_linea)+(offset*storage->tamanio_linea), datos);
}

/*!
 * Cuenta la cantidad de \n en un string
 * @param string
 * @return cantidad de \n
 */
int contar_lineas(char* string){
    int i = 0;
    char* copia_string = string_new();

    string_append(&copia_string, string);

    while(strsep(&copia_string, "\n") != NULL)
        i++;

    free(copia_string);
    return i;
}

/*!
 * Escribe un archivo entero en storage en posiciones contiguas
 * @param storage
 * @param script string con el archivo
 * @param base numero de linea base
 */
void escribir_archivo_en_storage(MemoriaReal *storage, char *script, int base){
    int i = 0;
    char* word;

    while((word=strsep(&script, "\n")) != NULL) {
        escribir_linea(storage, word, base+i, 0);
        log_info(storage->logger, "Linea escrita en posicion %d: %s", base+i, word);
        i++;
    }
}

/*!
 * Encuentra espacio suficiente para almacenar un segmento
 * @param storage
 * @param cant_lineas_segmento
 * @return Posicion real en donde se puede almacenar el segmento
 */
int encontrar_espacio_para_segmento(MemoriaReal* storage, int cant_lineas_segmento){
    int j;

    for(int i = 0; i < storage->cant_lineas;){

        if(!storage->estado_lineas[i]) {
            for (j = 0; j < cant_lineas_segmento; j++) {

                if (!storage->estado_lineas[j+i])
                    continue;
                else {
                    i += j;
                    break;
                }
            }
            if(j == cant_lineas_segmento)
                return i;

            continue;
        }
        i++;
    }

    return -1;
}

/*!
 * Nos informa cuantas páginas vamos a necesitar para guardar lo que nos solicitan
 * @param storage
 * @param cant_lineas
 * @return cantidad de páginas necesarias
 *
 * created: 10/11/2018
 * modified: 15/11/2018
 */

int obtener_cantidad_paginas_necesarias(MemoriaReal* storage, int cant_lineas){

	int paginas_necesarias;

	if(storage->cant_lineas_pagina < cant_lineas){
		paginas_necesarias = storage->cant_lineas_pagina / cant_lineas;
	}

	paginas_necesarias ++;

	return paginas_necesarias;
}

/*!
 * Buscamos si tenemos las páginas suficientes como para guardar lo solicitado
 * @param storage
 * @param cant_lineas
 * @return en caso de error retorna -1 sino otro número para ok
 *
 * created: 10/11/2018
 * modified: 15/11/2018
 */

int verificar_si_hay_cantidad_paginas_necesarias(MemoriaReal* storage, int cant_paginas){

	int j;

    for(int i = 0; i < storage->cant_paginas;){

        if(!storage->estado_paginas[i]){
            for (j = 0; j < cant_paginas; j++) {

                if (!storage->estado_paginas[j+i])
                    continue;
                else {
                    i += j;
                    break;
                }
            }
            if(j == cant_paginas)
                return i;

            continue;
        }
        i++;
    }

    return -1;
}

/*!
 * Calculamos el marco que corresponde
 * @param storage
 * @param nro de pagina
 * @param id_dtb
 * @return el nro de marco
 *
 * created: 10/11/2018
 * modified: 15/11/2018
 */

int encontrar_marco_hash(Memoria* memoria,int pagina,int id_dtb){

	t_list* tabla_filtrada_por_pagina, tabla_filtrada_por_pagina_id_dtb;

	bool _is_the_page(NodoTablaPaginasInvertida* nodo){
		return (nodo->id_tabla == pagina);
	}

	tabla_filtrada_por_pagina = list_filter(memoria->lista_tabla_de_paginas_invertida, (void*) _is_the_page);

	bool _is_the_id_dtb(NodoTablaPaginasInvertida* nodo){
		return (nodo->id_dtb == id_dtb);
	}

	tabla_filtrada_por_pagina_id_dtb = list_filter(tabla_filtrada_por_pagina, (void*) _is_the_id_dtb);

	if(list_size(tabla_filtrada_por_pagina_id_dtb) == 1){

		NodoTablaPaginasInvertida nodo = list_get(tabla_filtrada_por_pagina_id_dtb, 0);

		return nodo->nro_pagina;
	}
	else
	{
		if(list_size(tabla_filtrada_por_pagina_id_dtb) == 1){
			NodoTablaPaginasInvertida nodo_error = list_get(tabla_filtrada_por_pagina, 0);

			encontrar_marco_hash(memoria, nodo_error->encadenamiento, id_dtb);
		}
		else{
			return 0;
		}
	}

	return 0;
}

/*!
 * Controla que el marco este sin usar sino encadena en la tabla invertida
 * @param memoria
 * @param marco
 * @return el nro marco pasado por parámetro en el caso que no haya encadenamiento, sino nuevo marco correspondiente
 *
 * created: 10/11/2018
 * modified: 15/11/2018
 */

int encadenamiento_tabla_paginas_invertida(Memoria* memoria,int posicion_marco){
	NodoTablaPaginasInvertida* nodo_filtrado;

	int _is_the_one(NodoTablaPaginasInvertida*nodo) {
	   return (nodo->id_tabla == posicion_marco);
	}

	nodo_filtrado = list_find(memoria->lista_tabla_de_paginas_invertida, (void*) _is_the_one);

	if(nodo_filtrado && nodo_filtrado->encadenamiento != 0){
		posicion_marco ++;
		encadenamiento_tabla_paginas_invertida(memoria, posicion_marco);
	}
	else
	{
		return posicion_marco;
	}

	return 0;
}

/*!
 * Carga un escriptorio en memoria
 * @param memoria
 * @param id_dtb ID del DTB para el cual se esta cargando el script
 * @param string string que contiene al script
 * @return 0 si se realizo de manera correcta (en segmentacion), -10002 si hubo error
 */
int cargar_script(Memoria* memoria, int id_dtb, char* string){
    t_list* tabla_segmentos_proceso;
    NodoTablaSegmentos* segmento_script;
    NodoListaTablasSegmentos* nodo_lista_tablas_segmentos;
    int posicion_segmento, cant_lineas;

    if(memoria->modo == SEG){
        nodo_lista_tablas_segmentos = malloc(sizeof(NodoListaTablasSegmentos));
        nodo_lista_tablas_segmentos->contador_segmentos = 0;

        tabla_segmentos_proceso = list_create();
        segmento_script = malloc(sizeof(NodoTablaSegmentos));

        cant_lineas = contar_lineas(string);
        log_info(memoria->logger, "Buscando espacio para segmento 0 para script de DTB %d (%d lineas)", id_dtb,
                cant_lineas);
        posicion_segmento = encontrar_espacio_para_segmento(memoria->storage, cant_lineas);
        if(posicion_segmento == -1)
            return -10002;

        log_info(memoria->logger, "Escribiendo segmento 0 a partir de linea %d", posicion_segmento);
        escribir_archivo_en_storage(memoria->storage, string, posicion_segmento);

        log_info(memoria->logger, "Guardando segmento en tabla de segmentos del DTB %d", id_dtb);
        segmento_script->id_segmento = nodo_lista_tablas_segmentos->contador_segmentos++;
        segmento_script->inicio_segmento = posicion_segmento;
        segmento_script->longitud_segmento = cant_lineas;

        nodo_lista_tablas_segmentos->id_dtb = id_dtb;
        nodo_lista_tablas_segmentos->tabla_de_segmentos = tabla_segmentos_proceso;

        list_add(tabla_segmentos_proceso, segmento_script);
        list_add(memoria->lista_tablas_de_segmentos, nodo_lista_tablas_segmentos);

        return 0;
    }
    else if(memoria->modo == TPI){

        	NodoTablaPaginasInvertida * nodo_lista_tabla_paginas_invertida;
    	    nodo_lista_tabla_paginas_invertida = malloc(sizeof(NodoTablaPaginasInvertida));
    		cant_lineas = contar_lineas(string);

    		log_info(memoria->logger, "Buscando espacio para pagina/s para script de DTB %d (%d lineas)", id_dtb,cant_lineas);

    		int paginas_necesarias = obtener_cantidad_paginas_necesarias(memoria->storage, cant_lineas);
    		int hay_paginas_necesarias = verificar_si_hay_cantidad_paginas_necesarias(memoria->storage, paginas_necesarias);

    		if(hay_paginas_necesarias == -1)
    			return -10002;

    		for(int a = 0; a < paginas_necesarias;a++){

    			int pagina = a + 1;
    			int posicion_marco = encontrar_marco_hash(memoria->storage, pagina, id_dtb);
    			posicion_marco = encadenamiento_tabla_paginas_invertida(memoria, posicion_marco);

    			log_info(memoria->logger, "Escribiendo pagina %d", pagina);

    			char* particion_string = string_new();

    			int desde_linea = a * memoria->storage->cant_lineas_pagina;
    			int contador = 0;
    			char* linea;

    			while((linea=strsep(&string, "\n")) != NULL) {
    				if(desde_linea >= contador && contador < pagina * memoria->storage->cant_lineas_pagina){
    					string_append(&particion_string, linea);
    				}

    				contador ++;
    			}

    			escribir_archivo_en_storage(memoria->storage, particion_string, posicion_marco);

    			log_info(memoria->logger, "Guardando pagina en tabla de paginas invertida");

    			nodo_lista_tabla_paginas_invertida->id_tabla = posicion_marco;
    			nodo_lista_tabla_paginas_invertida->id_dtb = id_dtb;
    			nodo_lista_tabla_paginas_invertida->nro_pagina = pagina;
    			nodo_lista_tabla_paginas_invertida->encadenamiento = NULL;

    			list_add(memoria->lista_tabla_de_paginas_invertida, nodo_lista_tabla_paginas_invertida);

    		}

    		return 0;
	}
	else if(memoria->modo == SPA){


	}

        return 0;
    }

/*!
 * Trae la ultima pagina cargada en tabla invertida
 * @param nro_pagina1
 * @param nro_pagina2
 * @return devuelve un numero entero con el nro de la ultima pagina
 */

int traer_ultima_pagina_id_dtb(int nro_pagina1,int nro_pagina2){
	return nro_pagina1 >= nro_pagina2 ? nro_pagina1 : nro_pagina2;
}

/*!
 * Carga un archivo en memoria
 * @param memoria
 * @param id_dtb ID del DTB para cual se esta cargando el archivo
 * @param string string con el archivo entero
 * @return direccion logica al archivo si se cargo de manera correcta, -10002 si hubo error
 */
int cargar_archivo(Memoria* memoria, int id_dtb, char* string){
    NodoListaTablasSegmentos* tabla_segmentos_proceso;
    NodoTablaSegmentos* segmento_script, *nodo_aux;
    NodoTablaPaginasInvertida* nodo_lista_tabla_paginas_invertida;
    int posicion_segmento, posicion_pagina, cant_lineas, direccion_logica = 0;

    if(memoria->modo == SEG){
        log_info(memoria->logger, "Buscando tabla de segmentos para DTB %d", id_dtb);
        tabla_segmentos_proceso = encontrar_tabla_segmentos_por_id_dtb(memoria->lista_tablas_de_segmentos, id_dtb);
        segmento_script = malloc(sizeof(NodoTablaSegmentos));

        cant_lineas = contar_lineas(string);
        log_info(memoria->logger, "Buscando espacio para segmento %d para DTB %d (%d lineas)",
                tabla_segmentos_proceso->contador_segmentos, id_dtb, cant_lineas);
        posicion_segmento = encontrar_espacio_para_segmento(memoria->storage, cant_lineas);
        if(posicion_segmento == -1)
            return -10002;

        log_info(memoria->logger, "Escribiendo segmento %d a partir de linea %d",
                tabla_segmentos_proceso->contador_segmentos, posicion_segmento);
        escribir_archivo_en_storage(memoria->storage, string, posicion_segmento);

        log_info(memoria->logger, "Guardando segmento en tabla de segmentos del DTB %d", id_dtb);
        segmento_script->id_segmento = tabla_segmentos_proceso->contador_segmentos++;
        segmento_script->inicio_segmento = posicion_segmento;
        segmento_script->longitud_segmento = cant_lineas;

        for(int i = 0; i<list_size(tabla_segmentos_proceso->tabla_de_segmentos); i++){
            nodo_aux = list_get(tabla_segmentos_proceso->tabla_de_segmentos, i);
            direccion_logica+=nodo_aux->longitud_segmento;
        }

        list_add(tabla_segmentos_proceso->tabla_de_segmentos, segmento_script);

        return direccion_logica;
    }
    else if(memoria->modo == TPI){
		NodoTablaPaginasInvertida * nodo_lista_tabla_paginas_invertida;
		nodo_lista_tabla_paginas_invertida = malloc(sizeof(NodoTablaPaginasInvertida));
		cant_lineas = contar_lineas(string);
		int pagina_base = list_fold(memoria->lista_tabla_de_paginas_invertida,0,traer_ultima_pagina_id_dtb) + 1;

		log_info(memoria->logger, "Buscando espacio para pagina/s para script de DTB %d (%d lineas)", id_dtb,cant_lineas);

		int paginas_necesarias = obtener_cantidad_paginas_necesarias(memoria->storage, cant_lineas);
		int hay_paginas_necesarias = verificar_si_hay_cantidad_paginas_necesarias(memoria->storage, paginas_necesarias);

		if(hay_paginas_necesarias == -1)
			return -10002;

		for(int a = 0; a < paginas_necesarias;a++){

			int pagina = a + 1;
			int posicion_marco = encontrar_marco_hash(memoria->storage, pagina, id_dtb);
			posicion_marco = encadenamiento_tabla_paginas_invertida(memoria, posicion_marco);

			log_info(memoria->logger, "Escribiendo pagina %d", pagina);

			char* particion_string = string_new();

			int desde_linea = a * memoria->storage->cant_lineas_pagina;
			int contador = 0;
			char* linea;

			while((linea=strsep(&string, "\n")) != NULL) {
				if(desde_linea >= contador && contador < pagina * memoria->storage->cant_lineas_pagina){
					string_append(&particion_string, linea);
				}

				contador ++;
			}

			escribir_archivo_en_storage(memoria->storage, particion_string, posicion_marco);

			log_info(memoria->logger, "Guardando pagina en tabla de paginas invertida");

			nodo_lista_tabla_paginas_invertida->id_tabla = posicion_marco;
			nodo_lista_tabla_paginas_invertida->id_dtb = id_dtb;
			nodo_lista_tabla_paginas_invertida->nro_pagina = pagina;
			nodo_lista_tabla_paginas_invertida->encadenamiento = 0;

			list_add(memoria->lista_tabla_de_paginas_invertida, nodo_lista_tabla_paginas_invertida);
			direccion_logica = pagina_base + cant_lineas;
		}


		return direccion_logica;
	}

	return 0;
}

/*!
 * Encuentra y lee una linea
 * @param memoria
 * @param id_dtb
 * @param numero_linea
 * @return char* con la linea leida
 */
char* leer_linea(Memoria* memoria, int id_dtb, int numero_linea){
    NodoListaTablasSegmentos* tabla_segmentos_proceso;
    NodoTablaSegmentos* segmento;
    int cantidad_segmentos;
    char* linea = string_new();

    if(memoria->modo == SEG){
        tabla_segmentos_proceso = encontrar_tabla_segmentos_por_id_dtb(memoria->lista_tablas_de_segmentos, id_dtb);
        cantidad_segmentos = list_size(tabla_segmentos_proceso->tabla_de_segmentos);

        for(int i = 0; i < cantidad_segmentos; i++){
            segmento = list_get(tabla_segmentos_proceso->tabla_de_segmentos, i);

            if(numero_linea<segmento->longitud_segmento){
                log_info(memoria->logger, "Leyendo linea %d en segmento %d (direccion %d) para DTB %d", numero_linea,
                        segmento->id_segmento, segmento->inicio_segmento+numero_linea, id_dtb);
                string_append(&linea, leer_linea_storage(memoria->storage, segmento->inicio_segmento, numero_linea));
                return linea;
            }else
                numero_linea -= segmento->longitud_segmento;
        }
    }
    else if(memoria->modo == TPI){

		NodoTablaPaginasInvertida * tabla_invertida = memoria->lista_tabla_de_paginas_invertida;
		NodoTablaPaginasInvertida * tabla_invertida_id_dtb = list_filter(tabla_invertida, tabla_invertida->id_dtb == id_dtb);

		int desplazamiento = numero_linea % memoria->storage->tamanio_pagina;
		int pagina = (numero_linea / memoria->storage->tamanio_pagina) + 1;

		int _is_the_one(NodoTablaPaginasInvertida* nodo) {
			   return (nodo->nro_pagina == pagina);
			}

		NodoTablaPaginasInvertida* nodo_filtrado = list_find(tabla_invertida_id_dtb, (void*) _is_the_one);

		if(nodo_filtrado){
			log_info(memoria->logger, "Leyendo linea %d en pagina %d (direccion %d) para DTB %d", numero_linea,
					nodo_filtrado->nro_pagina, nodo_filtrado->id_tabla + desplazamiento, id_dtb);
						string_append(&linea, leer_linea_storage(memoria->storage, nodo_filtrado->id_tabla, desplazamiento));
						return linea;
		}
		else{
			return -1;
		}
	}
}

/*!
 * Modifica una linea de un archivo
 * @param memoria
 * @param id_dtb
 * @param direccion direccion logica de la linea
 * @param datos char* a escribir
 * @return 0 si se realizo correctamente, 20001 si hubo error
 */
int modificar_linea_archivo(Memoria* memoria, int id_dtb, int direccion, char* datos){
    NodoListaTablasSegmentos* tabla_segmentos_proceso;
    NodoTablaSegmentos* segmento;
    int cantidad_segmentos;

    if(memoria->modo == SEG){
        tabla_segmentos_proceso = encontrar_tabla_segmentos_por_id_dtb(memoria->lista_tablas_de_segmentos, id_dtb);
        cantidad_segmentos = list_size(tabla_segmentos_proceso->tabla_de_segmentos);

        for(int i = 0; i < cantidad_segmentos; i++){
            segmento = list_get(tabla_segmentos_proceso->tabla_de_segmentos, i);

            if(direccion<segmento->longitud_segmento){
                log_info(memoria->logger, "Modificando linea %d en segmento %d (direccion %d) para DTB %d", direccion,
                         segmento->id_segmento, segmento->inicio_segmento+direccion, id_dtb);
                modificar_linea_storage(memoria->storage, segmento->inicio_segmento, direccion, datos);
                return 0;
            }else
                direccion -= segmento->longitud_segmento;
        }
        return 20001;
    }
    else if(memoria->modo == TPI){

		int nro_pagina = direccion / memoria->storage->tamanio_pagina;
		int marco = encontrar_marco_hash(memoria, nro_pagina, id_dtb);

		if(marco == -1){
			return 20001;
		}
		else{
			log_info(memoria->logger, "Modificando linea %d en pagina %d con marco %d (direccion %d) para DTB %d", direccion, nro_pagina, marco, marco+direccion, id_dtb);
			modificar_linea_storage(memoria->storage, marco, direccion, datos);
			return 0;
		}
	}

    return 20001;
}

/*!
 * Flushea un archivo
 * @param memoria
 * @param id_dtb
 * @param direccion direccion logica del archivo
 * @return los contenidos del archivo o un string vacio si hubo error
 */
char* flush_archivo(Memoria* memoria, int id_dtb, int direccion){
    NodoListaTablasSegmentos* tabla_segmentos_proceso;
    NodoTablaSegmentos* segmento;
    int cantidad_segmentos;
    char* string_archivo = string_new();

    if(memoria->modo == SEG){
        tabla_segmentos_proceso = encontrar_tabla_segmentos_por_id_dtb(memoria->lista_tablas_de_segmentos, id_dtb);
        cantidad_segmentos = list_size(tabla_segmentos_proceso->tabla_de_segmentos);

        for(int i = 0; i < cantidad_segmentos; i++){
            segmento = list_get(tabla_segmentos_proceso->tabla_de_segmentos, i);

            if(direccion<segmento->longitud_segmento){
                log_info(memoria->logger, "Leyendo archivo en segmento %d para DTB %d", segmento->id_segmento,
                        id_dtb);

                for(int j = 0; j<segmento->longitud_segmento; j++){
                    string_append(&string_archivo, leer_linea_storage(memoria->storage, segmento->inicio_segmento, j));
                    string_append(&string_archivo, "\n");
                }

                return string_archivo;
            }else
                direccion -= segmento->longitud_segmento;
        }
        return "";
    }
}

/*!
 * Libera memoria de un archivo
 * @param memoria
 * @param id_dtb
 * @param direccion direccion logica del archivo
 * @return 0 si se realizo correctamente, 40002 si hubo error
 */
int cerrar_archivo(Memoria* memoria, int id_dtb, int direccion){
    NodoListaTablasSegmentos* tabla_segmentos_proceso;
    NodoTablaSegmentos* segmento;
    int cantidad_segmentos, i;

    if(memoria->modo == SEG){
        tabla_segmentos_proceso = encontrar_tabla_segmentos_por_id_dtb(memoria->lista_tablas_de_segmentos, id_dtb);
        cantidad_segmentos = list_size(tabla_segmentos_proceso->tabla_de_segmentos);

        for(i = 0; i < cantidad_segmentos; i++){
            segmento = list_get(tabla_segmentos_proceso->tabla_de_segmentos, i);

            if(direccion<segmento->longitud_segmento){
                log_info(memoria->logger, "Liberando archivo en segmento %d para DTB %d", segmento->id_segmento,
                         id_dtb);

                for(int j = 0; j<segmento->longitud_segmento; j++){
                    escribir_linea(memoria->storage, "", segmento->inicio_segmento+j, 1);
                    memoria->storage->estado_lineas[segmento->inicio_segmento+j] = 0;
                }
                break;
            }else
                direccion -= segmento->longitud_segmento;
        }
        list_remove(tabla_segmentos_proceso->tabla_de_segmentos, i);
        return 0;
    }

    return 40002;
}

int desalojar_script(Memoria* memoria, int id_dtb){
    NodoListaTablasSegmentos* tabla_segmentos_proceso;
    NodoTablaSegmentos* segmento;
    int cantidad_segmentos;

    if(memoria->modo == SEG) {
        tabla_segmentos_proceso = encontrar_tabla_segmentos_por_id_dtb(memoria->lista_tablas_de_segmentos, id_dtb);
        cantidad_segmentos = list_size(tabla_segmentos_proceso->tabla_de_segmentos);

        for(int i = 0; i<cantidad_segmentos; i++) {
            segmento = list_get(tabla_segmentos_proceso->tabla_de_segmentos, 0);

            log_info(memoria->logger, "Liberando archivo en segmento %d para DTB %d", segmento->id_segmento,
                     id_dtb);

            for (int j = 0; j < segmento->longitud_segmento; j++) {
                escribir_linea(memoria->storage, "", segmento->inicio_segmento + j, 1);
                memoria->storage->estado_lineas[segmento->inicio_segmento + j] = 0;
            }

            list_remove(tabla_segmentos_proceso->tabla_de_segmentos, 0);
        }

        for(int i = 0; i<list_size(memoria->lista_tablas_de_segmentos); i++){
            tabla_segmentos_proceso = list_get(memoria->lista_tablas_de_segmentos, i);

            if(tabla_segmentos_proceso->id_dtb == id_dtb) {
                list_remove(memoria->lista_tablas_de_segmentos, i);
                free(tabla_segmentos_proceso);
            }
        }
        return 0;
    }

    return -1;
}

/*!
 * Imprime contenidos de un DTB en memoria y el estado del storage
 * @param memoria
 * @param id_dtb
 */
void dump(Memoria* memoria, int id_dtb){
    NodoListaTablasSegmentos* tabla_segmentos_proceso;
    NodoTablaSegmentos* segmento;
    int cantidad_segmentos;
    char* linea;

    printf("-------DUMP DTB %d-------\n", id_dtb);

    if(memoria->modo == SEG){
        tabla_segmentos_proceso = encontrar_tabla_segmentos_por_id_dtb(memoria->lista_tablas_de_segmentos, id_dtb);

        if(tabla_segmentos_proceso == NULL){
            printf("No se encuentran datos del DTB %d en memoria\n", id_dtb);
        }else {
            cantidad_segmentos = list_size(tabla_segmentos_proceso->tabla_de_segmentos);
            printf("Cantidad de segmentos: %d\n", cantidad_segmentos);

            for (int i = 0; i < cantidad_segmentos; i++) {
                segmento = list_get(tabla_segmentos_proceso->tabla_de_segmentos, i);
                printf("Contenidos de segmento %d (longitud segmento %d):\n", segmento->id_segmento,
                       segmento->longitud_segmento);

                for (int j = 0; j < segmento->longitud_segmento; j++) {
                    printf("%s\n", leer_linea_storage(memoria->storage, segmento->inicio_segmento, j));
                }

            }
        }
    }

    printf("------------------------\n");
    printf("------DUMP STORAGE------\n");

    for(int i = 0; i<memoria->storage->cant_lineas; i++){
        linea = string_new();

        string_append(&linea, leer_linea_storage(memoria->storage, i, 0));
        if(string_is_empty(linea) && !memoria->storage->estado_lineas[i])
            string_append(&linea, "*vacio*");

        printf("Linea %d: %s\n", i, linea);
        free(linea);
    }
}
