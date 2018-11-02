//
// Created by utnso on 31/10/18.
//

#include "memoria.h"

MemoriaReal* inicializar_memoria_real(int tamanio, int tamanio_linea){
    MemoriaReal* memoria_real = malloc(sizeof(MemoriaReal));

    memoria_real->logger = log_create("fm9.log", "MemoriaReal", true, log_level_from_string("info"));
    memoria_real->tamanio = tamanio;
    memoria_real->tamanio_linea = tamanio_linea;
    memoria_real->cant_lineas = tamanio / tamanio_linea;
    memoria_real->estado_lineas = (char*)calloc(sizeof(char), (size_t)memoria_real->cant_lineas);

    memoria_real->inicio = malloc((size_t)tamanio);

    if(memoria_real->inicio == NULL)
        return NULL;

    log_info(memoria_real->logger, "Storage inicializado correctamente");
    return memoria_real;
}

Memoria* inicializar_memoria(MemoriaReal* storage, int modo){
    Memoria* memoria = malloc(sizeof(Memoria));

    memoria->storage = storage;
    memoria->lista_tablas_de_segmentos = list_create();
    memoria->modo = modo;

    if(modo == SEG)
        memoria->logger = log_create("fm9.log", "MemoriaSegmentada", true, log_level_from_string("info"));
    else if(modo == SEGPAG)
        memoria->logger = log_create("fm9.log", "MemoriaSegmentacionPaginada", true, log_level_from_string("info"));
    else if(modo == PAGINV)
        memoria->logger = log_create("fm9.log", "MemoriaPaginacionInvertida", true, log_level_from_string("info"));

    log_info(memoria->logger, "Memoria inicializada correctamente");
    return memoria;
}

NodoListaTablasSegmentos* encontrar_tabla_segmentos_por_id_dtb(t_list* lista, int id_dtb){

    int _is_the_one(NodoListaTablasSegmentos*nodo) {
        return (nodo->id_dtb == id_dtb);
    }

    return list_find(lista, (void*) _is_the_one);
}

void escribir_linea(MemoriaReal* storage, char* linea, int numero_linea){
    if(!storage->estado_lineas[numero_linea]) {
        strcpy(storage->inicio + (numero_linea * storage->tamanio_linea), linea);
        storage->estado_lineas[numero_linea] = 1;
    }
}

char* leer_linea_storage(MemoriaReal* storage, int base, int offset){
    char* linea = string_new();
    string_append(&linea, (storage->inicio+(base*storage->tamanio_linea)+(offset*storage->tamanio_linea)));

    return linea;
}

void modificar_linea_storage(MemoriaReal* storage, int base, int offset, char* datos){
    strcpy(storage->inicio+(base*storage->tamanio_linea)+(offset*storage->tamanio_linea), datos);
}

int contar_lineas(char* string){
    int i = 0;
    char* copia_string = string_new();

    string_append(&copia_string, string);

    while(strsep(&copia_string, "\n") != NULL)
        i++;

    free(copia_string);
    return i;
}

void escribir_archivo_en_storage(MemoriaReal *storage, char *script, int base){
    int i = 0;
    char* word;

    while((word=strsep(&script, "\n")) != NULL) {
        escribir_linea(storage, word, base+i);
        log_info(storage->logger, "Linea escrita en posicion %d: %s", base+i, word);
        i++;
    }
}

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
}

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
}

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
}

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

    return 20001;
}

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