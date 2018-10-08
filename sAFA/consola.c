#include "types.h"
#include "plp.h"

int correr_consola = 1;
extern PLP* plp;

void* consola_safa(void* arg){
	while(correr_consola){
		//signal(SIGINT, sig_handler);
		//signal(SIGTERM, sig_handler);
		//signal(SIGKILL, sig_handler);

		char *linea_leida = readline(">");

		add_history(linea_leida);

		ejecutar_linea(linea_leida);

		free(linea_leida);

	}

	//raise(SIGINT);
	return NULL;
}

void ejecutar_linea(char* linea){
	operacionConsolaSafa* op_consola = parsear_linea(linea);
	switch (op_consola->accion){
		case EJECUTAR:
			con_ejecutar(op_consola->argumento);
			destroy_operacion(op_consola);
			break;

		case STATUS:
			con_status(-1);
			destroy_operacion(op_consola);
			break;

		case FINALIZAR:
			con_finalizar(0);
			destroy_operacion(op_consola);
			break;

		case METRICAS:
			con_metricas(0);
			destroy_operacion(op_consola);
			break;

		case EXIT:
			destroy_operacion(op_consola);
			correr_consola = 0;
			break;

		default:

			printf("Operacion incorrecta\n");
			break;
	}
}

operacionConsolaSafa* parsear_linea(char* linea){
	operacionConsolaSafa* retorno = malloc(sizeof(operacionConsolaSafa));
	retorno->accion = 9999;
	retorno->argumento = malloc(1);
	int offset = 0;
	int i = 0;
	char* word;

	while((word = strsep(&linea," ")) != NULL && retorno->accion != -1)
	{
		if (i == 0)
		{
			retorno->accion = string_to_accion(word);
		}
		else {
			int longitudWord = strlen(word);
			retorno->argumento = realloc(retorno->argumento, (size_t)offset+longitudWord+1);
			memcpy(retorno->argumento+offset, word, (size_t)longitudWord);
			offset += longitudWord;
			retorno->argumento[offset] = ' ' ;
		}

		i++;
	}
	retorno->argumento[offset] = '\0' ;

	return retorno;
}

void destroy_operacion(operacionConsolaSafa* op_safa){
	free(op_safa->argumento);
	free(op_safa);
	return;
}

tipo_accion_consola_safa string_to_accion(char* string){
	if(!strcmp(string,"ejecutar"))
			return EJECUTAR;
	if(!strcmp(string,"status"))
			return STATUS;
	if(!strcmp(string,"finalizar"))
			return FINALIZAR;
	if(!strcmp(string,"metricas"))
			return METRICAS;
	if(!strcmp(string,"exit"))
				return EXIT;
	return -1;
}


void con_ejecutar(char* ruta_escriptorio){
    DTB* nuevo_dtb = crear_dtb(contador_id_dtb++, 1);
	log_info(plp->logger, "Creando DTB %d", nuevo_dtb->id);
	string_append(&nuevo_dtb->path_script, ruta_escriptorio);

    agregar_a_new(plp, nuevo_dtb);

	return;
}

void con_status(int id_DTB){
    if(id_DTB == -1){
        imprimir_estado(plp);
    }
	return;
}

void con_finalizar(int id_DTB){
	return;
}

void con_metricas(int id_DTB){
	return;
}
