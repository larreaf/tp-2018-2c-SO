

metadata_fifa levantar_metadata(char* pto_montaje){

	metadata_fifa metadata;

	char metadata_ruta_relativa[] = "Metadata/Metadata.bin";
	char* ruta = malloc(strlen(pto_montaje)+ strlen(metadata_ruta_relativa));

	memcpy(ruta,pto_montaje,strlen(pto_montaje));
	memcpy(ruta+strlen(pto_montaje),metadata_ruta_relativa,strlen(metadata_ruta_relativa));


	t_config* metadata_cfg = config_create(ruta);
	metadata.cantidad_bloques = config_get_int_value(metadata_cfg,"TAMANIO_BLOQUES");
	metadata.tamanio_bloques = config_get_int_value(metadata_cfg,"CANTIDAD_BLOQUES");

	config_destroy(metadata_cfg);
	free(ruta);

	return metadata;
}


void crear_bloques(int cantidad_bloques){

}
