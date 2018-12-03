
#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/config.h>

void validar_parametros(int argc){
	if(argc < 3){
		printf("La cantidad de parametros es incorrecta!\n");
		exit(1);
	}
}

int main(int argc, char **argv) {
	t_config* configuracion = config_create(argv[1]);
	if(configuracion != NULL){
		if(config_has_property(configuracion,argv[2])){
			config_set_value(configuracion,argv[2],argv[3]);
			config_save(configuracion);
			printf("%s seteado a: %s\n",argv[2],argv[3]);
		}else {
			printf("El archivo no tiene la propiedad: %s\n",argv[2]);
		}
		config_destroy(configuracion);
	} else {
		printf("El archivo no existe\n");
	}
	return EXIT_SUCCESS;
}
