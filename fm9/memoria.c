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
Memoria* inicializar_memoria(MemoriaReal* storage, int modo, int tamanio_maximo_segmento){
    Memoria* memoria = malloc(sizeof(Memoria));

    memoria->storage = storage;
    memoria->lista_tablas_de_segmentos = list_create();
    memoria->lista_tabla_de_paginas_invertida = list_create();
    memoria->tabla_procesos = list_create();
    memoria->modo = modo;
    memoria->tamanio_maximo_segmento = tamanio_maximo_segmento;

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
    list_destroy(memoria->lista_tabla_de_paginas_invertida);
    list_destroy(memoria->tabla_procesos);
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
 * modified: 29/11/2018
 */

int obtener_cantidad_paginas_necesarias(MemoriaReal* storage, int cant_lineas){

	int paginas_necesarias = 0;

	if(storage->cant_lineas_pagina < cant_lineas){
		paginas_necesarias = cant_lineas / storage->cant_lineas_pagina ;
		if(cant_lineas % storage->cant_lineas_pagina > 0){
			paginas_necesarias++;
		}
	}else {
		paginas_necesarias++;
	}
	return paginas_necesarias;
}

/*!
 * Buscamos si tenemos las páginas suficientes como para guardar lo solicitado
 * @param storage
 * @param cant_lineas
 * @return en caso de error retorna -1 sino 1 para ok
 *
 * created: 10/11/2018
 * modified: 29/11/2018
 */

int verificar_si_hay_cantidad_paginas_necesarias(MemoriaReal* storage, int cant_paginas_necesarias){

	int j;
	int i;

    for(i = 0; i < storage->cant_paginas;i++){

        if(!storage->estado_paginas[i]){
            for (j = 0; j < cant_paginas_necesarias; j++) {

                if (!storage->estado_paginas[j+i])
                    continue;
                else {
                    i++;
                    j--;

                    if(i >= storage->cant_paginas){
                    	return -1;
                    }
                }
            }
            if((j+1) == cant_paginas_necesarias){
            	return 1;
            }
        }
    }

    return -1;
}

/*!
 * Calculamos el marco que corresponde con la función hash
 * @param nro de pagina
 * @param id_dtb
 * @return el nro de marco
 *
 * created: 10/11/2018
 * modified: 29/11/2018
 */

int calcular_marco_hash(MemoriaReal* memoria_real, int pagina, int id_dtb){

	// en el caso que el id_dtb sea cero va a tirar error en la división
	id_dtb = id_dtb + 1;
	int marco = pagina % id_dtb;

	// sumamamos el uno ya que los marcos van del 0-n y la cant de paginas empieza de 1-n
	if((marco + 1) > memoria_real->cant_paginas){
		marco = 0;
	}

	return marco;
}

/*!
 * Buscamos el marco que corresponde
 * @param memoria
 * @param nro de pagina
 * @param id_dtb
 * @return el nro de marco
 *
 * created: 10/11/2018
 * modified: 29/11/2018
 */

int encontrar_marco(Memoria* memoria,int pagina,int id_dtb){

	t_list* tabla_filtrada_por_pagina;
	t_list* tabla_filtrada_por_pagina_id_dtb;
	tabla_filtrada_por_pagina = malloc(sizeof(NodoTablaPaginasInvertida));
	tabla_filtrada_por_pagina_id_dtb = malloc(sizeof(NodoTablaPaginasInvertida));

	bool _is_the_page(NodoTablaPaginasInvertida* nodo){
		return (nodo->id_tabla == pagina);
	}

	tabla_filtrada_por_pagina = list_filter(memoria->lista_tabla_de_paginas_invertida, (void*) _is_the_page);

	bool _is_the_id_dtb(NodoTablaPaginasInvertida* nodo){
		return (nodo->id_dtb == id_dtb);
	}

	tabla_filtrada_por_pagina_id_dtb = list_filter(tabla_filtrada_por_pagina, (void*) _is_the_id_dtb);

	if(list_size(tabla_filtrada_por_pagina_id_dtb) == 1){

		NodoTablaPaginasInvertida* nodo;
		nodo = malloc(sizeof(NodoTablaPaginasInvertida));
		nodo = list_get(tabla_filtrada_por_pagina_id_dtb, 0);

		return nodo->nro_pagina;
	}
	else
	{
		if(list_size(tabla_filtrada_por_pagina_id_dtb) == 1){
			NodoTablaPaginasInvertida* nodo_error;
			nodo_error = malloc(sizeof(NodoTablaPaginasInvertida));
			nodo_error = list_get(tabla_filtrada_por_pagina, 0);

			encontrar_marco(memoria, nodo_error->encadenamiento, id_dtb);
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
	nodo_filtrado = malloc(sizeof(NodoTablaPaginasInvertida));

	nodo_filtrado = list_get(memoria->lista_tabla_de_paginas_invertida, posicion_marco);

	if(nodo_filtrado){

		posicion_marco ++;

		if(nodo_filtrado->encadenamiento == -1){
			nodo_filtrado->encadenamiento = posicion_marco;
			list_replace(memoria->lista_tabla_de_paginas_invertida, posicion_marco - 1, nodo_filtrado);
			encadenamiento_tabla_paginas_invertida(memoria, posicion_marco);
		}
		else{
			encadenamiento_tabla_paginas_invertida(memoria, nodo_filtrado->encadenamiento);
		}
	}
	else
	{
		return posicion_marco;
	}

	return -1;
}

/*!
 * Buscar marco en la tabla de paginas invertidas
 * @param memoria
 * @param marco
 * @return el nro marco que corresponde con el id_dtb
 *
 * created: 01/12/2018
 * modified: 01/12/2018
 */

int buscar_marco_tabla_paginas_invertida(Memoria* memoria,int posicion_marco, int id_dtb){
	NodoTablaPaginasInvertida* nodo_filtrado;
	nodo_filtrado = malloc(sizeof(NodoTablaPaginasInvertida));

	int _is_the_one(NodoTablaPaginasInvertida*nodo) {
	   return (nodo->id_tabla == posicion_marco) && (nodo->id_dtb == id_dtb);
	}

	nodo_filtrado = list_find(memoria->lista_tabla_de_paginas_invertida, (void*) _is_the_one);

	if(!nodo_filtrado){

		int _por_marco(NodoTablaPaginasInvertida*nodo) {
			return (nodo->id_tabla == posicion_marco);
		}

		nodo_filtrado = list_find(memoria->lista_tabla_de_paginas_invertida, (void*) _por_marco);
		buscar_marco_tabla_paginas_invertida(memoria, nodo_filtrado->encadenamiento, id_dtb);
	}
	else
	{
		return posicion_marco;
	}

	return -1;
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
        log_info(memoria->logger, "Buscando espacio para segmento 0 para script de DTB %d (%d lineas)", id_dtb, cant_lineas);
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

    		int paginas_necesarias = 0;
    		int hay_paginas_necesarias = -1;
    		int pagina;
    		int posicion_marco;
    		int posicion_definitiva_marco;
        	NodoTablaPaginasInvertida * nodo_lista_tabla_paginas_invertida;
    	    nodo_lista_tabla_paginas_invertida = malloc(sizeof(NodoTablaPaginasInvertida));
    		cant_lineas = contar_lineas(string);

    		log_info(memoria->logger, "Buscando espacio para pagina/s para script de DTB %d (%d lineas)", id_dtb,cant_lineas);

    		paginas_necesarias = obtener_cantidad_paginas_necesarias(memoria->storage, cant_lineas);
    		hay_paginas_necesarias = verificar_si_hay_cantidad_paginas_necesarias(memoria->storage, paginas_necesarias);

    		//retorna que hubo error, no hay páginas necesarias
    		if(hay_paginas_necesarias == -1)
    			return -10002;

    		for(pagina = 0; pagina < paginas_necesarias; pagina++){

    			char* particion_string = string_new();
				int contador = 0;
				char* linea;

    			posicion_marco = calcular_marco_hash(memoria->storage, pagina, id_dtb);
    			posicion_definitiva_marco = encadenamiento_tabla_paginas_invertida(memoria, posicion_marco);

    			log_info(memoria->logger, "Escribiendo pagina %d", pagina);

    			while((linea=strsep(&string, "\n")) != NULL) {

    				if(contador < memoria->storage->cant_lineas_pagina){
    					string_append(&particion_string, linea);
    				}

    				contador ++;
    			}

    			escribir_archivo_en_storage(memoria->storage, particion_string, posicion_definitiva_marco);

    			log_info(memoria->logger, "Guardando pagina en tabla de paginas invertida");

    			nodo_lista_tabla_paginas_invertida->id_tabla = posicion_definitiva_marco;
    			nodo_lista_tabla_paginas_invertida->id_dtb = id_dtb;
    			nodo_lista_tabla_paginas_invertida->nro_pagina = pagina;
    			nodo_lista_tabla_paginas_invertida->lineas_usadas = contador;
    			nodo_lista_tabla_paginas_invertida->encadenamiento = -1;

    			list_add_in_index(memoria->lista_tabla_de_paginas_invertida, nodo_lista_tabla_paginas_invertida->id_tabla, nodo_lista_tabla_paginas_invertida);

    		}

    		return 0;
	}
	else if(memoria->modo == SPA){

		NodoProceso* nuevo_proceso = malloc(sizeof(NodoProceso));
		nuevo_proceso->id_proceso = id_dtb;
		nuevo_proceso->tabla_segmentos = list_create();
		nuevo_proceso->tabla_archivos = list_create();

		list_add(memoria->tabla_procesos, nuevo_proceso);
		int saltos_de_linea = cuenta_saltos_de_linea(string);
		//printf("String:\n %s \n",string);
		log_info(memoria->logger, "Buscando espacio para pagina/s para script de DTB %d (%d lineas)", id_dtb,saltos_de_linea);
		int resultado = crear_segmento_y_agregarlo_al_proceso(nuevo_proceso,memoria, saltos_de_linea, id_dtb);
		if(resultado == -10002)
			return -10002;
		nuevo_proceso->cantidad_segmentos_codigo = resultado;
		log_info(memoria->logger, "Escribiendo el código del DTB %d en memoria", id_dtb);
		escribir_archivo_seg_pag(memoria,id_dtb,0,false,string);

		return 0;
	}

        return -1;
    }

/*!
 * Trae la ultima pagina cargada en tabla invertida
 * @param nro_pagina1
 * @param nro_pagina2
 * @return devuelve un numero entero con el nro de la ultima pagina
 */

int traer_ultima_pagina_id_dtb(int max, NodoTablaPaginasInvertida* registro){
	int pagina = (max > registro->nro_pagina)? max : registro->nro_pagina;
	return pagina + 0;
}

/*!
 * Filtra la tabla pagina invertida por id_dtb
 * @param tabla_pagina_invertida
 * @return devuelve tabla pagina invertida filtrada por id_dtb
 */

t_list * traer_tabla_pagina_invertida_por_id_dtb(t_list* tabla_pagina_invertida, int id_dtb){

	int _es_del_id_dtb(NodoTablaPaginasInvertida*nodo) {
		return (nodo->id_dtb == id_dtb);
	}

	return list_find(tabla_pagina_invertida, (void*) _es_del_id_dtb);
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
    int posicion_segmento, cant_lineas, direccion_logica = 0;

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

    	NodoTablaPaginasInvertida* nodo_lista_tabla_paginas_invertida;
		nodo_lista_tabla_paginas_invertida = malloc(sizeof(NodoTablaPaginasInvertida));
		cant_lineas = contar_lineas(string);
		int paginas_necesarias;
		int hay_paginas_necesarias;
		int pagina;
		int posicion_marco;
		int posicion_definitiva_marco;
		t_list* tabla_filtrada_por_id_dtb;
		tabla_filtrada_por_id_dtb = malloc(sizeof(NodoTablaPaginasInvertida));
		tabla_filtrada_por_id_dtb = traer_tabla_pagina_invertida_por_id_dtb(memoria->lista_tabla_de_paginas_invertida, id_dtb);
		int pagina_base;

		pagina_base = *(int *) list_fold(tabla_filtrada_por_id_dtb,(int) 0, (void*) traer_ultima_pagina_id_dtb) + 1;


		log_info(memoria->logger, "Buscando espacio para pagina/s para script de DTB %d (%d lineas)", id_dtb, cant_lineas);

		paginas_necesarias = obtener_cantidad_paginas_necesarias(memoria->storage, cant_lineas);
		hay_paginas_necesarias = verificar_si_hay_cantidad_paginas_necesarias(memoria->storage, paginas_necesarias);

		if(hay_paginas_necesarias == -1){
			return -10002;
		}

		for(pagina = 0; pagina < paginas_necesarias; pagina++){

			char* particion_string = string_new();
			int contador = 0;
			char* linea;

			posicion_marco = calcular_marco_hash(memoria->storage, pagina + pagina_base, id_dtb);
			posicion_definitiva_marco = encadenamiento_tabla_paginas_invertida(memoria, posicion_marco);

			if(posicion_definitiva_marco == -1){
				return -10002;
			}

			log_info(memoria->logger, "Escribiendo pagina %d", pagina + pagina_base);

			while((linea=strsep(&string, "\n")) != NULL) {

				if(contador < memoria->storage->cant_lineas_pagina){
					string_append(&particion_string, linea);
				}

				contador ++;
			}

			escribir_archivo_en_storage(memoria->storage, particion_string, posicion_definitiva_marco);

			log_info(memoria->logger, "Guardando pagina %d en tabla de paginas invertida", pagina_base + pagina);

			nodo_lista_tabla_paginas_invertida->id_tabla = posicion_definitiva_marco;
			nodo_lista_tabla_paginas_invertida->id_dtb = id_dtb;
			nodo_lista_tabla_paginas_invertida->nro_pagina = pagina_base + pagina;
			nodo_lista_tabla_paginas_invertida->lineas_usadas = contador;
			nodo_lista_tabla_paginas_invertida->encadenamiento = -1;
			nodo_lista_tabla_paginas_invertida->tamanio_archivo = cant_lineas;

			list_add_in_index(memoria->lista_tabla_de_paginas_invertida, nodo_lista_tabla_paginas_invertida->id_tabla, nodo_lista_tabla_paginas_invertida);

		}

		direccion_logica = (pagina_base * memoria->storage->tamanio_pagina) + cant_lineas;

		return direccion_logica;
	}
    else if(memoria->modo == SPA){
    	bool _is_the_one(NodoProceso* proceso){
			if(proceso->id_proceso == id_dtb){
				return true;
			}else{
				return false;
			}
		}
    	NodoProceso* proceso = list_find(memoria->tabla_procesos, (void* )_is_the_one);
    	int cantidad_segmentos_nuevos = crear_segmento_y_agregarlo_al_proceso(proceso,memoria,cuenta_saltos_de_linea(string),id_dtb);
    	if(cantidad_segmentos_nuevos == -10002)
			return -10002;
    	bool inicializar = true;
    	if(string_length(string) > cuenta_saltos_de_linea(string)+1){
    		inicializar = false;
    	}
    	escribir_archivo_seg_pag(memoria, id_dtb, (list_size(proceso->tabla_segmentos) - cantidad_segmentos_nuevos), inicializar , string);
    	int direccion = 0;
    	/*int index = 0;
    	for(index = 0; index < list_size(proceso->tabla_segmentos) - cantidad_segmentos_nuevos ; index++){
    		NodoSegmento* segmento = list_get(proceso->tabla_segmentos, index);
    		int jndex = 0;
    		for(jndex = 0; jndex < list_size(segmento->tabla_paginas) ; jndex++){
    			NodoPagina* pagina = list_get(segmento->tabla_paginas, jndex);
    			direccion += pagina->lineas_usadas;
    		}
    	}*/
    	direccion = (list_size(proceso->tabla_segmentos) - cantidad_segmentos_nuevos)* memoria->tamanio_maximo_segmento * memoria->storage->cant_lineas_pagina;
    	return direccion;
    }

	return 0;
}

/*!
 * Encuentra y lee una linea de script
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

    	NodoTablaPaginasInvertida* nodo_filtrado;
    	nodo_filtrado = malloc(sizeof(NodoTablaPaginasInvertida*));
		t_list * tabla_invertida_id_dtb;
		tabla_invertida_id_dtb = malloc(sizeof(NodoTablaPaginasInvertida*));
		tabla_invertida_id_dtb = traer_tabla_pagina_invertida_por_id_dtb(memoria->lista_tabla_de_paginas_invertida, id_dtb);
		int desplazamiento, pagina, marco;

		desplazamiento = numero_linea % memoria->storage->tamanio_pagina;
		pagina = (numero_linea / memoria->storage->tamanio_pagina);
		marco = encontrar_marco(memoria, pagina, id_dtb);

		nodo_filtrado = list_get(tabla_invertida_id_dtb, marco);

		if(nodo_filtrado){

			log_info(memoria->logger, "Leyendo linea %d en pagina %d (direccion %d) para DTB %d",
					numero_linea,nodo_filtrado->nro_pagina, nodo_filtrado->id_tabla + desplazamiento, id_dtb);

			string_append(&linea, leer_linea_storage(memoria->storage, nodo_filtrado->id_tabla, desplazamiento));

			return linea;
		}
		else{
			return "";
		}
	}
    else if(memoria->modo == SPA){
    	bool _is_the_one(NodoProceso* proceso){
			if(proceso->id_proceso == id_dtb){
				return true;
			}else{
				return false;
			}
		}
    	NodoProceso* proceso = list_find(memoria->tabla_procesos,&_is_the_one);
    	NodoSegmento* segmento = NULL;
    	NodoPagina* pagina = NULL;
    	int numero_pagina_bruto = (numero_linea) / memoria->storage->cant_lineas_pagina;
    	int numero_pagina_neto = 0;
    	int numero_linea_neto = (numero_linea) % memoria->storage->cant_lineas_pagina;
    	int i = 0;
    	for (i = 0; i < list_size(proceso->tabla_segmentos) && numero_pagina_bruto > (memoria->tamanio_maximo_segmento*(i+1)) ; i++){}
    	segmento = list_get(proceso->tabla_segmentos, i);
    	numero_pagina_neto = numero_pagina_bruto - memoria->tamanio_maximo_segmento*i;
    	pagina = list_get(segmento->tabla_paginas, numero_pagina_neto);
    	string_append(&linea, leer_linea_storage(memoria->storage, obtener_numero_linea_pagina(pagina->numero_marco,memoria->storage->cant_lineas_pagina), numero_linea_neto));
    	return linea;
    }
    return "";
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
		int marco = encontrar_marco(memoria, nro_pagina, id_dtb);

		if(marco == -1){
			return 20001;
		}
		else{
			log_info(memoria->logger, "Modificando linea %d en pagina %d con marco %d (direccion %d) para DTB %d", direccion, nro_pagina, marco, marco+direccion, id_dtb);
			modificar_linea_storage(memoria->storage, marco, direccion, datos);
			return 0;
		}
	}
    else if(memoria->modo == SPA){
    	bool _is_the_one(NodoProceso* proceso){
			if(proceso->id_proceso == id_dtb){
				return true;
			}else{
				return false;
			}
		}
		NodoProceso* proceso = list_find(memoria->tabla_procesos,&_is_the_one);
		NodoSegmento* segmento = NULL;
		NodoPagina* pagina = NULL;
		int i = proceso->cantidad_segmentos_codigo;
		int j = 0;
		int lineas_recorridas = 0;
		int tamanio_pagina = memoria->storage->cant_lineas_pagina;
		int cantidad_paginas = 0;/*
		cantidad_segmentos = list_size(proceso->tabla_segmentos);
		for(i = 0; i < cantidad_segmentos; i++){ // Recorrer segmentos del proceso
			segmento = list_get(proceso->tabla_segmentos, i);
			cantidad_paginas = list_size(segmento->tabla_paginas);
			for(j = 0; j < cantidad_paginas; j++){ // Recorrer las paginas del segmento
				pagina = list_get(segmento->tabla_paginas, j);
				if (direccion < pagina->lineas_usadas){
					log_info(memoria->logger, "Modificando linea #%d en pagina #%d en segmento #%d (direccion %d) para DTB %d", direccion,j , i, ((pagina->numero_marco)*memoria->storage->cant_lineas_pagina)+direccion, id_dtb);
					modificar_linea_storage(memoria->storage, obtener_numero_linea_pagina(pagina->numero_marco,tamanio_pagina), direccion, datos);
					return 0;
				} else {
					direccion -= pagina->lineas_usadas;
					lineas_recorridas += pagina->lineas_usadas;
				}
			}
		}*/
		int num_segmento = (direccion / memoria->tamanio_maximo_segmento) - 1;
		int offset_segmento = direccion % memoria->tamanio_maximo_segmento;
		int num_pagina = offset_segmento / memoria->storage->cant_lineas_pagina;
		int offset_pagina = offset_segmento % memoria->storage->cant_lineas_pagina;
		segmento = list_get(proceso->tabla_segmentos, num_segmento);
		pagina = list_get(segmento->tabla_paginas, num_pagina);
		if(pagina != NULL){
			int dir_marco = obtener_numero_linea_pagina(pagina->numero_marco,tamanio_pagina);
			log_info(memoria->logger, "Modificando linea #%d en pagina #%d en segmento #%d (direccion %d) para DTB %d", offset_pagina, num_pagina, num_segmento, direccion/*(dir_marco)+offset_pagina*/, id_dtb);
			modificar_linea_storage(memoria->storage, dir_marco, offset_pagina, datos);
			return 0;
		}
		return 20001;
    }
    return 20001;
}

