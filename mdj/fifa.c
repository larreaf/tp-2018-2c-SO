#include "mdj_functions.h"
extern cfg_mdj* configuracion;
extern metadata_fifa metadata;

void crear_bloques(char* pto_montaje, int cantidad_bloques){
	int i=0;
	char bloques[] = "Bloques";
	for(i = 0; i < cantidad_bloques; i ++){
		char* bloque_name = malloc(strlen(pto_montaje)+strlen(bloques)+10);
		sprintf(bloque_name,"%s/%s/%d.bin",pto_montaje,bloques,i);
		FILE* ptr = fopen(bloque_name,"a");
		fclose(ptr);
		free(bloque_name);
	}
}

void levantar_metadata(t_log* logger){

	char metadata_ruta_relativa[] = "Metadata/Metadata.bin";
	char* ruta = string_new();
	string_append_with_format(&ruta,"%s/%s",configuracion->punto_montaje,metadata_ruta_relativa);
//	printf("\nruta: %s\n",ruta);
	t_config* metadata_cfg = config_create(ruta);
	if(metadata_cfg == NULL){
		log_error(logger, "No se encontró el archivo %s", ruta);
		exit(-1);
	}else{
		metadata.tamanio_bloques = config_get_int_value(metadata_cfg,"TAMANIO_BLOQUES");

		metadata.cantidad_bloques = config_get_int_value(metadata_cfg,"CANTIDAD_BLOQUES");

		crear_bloques(configuracion->punto_montaje, metadata.cantidad_bloques);
	}
	//config_destroy(metadata_cfg);
	free(ruta);

}




