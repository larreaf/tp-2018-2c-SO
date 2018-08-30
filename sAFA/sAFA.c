/*
 ============================================================================
 Name        : sAFA.c
 Author      : 
 Version     :
 Copyright   : 
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <ensalada/validacion.h>

int main(int argc, char **argv) {
	validar_parametros(argc);
	t_config* cfg_file = validar_config(argv[1],safa);
	cfg_safa* configuracion = asignar_config(cfg_file,safa);

	puts(""); /* prints  */
	return EXIT_SUCCESS;
}
