#include "mdj_functions.h"
#define TAM_LINEA		35
extern t_bitarray* bitmap;
extern metadata_fifa metadata;
extern cfg_mdj* configuracion;
extern pthread_mutex_t mutex_bitmap;


void bitmap_clean(){
	/**
	 * Limpiar bitmap
	 */
	int i = 0;
	int max = bitarray_get_max_bit(bitmap);
	for(i = 0 ; i < max; i++ ){
		bitarray_clean_bit(bitmap,i);
	}
}

char* obtener_path_archivo_fifa(){
	char archivos[] = "Archivos";
	char* path_fifa_archivos = string_new();
	string_append_with_format(&path_fifa_archivos,"%s/%s/",configuracion->punto_montaje,archivos);

	return path_fifa_archivos;
}

char* obtener_path_bloques_fifa(){
	char bloques[] = "Bloques";
	char* path_fifa_bloques = string_new();
	string_append_with_format(&path_fifa_bloques,"%s/%s/",configuracion->punto_montaje,bloques);

	return path_fifa_bloques;
}
char** obtener_bloques(t_config* cfg){
	char* aux = config_get_string_value(cfg,"BLOQUES");
	char* bloques_string = strdup(aux);
	bloques_string[0] = ' ';
	bloques_string[string_length(bloques_string)-1] = ' ';
	string_trim(&bloques_string);
	char** bloques = string_split(bloques_string,",");
	free(bloques_string);
	return bloques;

}

bool validar_archivo(t_mdj_interface* mdj_interface){
	bool ret;
	char* path_fifa_archivos = obtener_path_archivo_fifa(mdj_interface->path);
	char* path_absoluto = string_new();
	string_append_with_format(&path_absoluto,"%s%s",path_fifa_archivos,mdj_interface->path);
	FILE* ptr = fopen(path_absoluto, "r");
	if(ptr == NULL){
		printf("Archivo no existe!\n");
		ret = false;
	}else{
		fclose(ptr);
		ret = true;
	}
	free(path_fifa_archivos);
	free(path_absoluto);

	return ret;
}

void crear_archivo(t_mdj_interface* mdj_interface){
	t_config* config_nuevo_archivo;
	char* path_fifa_archivos = obtener_path_archivo_fifa(mdj_interface->path);
	char* path_absoluto = string_new();
	string_append_with_format(&path_absoluto,"%s%s",path_fifa_archivos,mdj_interface->path);
	char** directorios = string_split(mdj_interface->path,"/");
	int index = 0;


	int cantidad_bloques = mdj_interface->cantidad_lineas/metadata.tamanio_bloques;
	if(mdj_interface->cantidad_lineas%metadata.tamanio_bloques){
		cantidad_bloques++;
	}

	char* aux = string_new();
	string_append_with_format(&aux,"%s",path_fifa_archivos);
	while(directorios[index+1] != NULL){
		string_append(&aux,directorios[index]);
		mkdir(aux,0777);
		string_append(&aux,"/");
		index++;
	}
	FILE* f = fopen(path_absoluto,"w");
	fclose(f);
	char* bloques = string_new();
	string_append(&bloques,"[");

	/**
	 * Comienza Región crítica
	 */
	pthread_mutex_lock(&mutex_bitmap);
	for(index = 0; index < bitarray_get_max_bit(bitmap) && cantidad_bloques!=0 ; index++){
		if(!bitarray_test_bit(bitmap,index)){
			bitarray_set_bit(bitmap,index);
			string_append_with_format(&bloques,"%d",index);
			if(cantidad_bloques!=1){
				string_append(&bloques,",");
			}
			cantidad_bloques--;
		}
	}
	//TODO index >= bitarray_get_max_bit(bitmap) no alcanza el espacio para guardar el archivo
	pthread_mutex_unlock(&mutex_bitmap);
	/**
	 * Termina Región crítica
	 */

	string_append(&bloques,"]");
	config_nuevo_archivo = config_create(path_absoluto);
	char* tamanio = string_itoa(mdj_interface->cantidad_lineas);
	config_set_value(config_nuevo_archivo,"TAMANIO",tamanio);
	free(tamanio);
	config_set_value(config_nuevo_archivo,"BLOQUES",bloques);
	config_save(config_nuevo_archivo);

	config_destroy(config_nuevo_archivo);
	index = 0;
	while(directorios[index]!=NULL){
		free(directorios[index]);
		index++;
	}
	free(directorios);
	free(bloques);
	free(aux);
	free(path_fifa_archivos);
	free(path_absoluto);

}

void borrar_archivo(t_mdj_interface* mdj_interface){
	char* path_fifa_archivos = obtener_path_archivo_fifa();
	char* path_absoluto = string_new();
	string_append_with_format(&path_absoluto,"%s%s",path_fifa_archivos,mdj_interface->path);
	t_config* toDelete_cfg = config_create(path_absoluto);

	char** bloques = obtener_bloques(toDelete_cfg);

	int index = 0;
	/**
	 * Comienza Región crítica
	 */
	pthread_mutex_lock(&mutex_bitmap);
	while(bloques[index]!=NULL){
		bitarray_clean_bit(bitmap,atoi(bloques[index]));
		index++;
	}
	pthread_mutex_unlock(&mutex_bitmap);
	/**
	* Termina Región crítica
	*/
	config_destroy(toDelete_cfg);


	//printf("[BORRAR] path_absoluto%s\n",path_absoluto);
	index = 0;
	while(bloques[index]!=NULL){
		free(bloques[index]);
		index++;
	}
	free(bloques);
	remove(path_absoluto);
	free(path_fifa_archivos);
	free(path_absoluto);

}


