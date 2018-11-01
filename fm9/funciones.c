#include "funciones.h"

/*
	Variables const: textos para los logs
*/

char string_inicializar_config_fm9[] = "Inicializando config de fm9...";
char string_inicializar_conexiones_activas[] = "Inicializando conexiones_activas...";
char string_inicializar_storage[] = "Inicializando storage según archivo de configuración...";
char string_reinicializar_storage[] = "Error inicialización memoria, reintento de inicialización de storage...";
char string_inicializar_diccionario_estado_lineas_storage[] = "Inicializando diccionario estado de lineas storage...";
char string_reinicializar_storage_error[] = "Error inicialización memoria, se acabaron los intentos...";
char string_desempaquetar_mje_diego_buscar[] = "Desempaquetando mensaje tipo buscar de Diego a FM9...";
char string_datos_entrantes_diego_fm9_tipo_buscar[] = "Datos entrantes de Diego a fm9 tipo buscar %d...";
char string_desempaquetar_mje_diego_escribir[] = "Desempaquetando mensaje tipo escribir de Diego a FM9...";
char string_datos_entrantes_diego_fm9_tipo_escribir[] = "Datos entrantes de Diego a fm9 tipo escribir %d...";
char string_desempaquetar_mje_diego_modificar[] = "Desempaquetando mensaje tipo modificar de Diego a FM9...";
char string_datos_entrantes_diego_fm9_tipo_modificar[] = "Datos entrantes de Diego a fm9 tipo modificar %d...";


/*
	Variables const: contadores
*/

int cantidad_de_reintentos_ini_storage = 5;
int reintentar_inicializar_storage_ok = 1;

/*
	Funciones
*/

void inicializar_config_fm9(int argc, char **argv){
	log_info(logger, string_inicializar_config_fm9);
	validar_parametros(argc);
	config_general_fm9 = asignar_config(argv[1], fm9);
}

void inicializar_conexiones_activas_fm9(){
	int conexiones_permitidas[cantidad_tipos_procesos] = {0};

	log_info(logger,string_inicializar_conexiones_activas);
	conexiones_permitidas[t_cpu] = 1;
	conexiones_permitidas[elDiego] = 1;
	conexiones_activas = inicializar_conexiones_activas(logger, config_general_fm9->puerto, conexiones_permitidas, t_fm9);
}

void inicializar_storage(){

	log_info(logger, string_inicializar_storage);
	base_storage = malloc(config_general_fm9->tamanio);
	crear_diccionario_estado_lineas_storage();
	if(base_storage == 0 && reintentar_inicializar_storage_ok){
		reintentar_inicializar_storage();
	}

}

void crear_diccionario_estado_lineas_storage(){
	log_info(logger, string_inicializar_diccionario_estado_lineas_storage);
	diccionario_estado_lineas_storage = dictionary_create();

	incializar_diccionario_estado_lineas_storage();
}

void incializar_diccionario_estado_lineas_storage(){
	int total_de_lineas_storage = (config_general_fm9->tamanio / config_general_fm9->max_linea);

	for(int nro_linea = 0; nro_linea < total_de_lineas_storage; nro_linea ++){
		dictionary_put(diccionario_estado_lineas_storage, nro_linea, 0);
	}
}

void reintentar_inicializar_storage(){

	if(cantidad_de_reintentos_ini_storage != 0){
		log_info(logger, string_reinicializar_storage);
		cantidad_de_reintentos_ini_storage--;
		inicializar_storage();
	}
	else
	{
		log_info(logger, string_reinicializar_storage_error);
		reintentar_inicializar_storage_ok = 0;
	}
}

void inicializar_fm9(int argc, char **argv){
	//todo crear un log con error de inicialización y que cierre fm9

	// Se crea log
	logger = log_create("fm9.log", "fm9", true, log_level_from_string("info"));

	inicializar_config_fm9(argc, argv);
	inicializar_conexiones_activas_fm9();
	inicializar_storage();

	log_info(logger, "Se completo inicialización fm9...");
}

data_mje_buscar* desempaquetar_mensaje_buscar(MensajeDinamico* mensaje){

	data_mje_buscar *data;

	log_info(logger, string_desempaquetar_mje_diego_buscar);

	recibir_int(&data->idmensaje, mensaje);
    recibir_int(&data->idproceso, mensaje);
    recibir_int(&data->direccion_logica, mensaje);

    log_info(logger, string_datos_entrantes_diego_fm9_tipo_buscar, data->idmensaje);

    return data;
}

