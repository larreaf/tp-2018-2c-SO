#include "mdj_functions.h"


void validar_archivo(t_mdj_interface* data_operacion){

}

void crear_archivo(t_mdj_interface* data_operacion){

}

char* obtener_datos(t_mdj_interface* data_operacion){
	char* datos = NULL;
	return datos;
}

int guardar_datos(t_mdj_interface* data_operacion){
	return 0;
}

t_mdj_interface* crear_data_mdj_operacion(MensajeDinamico* mensaje){
	t_mdj_interface* data_mdj_operacion = malloc(sizeof(t_mdj_interface));
	NodoPayload* nodo_aux = queue_pop(mensaje->payload);
	data_mdj_operacion->path = nodo_aux->datos;
	data_mdj_operacion->t_operacion = mensaje->header;
	 switch(mensaje->header){
		case VALIDAR_ARCHIVO:
			break;
		case CREAR_ARCHIVO:
			nodo_aux = queue_pop(mensaje->payload);
			memcpy(&data_mdj_operacion->cantidad_lineas, nodo_aux->datos,nodo_aux->longitud);
			break;
		case OBTENER_DATOS:
			nodo_aux = queue_pop(mensaje->payload);
			memcpy(&data_mdj_operacion->offset, nodo_aux->datos,nodo_aux->longitud);
			nodo_aux = queue_pop(mensaje->payload);
			memcpy(&data_mdj_operacion->size, nodo_aux->datos,nodo_aux->longitud);
			break;
		case GUARDAR_DATOS:
			nodo_aux = queue_pop(mensaje->payload);
			memcpy(&data_mdj_operacion->offset, nodo_aux->datos,nodo_aux->longitud);
			nodo_aux = queue_pop(mensaje->payload);
			memcpy(&data_mdj_operacion->size, nodo_aux->datos,nodo_aux->longitud);
			nodo_aux = queue_pop(mensaje->payload);
			memcpy(&data_mdj_operacion->buffer, nodo_aux->datos,nodo_aux->longitud);
			break;

	}
	 return data_mdj_operacion;

}

void interface_mdj(MensajeDinamico* mensaje_dinamico){
	pthread_t nuevo_hilo;
	t_mdj_interface* data_operacion = crear_data_mdj_operacion(mensaje_dinamico);

	switch(mensaje_dinamico->header){

		case VALIDAR_ARCHIVO:
			pthread_create(&nuevo_hilo,validar_archivo(),data_operacion);
			break;
		case CREAR_ARCHIVO:
			pthread_create(&nuevo_hilo,crear_archivo(),data_operacion);
			break;
		case OBTENER_DATOS:
			pthread_create(&nuevo_hilo,obtener_datos(),data_operacion);
			break;
		case GUARDAR_DATOS:
			pthread_create(&nuevo_hilo,guardar_datos(),data_operacion);
			break;

	}
	pthread_detach(nuevo_hilo);
}

