#include "mdj_functions.h"
#define TAM_LINEA		150// TODO corregir esto y recibirlo por el diego
extern t_bitarray* bitmap;
extern metadata_fifa metadata;
extern cfg_mdj* configuracion;



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
char* get_bitmap_to_string(){
	int i = 0;
	char* bitmap_string = string_new();
	string_append(&bitmap_string,"[");
	bool valor_bit;
	int max = bitarray_get_max_bit(bitmap);
	for(i = 0 ; i < max; i++ ){
		valor_bit = bitarray_test_bit(bitmap,i);
		(valor_bit) ?
		string_append(&bitmap_string,"1"):
		string_append(&bitmap_string,"0");
		if(i < (max-1)){
			string_append(&bitmap_string,",");
		}

	}
	string_append(&bitmap_string,"]");
	return bitmap_string;
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

void inicializar_bloque(t_mdj_interface* mdj_interface){

	mdj_interface->size = mdj_interface->cantidad_bytes;
	mdj_interface->offset = 0;
	char* path_fifa_bloques = obtener_path_bloques_fifa();
	char* path_fifa_archivos = obtener_path_archivo_fifa();

	int size = mdj_interface->size;
	/**
     * Obtener path absoluto del archivo
     */
	char* path_absoluto = string_new();
	string_append_with_format(&path_absoluto,"%s%s",path_fifa_archivos,mdj_interface->path);

	/*
     * Obtener los bloques que conforman el archivo
     */
	t_config* toGet_cfg = config_create(path_absoluto);
	char** bloques = obtener_bloques(toGet_cfg);
	char* path_bloque;

	if(mdj_interface->size <= 0){
		mdj_interface->size = 10000;
	}

	/*
     * Leer de a bytes
     */
	FILE* ptr_filebloque;
	int index = 0;

	while(bloques[index]!=NULL){
		/**
         * Obtener path de un bloque
         */
		path_bloque = string_new();
		string_append_with_format(&path_bloque,"%s%s.bin",path_fifa_bloques,bloques[index]);
		/**
         * Abrir bloque
         */
		ptr_filebloque = fopen(path_bloque,"r+");
		fseek(ptr_filebloque,0,SEEK_SET);
		int lineas = 0;
		/*
         * Guardar bytes
         */
		while( lineas < metadata.tamanio_bloques-1 && size > 0){
			fprintf(ptr_filebloque,"%c",'\n');
			lineas++;
			size--;
		}

		/*
         * Cerrar bloque y liberar memoria
         */
		fclose(ptr_filebloque);
		free(path_bloque);
		index++;
	}//while (para recorrer bloque por bloque)
	index = 0;
	while(bloques[index]!=NULL){
		free(bloques[index]);
		index++;
	}//while (liberar memoria de char** bloques)
	free(bloques);
	free(path_fifa_archivos);
	free(path_fifa_bloques);
	free(path_absoluto);
	config_destroy(toGet_cfg);
	/*if(lineas_obtenidas[string_length(lineas_obtenidas)-1] != '\n'){
        string_append(&lineas_obtenidas,"\n");
    }*/

}

bool validar_archivo(t_mdj_interface* mdj_interface){
	bool ret;
	char* path_fifa_archivos = obtener_path_archivo_fifa(mdj_interface->path);
	char* path_absoluto = string_new();

	if(mdj_interface->path[0] == '/'){
		mdj_interface->path[0] = ' ';
		string_trim(&mdj_interface->path);
	}

	string_append_with_format(&path_absoluto,"%s%s",path_fifa_archivos,mdj_interface->path);
	FILE* ptr = fopen(path_absoluto, "r");
	if(ptr == NULL){
		//	printf("Archivo no existe!\n");
		ret = false;
	}else{
		fclose(ptr);
		ret = true;
	}
	free(path_fifa_archivos);
	free(path_absoluto);

	return ret;
}

int crear_archivo(t_mdj_interface* mdj_interface){
	t_config* config_nuevo_archivo;
	char* path_fifa_archivos = obtener_path_archivo_fifa(mdj_interface->path);
	char* path_absoluto = string_new();
	string_append_with_format(&path_absoluto,"%s%s",path_fifa_archivos,mdj_interface->path);
	char** directorios = string_split(mdj_interface->path,"/");
	int index = 0;
	int bytes_asignados = 0;

	int cantidad_bloques = mdj_interface->cantidad_bytes/metadata.tamanio_bloques;

	if(mdj_interface->cantidad_bytes%metadata.tamanio_bloques){
		cantidad_bloques++;
	}

	char* aux = string_new();
	string_append_with_format(&aux,"%s",path_fifa_archivos);
	/*
	 * Crear directorios intermedios necesarios
	 */
	while(directorios[index+1] != NULL){
		string_append(&aux,directorios[index]);
		mkdir(aux,0777);
		string_append(&aux,"/");
		index++;
	}
	/*
	 * Crear archivo
	 */
	FILE* f = fopen(path_absoluto,"w");
	fclose(f);
	/*
	 * Inicializar su metadata de bloques asignados
	 */
	char* bloques = string_new();
	string_append(&bloques,"[");

	/*
	 * Asignar los bloques
	 */
	for(index = 0; index < bitarray_get_max_bit(bitmap) && cantidad_bloques!=0 ; index++){
		if(!bitarray_test_bit(bitmap,index)){
			bitarray_set_bit(bitmap,index);
			string_append_with_format(&bloques,"%d",index);
			if(cantidad_bloques!=1 && index+1 < bitarray_get_max_bit(bitmap)){
				string_append(&bloques,",");
			}
			cantidad_bloques--;
			bytes_asignados += metadata.tamanio_bloques;
		}
	}

	string_append(&bloques,"]");

	/*
	 * Guardar la metadata del archivo
	 */
	config_nuevo_archivo = config_create(path_absoluto);
	char* tamanio = string_itoa(mdj_interface->cantidad_bytes);
	config_set_value(config_nuevo_archivo,"TAMANIO",tamanio);
	free(tamanio);
	config_set_value(config_nuevo_archivo,"BLOQUES",bloques);
	config_save(config_nuevo_archivo);

	/*
	 * Inicializar los bloques
	 */
	//TODO index >= bitarray_get_max_bit(bitmap) no alcanza el espacio para guardar el archivo
	if(index >= bitarray_get_max_bit(bitmap)){
		borrar_archivo(mdj_interface);
		bytes_asignados = -1;
	}else {
		inicializar_bloque(mdj_interface);
	}
	/*
	 * Liberar la memoria
	 */
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
	/*
	 * Retornar los bytes asignados
	 */
	return ((cantidad_bloques == 0) ? mdj_interface->cantidad_bytes : bytes_asignados);
}

int borrar_archivo(t_mdj_interface* mdj_interface){
	char* path_fifa_archivos = obtener_path_archivo_fifa();
	char* path_absoluto = string_new();
	string_append_with_format(&path_absoluto,"%s%s",path_fifa_archivos,mdj_interface->path);
	t_config* toDelete_cfg = config_create(path_absoluto);

	char** bloques = obtener_bloques(toDelete_cfg);

	int index = 0;


	while(bloques[index]!=NULL){
		bitarray_clean_bit(bitmap,atoi(bloques[index]));
		index++;
	}

	config_destroy(toDelete_cfg);


	//printf("[BORRAR] path_absoluto%s\n",path_absoluto);
	index = 0;
	while(bloques[index]!=NULL){
		free(bloques[index]);
		index++;
	}
	free(bloques);
	//Borrar archivo {0: exito, -1: error}
	int return_value = remove(path_absoluto);

	free(path_fifa_archivos);
	free(path_absoluto);


	return return_value;

}


char* obtener_datos(t_mdj_interface* mdj_interface){

	char* lineas_obtenidas = string_new();
	char* path_fifa_bloques = obtener_path_bloques_fifa();
	char* path_fifa_archivos = obtener_path_archivo_fifa();

	int size = mdj_interface->size ;
	/**
	 * Obtener path absoluto del archivo
	 */
	char* path_absoluto = string_new();
	string_append_with_format(&path_absoluto,"%s%s",path_fifa_archivos,mdj_interface->path);

	/*
	 * Obtener los bloques que conforman el archivo
	 */

	t_config* toGet_cfg = config_create(path_absoluto);
	char** bloques = obtener_bloques(toGet_cfg);
	char* path_bloque;

	if(size <= 0){
		size = 10000;
	}
	FILE* ptr_filebloque;
	/*
	 * Leer de a bytes
	 */
	int i = 0;
	int index = 0;
	int caracter = 2;
	while(bloques[index]!=NULL){
		/**
		 * Obtener path de un bloque
		 */
		path_bloque = string_new();
		string_append_with_format(&path_bloque,"%s%s.bin",path_fifa_bloques,bloques[index]);
		/**
		 * Abrir bloque
		 */
		ptr_filebloque = fopen(path_bloque,"r");
		/*
		 * Moverse al offset en bytes
		 */
		for(i = 0; i < mdj_interface->offset && fseek(ptr_filebloque,1,SEEK_CUR) != EOF ; i++){	}//for (apuntar al offset)
		/*
		 * Leer y guardar bytes
		 */
		while((caracter = fgetc(ptr_filebloque)) != EOF && size > 0){
			char c = ((char) caracter);
			string_append_with_format(&lineas_obtenidas,"%c", c);
			size--;
		}//while (lectura de bytes de un bloque)
		if(i >= mdj_interface->offset && caracter == EOF){
			//string_append(&lineas_obtenidas,"\n");
		}//if (agregar \n cuando es EOF) TODO por ahora está comentado
		/*
		 * Cerrar bloque y liberar memoria
		 */
		fclose(ptr_filebloque);
		free(path_bloque);
		index++;
	}//while (para recorrer bloque por bloque)
	index = 0;
	while(bloques[index]!=NULL){
		free(bloques[index]);
		index++;
	}//while (liberar memoria de char** bloques)
	free(bloques);
	free(path_fifa_archivos);
	free(path_fifa_bloques);
	free(path_absoluto);
	config_destroy(toGet_cfg);
	/*if(lineas_obtenidas[string_length(lineas_obtenidas)-1] != '\n'){
		string_append(&lineas_obtenidas,"\n");
	}*/

	return lineas_obtenidas;

}

int guardar_datos(t_mdj_interface* mdj_interface){
	char* path_fifa_bloques = obtener_path_bloques_fifa();
	char* path_fifa_archivos = obtener_path_archivo_fifa();

	int size;
	/**
	 * Obtener path absoluto del archivo
	 */
	char* path_absoluto = string_new();
	string_append_with_format(&path_absoluto,"%s%s",path_fifa_archivos,mdj_interface->path);

	/*
	 * Obtener los bloques que conforman el archivo
	 */
	t_config* toGet_cfg = config_create(path_absoluto);
	int tamanio_archivo = config_get_int_value(toGet_cfg,"TAMANIO");
	char** bloques = obtener_bloques(toGet_cfg);
	char* path_bloque;

	if(mdj_interface->size <= 0){
		size = string_length(mdj_interface->buffer);
		mdj_interface->size = size;
	}else{
		size = mdj_interface->size;
	}

	FILE* ptr_filebloque;
	/*
	 * Leer de a bytes
	 */
	int i = 0;
	int index = 0;
	int buffer_index = 0;
	int caracter = 0;

	while(bloques[index]!=NULL){
		int saltos_de_linea_bloque_totales = 0;
		/**
		 * Obtener path de un bloque
		 */
		path_bloque = string_new();
		string_append_with_format(&path_bloque,"%s%s.bin",path_fifa_bloques,bloques[index]);
		/**
		 * Abrir bloque
		 */
		ptr_filebloque = fopen(path_bloque,"r+");
		fseek(ptr_filebloque,0,SEEK_SET);
		/*
		 * Contar saltos de linea del bloque
		 */
		while( (caracter = fgetc(ptr_filebloque)) != EOF ){
			if(caracter == '\n'){
				saltos_de_linea_bloque_totales++;
			}
		}
		fseek(ptr_filebloque,0,SEEK_SET);
		/*
		 * Moverse al offset en bytes
		 */
		int saltos_de_linea_buffer = 0;
		if( i < mdj_interface->offset ){
			for(i = 0; i < mdj_interface->offset && (caracter = fgetc(ptr_filebloque)) != EOF ; i++){
				if(caracter == '\n'){
					saltos_de_linea_buffer++;
				}
			}//for (apuntar al offset)
		}
		/*
		 * Guardar bytes
		 */

		while( (saltos_de_linea_buffer < saltos_de_linea_bloque_totales || (saltos_de_linea_buffer == saltos_de_linea_bloque_totales && mdj_interface->buffer[buffer_index] != '\n') )
				&& (size > 0 || mdj_interface->buffer[buffer_index] != '\0')
		){
			if(mdj_interface->buffer[buffer_index] == '\n'){
				saltos_de_linea_buffer++;
			}
			fprintf(ptr_filebloque,"%c",mdj_interface->buffer[buffer_index]);
			buffer_index++;
			size--;
		}
		/*if(mdj_interface->buffer[buffer_index] == '\n'){
			buffer_index++;
		}*/
		/*
		 * Cerrar bloque y liberar memoria
		 */
		fclose(ptr_filebloque);
		free(path_bloque);
		index++;
	}//while (para recorrer bloque por bloque)
	index = 0;
	while(bloques[index]!=NULL){
		free(bloques[index]);
		index++;
	}//while (liberar memoria de char** bloques)
	free(bloques);
	free(path_fifa_archivos);
	free(path_fifa_bloques);
	free(path_absoluto);
	/*
	 * Guardar tamaño nuevo en bytes
	 */
	tamanio_archivo += (mdj_interface->size - size);
	char* tam = string_new();
	string_append_with_format(&tam,"%d",tamanio_archivo);
	config_set_value(toGet_cfg,"TAMANIO",tam);
	config_save(toGet_cfg);
	free(tam);
	config_destroy(toGet_cfg);
	/*if(lineas_obtenidas[string_length(lineas_obtenidas)-1] != '\n'){
		string_append(&lineas_obtenidas,"\n");
	}*/
	return (mdj_interface->size - size);
}


t_mdj_interface* crear_data_mdj_operacion(MensajeDinamico* mensaje){
	t_mdj_interface* data_mdj_operacion = malloc(sizeof(t_mdj_interface));
	//NodoPayload* nodo_aux = queue_pop(mensaje->payload);
	//data_mdj_operacion->path = nodo_aux->datos;
	data_mdj_operacion->t_operacion = mensaje->header;
	switch(data_mdj_operacion->t_operacion){
		case VALIDAR_ARCHIVO:
			recibir_string(&data_mdj_operacion->path,mensaje);
			break;
		case CREAR_ARCHIVO:
			recibir_string(&data_mdj_operacion->path, mensaje);
			recibir_int(&data_mdj_operacion->cantidad_bytes, mensaje);
			break;
		case OBTENER_DATOS:
			recibir_string(&data_mdj_operacion->path,mensaje);
			recibir_int(&data_mdj_operacion->offset, mensaje);
			recibir_int(&data_mdj_operacion->size, mensaje);
			break;
		case GUARDAR_DATOS:
			recibir_string(&data_mdj_operacion->path,mensaje);
			recibir_int(&data_mdj_operacion->offset, mensaje);
			recibir_int(&data_mdj_operacion->size, mensaje);
			recibir_string(&data_mdj_operacion->buffer,mensaje);
			break;
		case BORRAR_ARCHIVO:
			recibir_string(&data_mdj_operacion->path,mensaje);
			break;
		default:
			//TODO log_error();
			break;

	}
	return data_mdj_operacion;

}

void interface_mdj(MensajeDinamico* mensaje_dinamico){

	t_mdj_interface* data_operacion = crear_data_mdj_operacion(mensaje_dinamico);

	switch(mensaje_dinamico->header){

		case VALIDAR_ARCHIVO:
			validar_archivo(data_operacion);
			break;
		case CREAR_ARCHIVO:
			crear_archivo(data_operacion);
			break;
		case OBTENER_DATOS:
			obtener_datos(data_operacion);
			break;
		case GUARDAR_DATOS:
			//TODO	guardar_datos(data_operacion);
			break;
		case BORRAR_ARCHIVO:
			borrar_archivo(data_operacion);
			break;
	}

}

