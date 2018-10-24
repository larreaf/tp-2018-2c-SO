#ifndef CONSOLA_H_
#define CONSOLA_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <readline/readline.h>
	#include <readline/history.h>
	#include <unistd.h>
	#include <dirent.h>
	#include <errno.h>
	#include <commons/string.h>



	typedef enum{
		LS,
		CD,
		MD5_CONSOLA,
		CAT,
		EXIT
	} tipo_accion_consola_mdj;

	typedef struct{
		tipo_accion_consola_mdj accion;
		char* argumento;
	}operacionConsolaMDJ;


	void consola_mdj();

	void ejecutar_linea(char* linea);

	operacionConsolaMDJ* parsear_linea(char* linea);

	void destroy_operacion(operacionConsolaMDJ* op_safa);

	tipo_accion_consola_mdj string_to_accion(char* string);

	void con_ls(char* linea);

	void con_cd(char* linea);

	void con_md5(char* linea);

	void con_cat(char* linea);

#endif /* CONSOLA_H_ */