/*!
 * Flushea un archivo (guarda los datos en MDJ)
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

                for(int j = 0; j<segmento->longitud_segmento; j++){ // Arreglar para no leer \n demás
                    string_append(&string_archivo, leer_linea_storage(memoria->storage, segmento->inicio_segmento, j));
                    string_append(&string_archivo, "\n");
                }

                return string_archivo;
            }else
                direccion -= segmento->longitud_segmento;
        }
        return "";
    }
    else if(memoria->modo == TPI){

    	int desplazamiento, pagina, marco, tamanio_archivo;
    	NodoTablaPaginasInvertida* nodo_filtrado;
    	nodo_filtrado = malloc(sizeof(NodoTablaPaginasInvertida*));


    	pagina = direccion / memoria->storage->tamanio_pagina;
    	desplazamiento = direccion % memoria->storage->tamanio_pagina;
		marco = encontrar_marco(memoria, pagina, id_dtb);

		nodo_filtrado = list_get(memoria->lista_tabla_de_paginas_invertida, marco);
		tamanio_archivo = nodo_filtrado->tamanio_archivo;

		if(nodo_filtrado){

			for(int linea = 0; linea < tamanio_archivo; linea++ ){
				log_info(memoria->logger, "Leyendo linea %d en pagina %d (direccion %d) para DTB %d flush archivo",
						linea,nodo_filtrado->nro_pagina, nodo_filtrado->id_tabla + desplazamiento, id_dtb);

				string_append(&string_archivo, leer_linea_storage(memoria->storage, nodo_filtrado->id_tabla, desplazamiento + linea));

				string_append(&string_archivo, "\n");
			}

			return string_archivo;
		}
		else{
			return "";
		}
    }
    else if(memoria->modo == SPA){
    	bool _is_the_one(NodoProceso* proceso){
			if(proceso->id_proceso == id_dtb){
				return true;
			}else{
				return false;
			}
		}
		NodoProceso* proceso = list_find(memoria->tabla_procesos,&_is_the_one);
		NodoSegmento* segmento = NULL;
		NodoPagina* pagina = NULL;
		int i /*= proceso->cantidad_segmentos_codigo*/;
		/*int j = 0;
		int lineas_recorridas = 0;
		int lineas_leidas = 0;
		int tamanio_pagina = memoria->storage->cant_lineas_pagina;
		int cantidad_paginas = 0;

		cantidad_segmentos = list_size(proceso->tabla_segmentos);
		for(i = 0; i < cantidad_segmentos && lineas_leidas == 0; i++){ // Recorrer segmentos del proceso
			segmento = list_get(proceso->tabla_segmentos, i);
			cantidad_paginas = list_size(segmento->tabla_paginas);
			for(j = 0; j < cantidad_paginas; j++){ // Recorrer las paginas del segmento
				pagina = list_get(segmento->tabla_paginas, j);
				if (direccion < pagina->lineas_usadas){
					int lineas_leidas_en_pagina = 0;
					int linea = obtener_numero_linea_pagina(pagina->numero_marco,tamanio_pagina);
					for(lineas_leidas_en_pagina = 0; lineas_leidas_en_pagina < pagina->lineas_usadas ; lineas_leidas_en_pagina++){
						char* linea_leida =leer_linea_storage(memoria->storage, linea,lineas_leidas_en_pagina);
						log_info(memoria->logger, "Leyendo linea #%d en pagina #%d en segmento #%d (direccion %d) para DTB %d: %s", lineas_leidas_en_pagina,j, i, linea+direccion+lineas_leidas_en_pagina, id_dtb
								,linea_leida);
						string_append_with_format(&string_archivo,"%s\n",linea_leida);
						lineas_leidas++;
						free(linea_leida);
					}
				} else {
					direccion -= pagina->lineas_usadas;
				}
				lineas_recorridas += pagina->lineas_usadas;
			}
		}*/

		int numero_segmento = direccion / memoria->tamanio_maximo_segmento;
		//int offset_segmento = direccion % memoria->tamanio_maximo_segmento;
		bool _archivo_is_the_one(NodoArchivo* un_file_cualquiera){
			if(un_file_cualquiera->seg_inicial == numero_segmento){
			return true;
		}else{
			return false;
		}
		}
		NodoArchivo* archivo = list_find(proceso->tabla_archivos,& _archivo_is_the_one);
		if(archivo != NULL){
			int i_segmentos = archivo->seg_inicial;
			for(i_segmentos = archivo->seg_inicial; i_segmentos <= archivo->seg_final ; i_segmentos++){
				segmento = list_get(proceso->tabla_segmentos, i_segmentos);
				int i_paginas = 0;
				int cantidad_paginas = list_size(segmento->tabla_paginas);
				for(i_paginas = 0; i_paginas < cantidad_paginas; i_paginas++){
					pagina = list_get(segmento->tabla_paginas, i_paginas);
					int linea_marco = obtener_numero_linea_pagina(pagina->numero_marco, memoria->storage->cant_lineas_pagina);
					int i_offset_pagina = 0;
					for (i_offset_pagina = 0; i_offset_pagina < pagina->lineas_usadas ; i_offset_pagina++){
						char* linea_leida =leer_linea_storage(memoria->storage, linea_marco,i_offset_pagina);
						log_info(memoria->logger, "Leyendo linea #%d en pagina #%d en segmento #%d (direccion %d) para DTB %d: %s", i_offset_pagina,i_paginas, i_segmentos, linea_marco+i_offset_pagina, id_dtb, linea_leida);
						string_append_with_format(&string_archivo,"%s\n",linea_leida);
					}// dentro de la pagina
				}// dentro del segmento
			}//iterar los segmentos
			return string_archivo;
		}

    }
    return "";
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
    else if(memoria->modo == TPI){
    	int desplazamiento, pagina, marco, tamanio_archivo, cantidad_de_paginas_ocupadas;
		NodoTablaPaginasInvertida* nodo_filtrado;
		nodo_filtrado = malloc(sizeof(NodoTablaPaginasInvertida*));


		pagina = direccion / memoria->storage->tamanio_pagina;
		desplazamiento = direccion % memoria->storage->tamanio_pagina;
		marco = encontrar_marco(memoria, pagina, id_dtb);

		nodo_filtrado = list_get(memoria->lista_tabla_de_paginas_invertida, marco);
		tamanio_archivo = nodo_filtrado->tamanio_archivo;
		cantidad_de_paginas_ocupadas = tamanio_archivo / memoria->storage->tamanio_pagina;

		//Elimina de la tabla de paginas invertidas
		for(int nro_pagina = 0; nro_pagina < cantidad_de_paginas_ocupadas; nro_pagina++){
			list_remove(memoria->lista_tabla_de_paginas_invertida, pagina + nro_pagina);
		}

		//Elimina de memoria real
		for(int l = 0; l < tamanio_archivo; l++){
			escribir_linea(memoria->storage, "", marco*memoria->storage->cant_lineas_pagina + l, 1);
		}

		return 0;

    }
    else if(memoria->modo == SPA){
		bool _is_the_one(NodoProceso* proceso){
			if(proceso->id_proceso == id_dtb){
				return true;
			}else{
				return false;
			}
		}
		NodoProceso* proceso = list_find(memoria->tabla_procesos,&_is_the_one);
		NodoSegmento* segmento = NULL;
		NodoPagina* pagina = NULL;
		int i = proceso->cantidad_segmentos_codigo;
		int j = 0;
		int lineas_recorridas = 0;
		int lineas_leidas = 0;
		int tamanio_pagina = memoria->storage->cant_lineas_pagina;
		int cantidad_paginas = 0;

		int numero_segmento = direccion / memoria->tamanio_maximo_segmento - 1;
		bool _archivo_is_the_one(NodoArchivo* un_file_cualquiera){
			if(un_file_cualquiera->seg_inicial == numero_segmento){
				return true;
			}else{
				return false;
			}
		}
		NodoArchivo* archivo = list_find(proceso->tabla_archivos,& _archivo_is_the_one);
/*
		cantidad_segmentos = list_size(proceso->tabla_segmentos);
		for(i = 1; i < cantidad_segmentos && lineas_leidas == 0; i++){ // Recorrer segmentos del proceso
			segmento = list_get(proceso->tabla_segmentos, i);
			cantidad_paginas = list_size(segmento->tabla_paginas);
			for(j = 0; j < cantidad_paginas; j++){ // Recorrer las paginas del segmento
				pagina = list_get(segmento->tabla_paginas, j);
				if (direccion < pagina->lineas_usadas){
					int lineas_leidas_en_pagina = 0;
					memoria->storage->estado_paginas[pagina->numero_marco] = 0;
					int linea_inicial_pagina = obtener_numero_linea_pagina(pagina->numero_marco,tamanio_pagina);
					for(lineas_leidas_en_pagina = 0; lineas_leidas_en_pagina < pagina->lineas_usadas ; lineas_leidas_en_pagina++){
						escribir_linea(memoria->storage, "", linea_inicial_pagina+j, 1);
						lineas_leidas++;
					}
				} else {
					direccion -= pagina->lineas_usadas;
				}
				lineas_recorridas += pagina->lineas_usadas;
			}
		}*/
		int i_seg = 0;
		int cantidad_segmentos = archivo->seg_final - archivo->seg_inicial;
		if(cantidad_segmentos == 0){
			cantidad_segmentos = 1;
		}
		for(i_seg = 0; i_seg < cantidad_segmentos; i_seg++){
			segmento = list_get(proceso->tabla_segmentos, i_seg + archivo->seg_inicial);
			int cantidad_paginas = list_size(segmento->tabla_paginas);
			int i_pag = 0;
			for(i_pag = 0; i_pag < cantidad_paginas; i_pag++){
				pagina = list_get(segmento->tabla_paginas, i_pag);
				memoria->storage->estado_paginas[pagina->numero_marco] = 0;
				int linea_inicial_marco = obtener_numero_linea_pagina(pagina->numero_marco, tamanio_pagina);
				int i_offset_pag = 0;
				for(i_offset_pag = 0; i_offset_pag < pagina->lineas_usadas; i_offset_pag++){
					escribir_linea(memoria->storage,"", linea_inicial_marco + i_offset_pag, 1);
				}// dentro del marco
			}//dentro del segmento
			list_clean_and_destroy_elements(segmento->tabla_paginas, free);
		}// iterar segmentos
		return 0;
    }

    return 40002;
}