data_mje_escribir* desempaquetar_mensaje_escribir(MensajeDinamico* mensaje){

	data_mje_escribir *data;

	log_info(logger, string_desempaquetar_mje_diego_escribir);

	recibir_int(&data->idmensaje, mensaje);
    recibir_int(&data->idproceso, mensaje);
    recibir_string(&data->contenido, mensaje);

    log_info(logger, string_datos_entrantes_diego_fm9_tipo_escribir, data->idmensaje);

    return data;
}

data_mje_modificar* desempaquetar_mensaje_modificar(MensajeDinamico* mensaje){

	data_mje_modificar *data;

	log_info(logger, string_desempaquetar_mje_diego_modificar);

	recibir_int(&data->idmensaje, mensaje);
    recibir_int(&data->idproceso, mensaje);
    recibir_int(&data->direccion_logica, mensaje);
    recibir_string(&data->contenido, mensaje);

    log_info(logger, string_datos_entrantes_diego_fm9_tipo_modificar, data->idmensaje);

    return data;
}

char** buscar_en_memoria(data_mje_buscar* data){
	char** contenido_en_memoria;

	switch (config_general_fm9->modo){
		case SEG:
			contenido_en_memoria = buscar_en_memoria_seg();
		break;
		case TPI:
			contenido_en_memoria = buscar_en_memoria_tpi();
		break;
		case SPA:
			contenido_en_memoria = buscar_en_memoria_spa();
		break;
		default:
		break;
	}

	return contenido_en_memoria;
}

char** escribir_en_memoria(data_mje_buscar* data){
	char** contenido_en_memoria;

	switch (config_general_fm9->modo){
		case SEG:
			contenido_en_memoria = escribir_en_memoria_seg();
		break;
		case TPI:
			contenido_en_memoria = escribir_en_memoria_tpi();
		break;
		case SPA:
			contenido_en_memoria = escribir_en_memoria_spa();
		break;
		default:
		break;
	}

	return contenido_en_memoria;
}

char** modificar_en_memoria(data_mje_buscar* data){
	char** contenido_en_memoria;

	switch (config_general_fm9->modo){
		case SEG:
			contenido_en_memoria = modificar_en_memoria_seg();
		break;
		case TPI:
			contenido_en_memoria = modificar_en_memoria_tpi();
		break;
		case SPA:
			contenido_en_memoria = modificar_en_memoria_spa();
		break;
		default:
		break;
	}

	return contenido_en_memoria;
}

char** buscar_en_memoria_seg(data_mje_buscar* data){
	/*list_get(tabla_de_segmentos,data->idproceso);
	// con la direccion fisica busco en memoria y retorno el valor que hay*/
	return "hola";
}

char** buscar_en_memoria_tpi(data_mje_buscar* data){
	return "hola";
}
char** buscar_en_memoria_spa(data_mje_buscar* data){
	return "hola";
}

char** escribir_en_memoria_seg(data_mje_escribir* data){
	//todo agregar en memoria princial y obtener direccion del segmento
	/*int idsegmento = 1;
	int base_segmento = 100;
	int desplazamiento = 50;
	t_list *tabla_de_segmentos_por_proceso;
	//todo estructura direccion fisica
	list_add(tabla_de_segmentos_por_proceso, direccion_fisica(idsegmento, desplazamiento, base_segmento));
	list_add(tabla_de_segmentos, data->idproceso, tabla_de_segmentos_por_proceso);*/
	return 1;
}

char** escribir_en_memoria_tpi(data_mje_escribir* data){
	return "hola";
}
char** escribir_en_memoria_spa(data_mje_escribir* data){
	return "hola";
}

char** modificar_en_memoria_seg(data_mje_modificar* data){
	return "hola";
}
char** modificar_en_memoria_tpi(data_mje_modificar* data){
	return "hola";
}
char** modificar_en_memoria_spa(data_mje_modificar* data){
	return "hola";
}

void destruir_storage(){
	free(base_storage);
}

void cerrar_fm9(){
    log_info(logger, "Cerrando FM9...");

    // destruir_conexiones_activas manda headers CONEXION_CERRADA a todos los clientes conectados para que se enteren y despues
    // cierra cada socket
    destruir_conexiones_activas(conexiones_activas);
    log_destroy(logger);
    destroy_cfg(config_general_fm9, t_fm9);
    destruir_storage();
    exit(0);
}