char* obtener_datos(t_mdj_interface* mdj_interface){

	char* lineas_obtenidas = string_new();
	char* path_fifa_bloques = obtener_path_bloques_fifa();
	char* path_fifa_archivos = obtener_path_archivo_fifa();

	char* path_absoluto = string_new();
	string_append_with_format(&path_absoluto,"%s%s",path_fifa_archivos,mdj_interface->path);

	t_config* toGet_cfg = config_create(path_absoluto);
	char** bloques = obtener_bloques(toGet_cfg);
	//TODO obtener_tamaño para leer
	int index = mdj_interface->offset/metadata.tamanio_bloques;
	int desplazamiento = mdj_interface->offset%metadata.tamanio_bloques;

	if(mdj_interface->size <= 0){
		mdj_interface->size = 10000;
	}

	FILE* ptr_filebloque;
	while(bloques[index]!=NULL && mdj_interface->size>0){
		int jndex = 1;
		char* path_bloque = string_new();
		string_append_with_format(&path_bloque,"%s%s.bin",path_fifa_bloques,bloques[index]);
		ptr_filebloque = fopen(path_bloque,"r");
		char* buffer = malloc(TAM_LINEA+1);
		while(fgets(buffer, TAM_LINEA+1, ptr_filebloque) != NULL && mdj_interface->size>0){
			if(jndex > desplazamiento && jndex <= metadata.tamanio_bloques){
				string_append_with_format(&lineas_obtenidas,"%s",buffer);
				mdj_interface->size--;
			}
			jndex++;

		}
		desplazamiento = -999;
		if(lineas_obtenidas[string_length(lineas_obtenidas)-1] != '\n'){
			string_append(&lineas_obtenidas,"\n");
		}


		fclose(ptr_filebloque);
		free(buffer);
		free(path_bloque);
		index++;
	}
	index = 0;
	while(bloques[index]!=NULL){
		free(bloques[index]);
		index++;
	}
	free(bloques);
	free(path_fifa_archivos);
	free(path_fifa_bloques);
	free(path_absoluto);

	config_destroy(toGet_cfg);
	//printf("%s",lineas_obtenidas);
	return lineas_obtenidas;

}
/*
char* obtener_path_archivo_fifa(char* pto_montaje, char* path_relativo){
	char archivos[] = "Archivos";
	char* path_fifa = malloc(strlen(pto_montaje)+strlen(path_relativo)+strlen(archivos)+3); // +2 por los '/' +1 por '\0'
	sprintf(path_fifa,"%s/%s/%s", pto_montaje, archivos, path_relativo);

	return path_fifa;
}

void validar_archivo(t_mdj_interface* data_operacion, char* pto_montaje){
	char* path_fifa = obtener_path_archivo_fifa(pto_montaje,data_operacion->path);
	FILE* ptr = fopen(path_fifa, "r");
		if(ptr == NULL){
			printf("Archivo no existe!\n");
			exit(200);
		}else{
			fclose(ptr);
		}
}

void crear_archivo(t_mdj_interface* data_operacion, char* pto_montaje){
	char* path_fifa = obtener_path_archivo_fifa(pto_montaje,data_operacion->path);
	char** directorios = string_split(data_operacion->path);
	int index = 0;
	int malloc_size = strlen(pto_montaje)+strlen("/");

	char* aux = string_new();
	string_append(&aux, pto_montaje);
	string_append(&aux,"/");
	while(directorios[index+1] != NULL){
		string_append(&aux,directorios[index]);
		mkdir(aux,0777);
		index++;
	}
	FILE* f = fopen(path_fifa,"a");


	fclose(f);
	free(aux);


}

char* obtener_datos(t_mdj_interface* data_operacion){
	char* datos = NULL;
	return datos;
}

int guardar_datos(t_mdj_interface* data_operacion){
	return 0;
}

void borrar_archivo(t_mdj_interface* data_operacion){

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
			pthread_create(&nuevo_hilo, NULL,(void*)validar_archivo(),data_operacion);
			break;
		case CREAR_ARCHIVO:
			pthread_create(&nuevo_hilo, NULL,(void*)crear_archivo(),data_operacion);
			break;
		case OBTENER_DATOS:
			pthread_create(&nuevo_hilo, NULL,(void*)obtener_datos(),data_operacion);
			break;
		case GUARDAR_DATOS:
			pthread_create(&nuevo_hilo, NULL,(void*)guardar_datos(),data_operacion);
			break;
		case BORRAR_ARCHIVO:
			pthread_create(&nuevo_hilo, NULL,(void*)borrar_archivo(),data_operacion);
			break;
	}
	pthread_detach(nuevo_hilo);
}*/