/*!
 * Desaloja un script del storage, junto con todos los archivos que no cerro previamente
 * @param memoria
 * @param id_dtb
 * @return 0 si no hubo error, -1 si hubo error
 */
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
    else if(memoria->modo == SPA){
		bool _is_the_one(NodoProceso* proceso){
			if(proceso->id_proceso == id_dtb){
				return true;
			}else{
				return false;
			}
		}
		NodoProceso* proceso = list_find(memoria->tabla_procesos,&_is_the_one);
		NodoSegmento* segmento = NULL;
		NodoPagina* pagina = NULL;
		int i = proceso->cantidad_segmentos_codigo;
		int j = 0;
		int lineas_recorridas = 0;
		int lineas_leidas = 0;
		int tamanio_pagina = memoria->storage->cant_lineas_pagina;
		int cantidad_paginas = 0;

		cantidad_segmentos = list_size(proceso->tabla_segmentos);
		for(i = 0; i < cantidad_segmentos ; i++){ // Recorrer segmentos del proceso
			segmento = list_get(proceso->tabla_segmentos, i);
			cantidad_paginas = list_size(segmento->tabla_paginas);
			for(j = 0; j < cantidad_paginas; j++){ // Recorrer las paginas del segmento
				pagina = list_get(segmento->tabla_paginas, j);
				memoria->storage->estado_paginas[pagina->numero_marco] = 0;
				int linea_inicial_pagina = obtener_numero_linea_pagina(pagina->numero_marco,tamanio_pagina);

				int lineas_leidas_en_pagina = 0;
				for(lineas_leidas_en_pagina = 0; lineas_leidas_en_pagina < pagina->lineas_usadas ; lineas_leidas_en_pagina++){
					escribir_linea(memoria->storage, "", linea_inicial_pagina+j, 1);
					lineas_leidas++;
				}

				lineas_recorridas += pagina->lineas_usadas;
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
    else if(memoria->modo == SPA){
    	bool _is_the_one(NodoProceso* proceso){
			if(proceso->id_proceso == id_dtb){
				return true;
			}else{
				return false;
			}
		}
    	NodoProceso* proceso = list_find(memoria->tabla_procesos, (void*) _is_the_one);
    	if(proceso == NULL){
    		printf("No se encuentran datos del DTB %d en memoria\n", id_dtb);
    	} else {
    		cantidad_segmentos = list_size(proceso->tabla_segmentos);
    		printf("Cantidad de segmentos: %d\n", cantidad_segmentos);
    		for (int i = 0; i < cantidad_segmentos; i++) {
				NodoSegmento* seg = list_get(proceso->tabla_segmentos, i);
				int cantidad_paginas = list_size(seg->tabla_paginas);
				printf("Contenido del segmento #%d (%d paginas):\n", i, cantidad_paginas);
				for (int j = 0; j < cantidad_paginas; j++) {
					NodoPagina* pagina = list_get(seg->tabla_paginas, j);
					int tamanio_pagina = memoria->storage->cant_lineas_pagina;
					int linea_marco = obtener_numero_linea_pagina(pagina->numero_marco, tamanio_pagina);
					for(int k = 0; k < tamanio_pagina ; k++){
						char* linea_leida = leer_linea_storage(memoria->storage, linea_marco, k);
						printf("%s\n", linea_leida );
						free(linea_leida);
					}// dentro de pagina
				}// dentro de segmento
			}// dentro del proceso
    	}//Existe el proceso
    }// SPA

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
/*!
 * Busca un marco libre y si lo encuentra lo retorna y lo coloca como ocupado
 * @param storage
 * @return -1 en caso de error
 * @return #marco libre
 */
int encontrar_marco_libre(MemoriaReal* storage){

	int index = 0;
	for(index = 0; index < storage->cant_paginas && storage->estado_paginas[index];index++){}
	if(storage->estado_paginas[index]){
		 return -1;
	}else{
		storage->estado_paginas[index] = 1;
		return index;
	}

}
void escribir_archivo_seg_pag(Memoria* memoria,int pid,int seg_init, bool inicializar_archivo, char* buffer){
	bool _is_the_one(NodoProceso* proceso){
		if(proceso->id_proceso == pid){
			return true;
		}else{
			return false;
		}
	}
	char sobreescribir = 1;
	int tamanio_pagina = memoria->storage->cant_lineas_pagina;
	NodoSegmento* segmento;
	NodoPagina* pagina;
	NodoProceso* proceso;
	int cantidad_lineas = cuenta_saltos_de_linea(buffer);
	int lineas_escritas = 0;
	//Verificar porque devuelve una linea demas
	/*if(inicializar_archivo == false){
		cantidad_lineas--;
	}*/

	int paginas_necesarias = obtener_cantidad_paginas_necesarias(memoria->storage, cantidad_lineas);
	proceso = list_find(memoria->tabla_procesos, &_is_the_one);
	if(seg_init <= 0){
		seg_init = 0;
		segmento = list_get(proceso->tabla_segmentos,0);
	} else {
		segmento = list_get(proceso->tabla_segmentos,seg_init);
	}
	char** lineas = string_split(buffer, "\n");
	int cant_substring = longitud_matriz(lineas);
	/*
	 * Ver acá para inicializar archivo con \n solamente
	 */
	int index = 0;
	for(index = 0; index < paginas_necesarias; index++){
		int j = 0;
		pagina = list_get(segmento->tabla_paginas, index);
		int numero_inicial_pagina = obtener_numero_linea_pagina(pagina->numero_marco,tamanio_pagina);
		int indice_lineas = (j+index*tamanio_pagina);
		if(inicializar_archivo){
			for(j = 0;  j < tamanio_pagina && lineas_escritas < cantidad_lineas ; j++){
				indice_lineas = (j+index*tamanio_pagina);
				log_info(memoria->logger, "Escribiendo linea #%d en pagina #%d del segmento #%d del DTB %d: \\n", indice_lineas, index ,seg_init, pid);
				escribir_linea(memoria->storage,"\n",(j+numero_inicial_pagina),sobreescribir);
				pagina->lineas_usadas++;
				lineas_escritas++;
			}
		} else {
			for(j = 0; j < tamanio_pagina && lineas_escritas < cantidad_lineas; j++){
				indice_lineas = (j+index*tamanio_pagina);

				if(lineas[indice_lineas] != NULL && lineas_escritas < cant_substring ){
					log_info(memoria->logger, "Escribiendo linea #%d en pagina #%d del segmento #%d del DTB %d: %s", indice_lineas, index ,seg_init, pid,lineas[indice_lineas]);
					escribir_linea(memoria->storage,lineas[indice_lineas],(j+numero_inicial_pagina),sobreescribir);
				} else{
					log_info(memoria->logger, "Escribiendo linea #%d en pagina #%d del segmento #%d del DTB %d: \\n", indice_lineas, index ,seg_init, pid,"\n");
					escribir_linea(memoria->storage,"\n",(j+numero_inicial_pagina),sobreescribir);
				}
				pagina->lineas_usadas++;
				lineas_escritas++;
			}
		}
	}
	liberar_memoria_matriz(lineas);


}

int obtener_numero_linea_pagina(int numero_marco, int tamanio_marco){
	return numero_marco*tamanio_marco;
}

int crear_segmento_y_agregarlo_al_proceso(NodoProceso* un_proceso,Memoria* memoria, int cant_lineas, int id_dtb){

	int cantidad_segmentos_agregados = 0;
	NodoSegmento* segmento ;
	NodoPagina* pagina;

	int paginas_necesarias = obtener_cantidad_paginas_necesarias(memoria->storage, cant_lineas);
	int cantidad_marcos_libres = marcos_libres(memoria->storage, paginas_necesarias);

	if(cantidad_marcos_libres < paginas_necesarias){
		return -10002;
	}

	//log_info(memoria->logger, "Hay paginas para el  DTB %d (%d lineas)", id_dtb,cant_lineas);
	int segmentos_necesarios = paginas_necesarias / memoria->tamanio_maximo_segmento;
	int paginas_ultimo_segmento = paginas_necesarias % memoria->tamanio_maximo_segmento;
	if( paginas_ultimo_segmento != 0){
		segmentos_necesarios++;
	}
	if(segmentos_necesarios == 1 && paginas_ultimo_segmento == 0){
		paginas_ultimo_segmento = memoria->tamanio_maximo_segmento;
	}
	int segmentos_existentes = list_size(un_proceso->tabla_segmentos);
	/*
	int index = 0;
	for (index = 0; index < paginas_necesarias ; index++){ //Establecer paginas
		pagina = malloc(sizeof(NodoPagina));
		pagina->numero_marco = encontrar_marco_libre(memoria->storage);
		pagina->lineas_usadas = 0;
		if(pagina->numero_marco == -1)
			return -10002;
		list_add(segmento->tabla_paginas,pagina);
		log_info(memoria->logger, "Agregada una pagina para el segmento %d para el código del DTB %d",(un_proceso->tabla_segmentos->elements_count - 1) ,id_dtb);
		if(list_size(segmento->tabla_paginas) >= memoria->tamanio_maximo_segmento){ //Agrega un nuevo segmento si el anterior se llenó
			segmento = NULL;
			segmento = malloc(sizeof(NodoSegmento));
			segmento->tabla_paginas = list_create();
			list_add(un_proceso->tabla_segmentos, segmento);
			log_info(memoria->logger, "Agregado un nuevo segmento para el código del DTB %d", id_dtb);
			cantidad_segmentos_agregados++;
		}
	}*/
	int index = 0;
	int cant_paginas_a_agregar = memoria->tamanio_maximo_segmento;
	for (index = 0; index < (segmentos_necesarios ) ; index++){
		NodoSegmento* segmento = malloc(sizeof(NodoSegmento));
		segmento->tabla_paginas = list_create();
		int j = 0;
		if(index == segmentos_necesarios - 1){
			cant_paginas_a_agregar = paginas_ultimo_segmento;
		}
		for (j = 0; j < cant_paginas_a_agregar; j++){
			NodoPagina* pagina = malloc(sizeof(NodoPagina));
			pagina->lineas_usadas = 0;
			pagina->numero_marco = encontrar_marco_libre(memoria->storage);
			list_add(segmento->tabla_paginas, pagina);
		}
		list_add(un_proceso->tabla_segmentos, segmento);
		cantidad_segmentos_agregados++;
	}
	NodoArchivo* archivo = malloc(sizeof(NodoArchivo));
	archivo->seg_inicial = segmentos_existentes;
	archivo->seg_final = segmentos_existentes + segmentos_necesarios - 1;


	list_add(un_proceso->tabla_archivos, archivo);
	return cantidad_segmentos_agregados;
}

char** lineas_split_(char* lineas){
	int saltos = cuenta_saltos_de_linea(lineas);
	char** buffer = string_split(lineas, "\n");
	int index = 0;
	char** nuevo = malloc(saltos+1); // +1 por el NULL
	for(index = 0; index < saltos; index++){
		if(buffer[index] != NULL){
			nuevo[index] = buffer[index];
		}else {
			nuevo[index] = malloc(2);
			nuevo[index][0] = '\n';
			nuevo[index][1] = '\0';
		}
	}
	nuevo[saltos] = 0x0;
	free(buffer);
	return nuevo;
}

int cuenta_saltos_de_linea(char* string){
	int i = 0;
	int contador_saltos_linea = 0;
	while(/*(string[i] != '\0' && string[i] != NULL)*/( string[i]== ' ' ||string[i] == '\n' || (string[i] > 45 && string[i] < 123))){
		if(string[i] == '\n'){
			contador_saltos_linea++;
		}
		i++;
	}
	return contador_saltos_linea;
}
void liberar_memoria_matriz(char** matriz){
	int index = 0;
	while(matriz[index]!= NULL){
		free(matriz[index]);
		index++;
	}
	free(matriz);
}
int longitud_matriz(char** matriz){
	int longitud = 0;

	while(matriz[longitud]!= NULL){
		longitud++;
	}
	return longitud;
}

int marcos_libres(MemoriaReal* storage, int cantidad_requerida_marcos){
	int marcos_libres = 0;
	int i = 0 ;
	for(i = 0; i < storage->cant_paginas; i++){
		if (!storage->estado_paginas[i]){
			marcos_libres++;
		}
	}
	return marcos_libres;
}
