#include <funciones.h>

void* puntero = NULL;
void* comienzo_storage = NULL;

void inicializar_storage(t_log *logger,int tamanioMemoria){

	log_info(logger, "Inicializando storage según archivo de configuración...");

	comienzo_storage = asignar_memoria(logger, tamanioMemoria);
	//que hace si asigna_memoria devuelve 0 (no pudo asignar)
	// para mi tendriamos que en este caso cerrar fm9 o volver a solicitarlo cada tanto tiempo con un tope de veces

}

void* asignar_memoria(t_log *logger, int tamanioMemoria){
	//esta funcion deberia estar en ensalada quiza
	puntero = malloc(tamanioMemoria);

	if(puntero == NULL)
	{
		log_info(logger, "No se puede asignar ese espacio de memoria...");
	}
	else
	{
		log_info(logger, "Se asigno correctamente el espacio de memoria necesario...");
	}

	return puntero;
}

void desempaquetar_mensaje_diego_a_fm9(t_log *logger, MensajeDinamico* mensaje, DiegoAFM9* DiegoAFM9){
	log_info(logger, "Desempaquetando mensaje de Diego a FM9...");

	recibir_int(&DiegoAFM9->id, mensaje);
    recibir_string(&DiegoAFM9->instruccion, mensaje);
    recibir_string(&DiegoAFM9->contenido, mensaje);
}

//Distintos tipos de funcionamiento de la memoria, definidos en el arch de config

void segmentacion_pura(){
	//tabla de segmentos tiene que ser variable global id/limite/base
	//ver el tamaño de la linea
}

void tabla_de_paginas_invertidas(){
   //tabla de paginas invertidas indice/pid/pagina
}

void segmentacion_paginada(){
	//tabla de procesos
	//tabla de paginas
	//tabla de segmentos
}

void elegir_tipo_funcionamiento_memoria(t_log *logger,char unTipoFuncionamiento){
	log_info(logger, "Eligiendo el tipo de funcionamiento de la memoria...");

	switch(unTipoFuncionamiento){
		case "seg":
			log_info(logger, "Inicializando segmentación pura...");
			segmentacion_pura();
		break;
		case "pag_inv":
			log_info(logger, "Inicializando paginación invertida...");
			tabla_de_paginas_invertidas();
		break;
		case "seg_pag":
			log_info(logger, "Inicializando segmentación con paginación...");
			segmentacion_paginada();
		break;
		default:
			log_info(logger, "Error al querer elegir el tipo de funcionamiento de la memoria...");
		break;
	}
}

// Acciones según las instrucciones recibidas por el Diego

char* buscar_en_memoria(t_log *logger, DiegoAFM9* datos_fm9){
	return "hola";
}

int escribir_en_memoria(t_log *logger, DiegoAFM9* datos_fm9){
	return 1;
}

int modificar_en_memoria(t_log *logger, DiegoAFM9* datos_fm9){
	return 1;
}

void destruir_storage(){
	free(comienzo_storage);
	free(puntero);
}

void cerrar_fm9(t_log* logger, cfg_fm9* configuracion, ConexionesActivas server){
    log_info(logger, "Cerrando FM9...");

    // destruir_conexiones_activas manda headers CONEXION_CERRADA a todos los clientes conectados para que se enteren y despues
    // cierra cada socket
    destruir_conexiones_activas(server);
    log_destroy(logger);
    destroy_cfg(configuracion, t_fm9);
    destruir_storage();
    exit(0);
}
