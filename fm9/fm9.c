/*
 ============================================================================
 Name        : fm9.c
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
	t_config* cfg_file = validar_config(argv[1],fm9);
	puts(""); /* prints  */
	return EXIT_SUCCESS;
}