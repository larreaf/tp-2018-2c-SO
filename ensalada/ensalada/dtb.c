#include "dtb.h"

/*!
 * Copia los datos de un mensaje dinamico conteniendo datos de un DTB a un struct DTB
 * @param mensaje MensajeDinamico* que contiene los datos de un DTB (header = DATOS_DTB)
 * @param dtb puntero a struct DTB en la cual copiar los datos
 */
void desempaquetar_dtb(MensajeDinamico* mensaje, DTB* dtb){
    ArchivoAbierto* archivo_abierto;
    int size_lista_archivos;

    dtb->archivos_abiertos = list_create();
    recibir_int(&dtb->id, mensaje);
    recibir_int(&dtb->program_counter, mensaje);
    recibir_int(&dtb->inicializado, mensaje);
    recibir_int((int*)&dtb->status, mensaje);
    recibir_int(&dtb->quantum, mensaje);
    recibir_string(&dtb->path_script, mensaje);
    recibir_int(&size_lista_archivos, mensaje);

    for(int i = 0; i<size_lista_archivos; i++){
        archivo_abierto = malloc(sizeof(ArchivoAbierto));

        recibir_string(&archivo_abierto->path, mensaje);
        recibir_int(&archivo_abierto->direccion_memoria, mensaje);
        recibir_int(&archivo_abierto->equipo_grande, mensaje);

        list_add(dtb->archivos_abiertos, archivo_abierto);
    }
}

/*!
 * Envia los datos de un struct DTB a un CPU a traves de un MensajeDinamico
 * @param socket_cpu CPU al cual enviar los datos
 * @param dtb DTB del cual enviar los datos
 */
MensajeDinamico* generar_mensaje_dtb(int socket_cpu, DTB *dtb){
    MensajeDinamico* mensaje = crear_mensaje(DATOS_DTB, socket_cpu, 0);
    int tamanio_lista_archivos_abiertos = list_size(dtb->archivos_abiertos);
    ArchivoAbierto* archivo_abierto_seleccionado;

    agregar_dato(mensaje, sizeof(int), &(dtb->id));
    agregar_dato(mensaje, sizeof(int), &(dtb->program_counter));
    agregar_dato(mensaje, sizeof(int), &(dtb->inicializado));
    agregar_dato(mensaje, sizeof(int), &(dtb->status));
    agregar_dato(mensaje, sizeof(int), &(dtb->quantum));
    agregar_string(mensaje, dtb->path_script);

    agregar_dato(mensaje, sizeof(int), &tamanio_lista_archivos_abiertos);

    for(int i = 0; i<tamanio_lista_archivos_abiertos; i++){
        archivo_abierto_seleccionado = list_get(dtb->archivos_abiertos, i);

        agregar_string(mensaje, archivo_abierto_seleccionado->path);
        agregar_dato(mensaje, sizeof(int), &(archivo_abierto_seleccionado->direccion_memoria));
        agregar_dato(mensaje, sizeof(int), &(archivo_abierto_seleccionado->equipo_grande));
    }

    return mensaje;
}

DTB* encontrar_dtb_en_lista(t_list* lista, int id_dtb, bool remover_de_lista){
    DTB* dtb_seleccionado = NULL;
    int tamanio_lista = list_size(lista);

    for(int i = 0; i<tamanio_lista; i++){
        dtb_seleccionado = list_get(lista, i);

        if(dtb_seleccionado->id == id_dtb) {
            if(remover_de_lista)
                list_remove(lista, i);
            return dtb_seleccionado;
        }
    }
    return NULL;
}

MetricasDTB* encontrar_metricas_en_lista(t_list* lista, int id_dtb, bool remover_de_lista){
    MetricasDTB* metricas_seleccionado = NULL;
    int tamanio_lista = list_size(lista);

    for(int i = 0; i<tamanio_lista; i++){
        metricas_seleccionado = list_get(lista, i);

        if(metricas_seleccionado->id_dtb == id_dtb) {
            if(remover_de_lista)
                list_remove(lista, i);
            return metricas_seleccionado;
        }
    }
    return NULL;
}