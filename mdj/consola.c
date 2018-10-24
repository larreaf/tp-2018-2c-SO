#include "consola.h"
#include "mdj_functions.h"

char* dir_mdj;
char* dir_fifa_abs;
char* dir_fifa_rel;
char* dir_actual;
void consola_mdj(){
	dir_fifa_rel = string_new();
	string_append(&dir_fifa_rel,"mnt/FIFA_FS/Archivos");
	dir_mdj = get_current_dir_name();
	dir_fifa_abs = strdup(dir_mdj);
	string_append_with_format(&dir_fifa_abs,"/%s",dir_fifa_rel);
	dir_actual = string_new();
	char* prompt = string_new();
	//string_append(dir_actual,"/");
	while(1){
		char *linea_leida;
		prompt = string_new();
		//signal(SIGINT, sig_handler);
		//signal(SIGTERM, sig_handler);
		//signal(SIGKILL, sig_handler);
		string_append_with_format(&prompt,"/%s$ ",dir_actual);

		linea_leida = readline(prompt);

		add_history(linea_leida);

		ejecutar_linea(linea_leida);

		free(linea_leida);
		free(prompt);

	}
}

void ejecutar_linea(char* linea){
	operacionConsolaMDJ* op_consola = parsear_linea(linea);
	switch (op_consola->accion){
		case LS:
			con_ls(op_consola->argumento);
			destroy_operacion(op_consola);
			break;

		case CD:
			con_cd(op_consola->argumento);
			destroy_operacion(op_consola);
			break;

		case MD5_CONSOLA:
			con_md5(op_consola->argumento);
			destroy_operacion(op_consola);
			break;

		case CAT:
			con_cat(op_consola->argumento);
			destroy_operacion(op_consola);
			break;

		case EXIT:
			break;

		default:

			printf("Operacion incorrecta\n");
			break;
	}
}

operacionConsolaMDJ* parsear_linea(char* linea){
	operacionConsolaMDJ* retorno = malloc(sizeof(operacionConsolaMDJ));
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

void destroy_operacion(operacionConsolaMDJ* op_safa){
	free(op_safa->argumento);
	free(op_safa);
	return;
}

tipo_accion_consola_mdj string_to_accion(char* string){
	tipo_accion_consola_mdj retorno = -1;
	if(!strcmp(string,"cd"))
		retorno = CD;
	if(!strcmp(string,"ls"))
		retorno = LS;
	if(!strcmp(string,"md5"))
		retorno = MD5_CONSOLA;
	if(!strcmp(string,"cat"))
		retorno = CAT;
	if(!strcmp(string,"exit"))
		retorno = EXIT;
	return retorno;
}

void con_ls(char* linea){
	char* arg = string_new();
	string_append_with_format(&arg,"ls %s/%s%s",dir_fifa_abs,dir_actual,linea);
	system(arg);
	free(arg);
}


void mover_dir(char* dir){

	if(string_contains(dir,"..")){
		char** aux = string_split(dir_actual,"/");

		int i = 0;
		for(i = 0; aux[i]!= NULL; i++){	}
		i-= 2;
		free(dir_actual);
		dir_actual = string_new();
		if(i < 0){

		}else{
			int j = 0;
			for(j = 0; j <= i; j++){
				string_append_with_format(&dir_actual,"%s/",aux[j]);
			}
		}
		int index = 0;
		while(aux[index]!=NULL){
			free(aux[index]);
			index++;
		}
		free(aux);
	}else{
		char* arg = string_new();
		string_append_with_format(&arg,"%s/%s%s", dir_fifa_rel, dir_actual, dir);

		DIR* directory = opendir(arg);
		if (directory)
		{
			// Directory exists.
			closedir(directory);
			string_append_with_format(&dir_actual,"%s/",dir);
		}else{
			printf("Directorio invalido\n");
		}

		free(arg);
	}
}

void con_cd(char* linea){

	if(string_equals_ignore_case(linea,"")){
		free(dir_actual);
		dir_actual = string_new();
	} else {
		char** dirs = string_split(linea,"/");
		int i = 0;

		for(i = 0; dirs[i]!= NULL; i++){
//printf("%d) %s\n", i, dirs[i]);
			mover_dir(dirs[i]);
		}
		int index = 0;
		while(dirs[index]!=NULL){
			free(dirs[index]);
			index++;
		}
		free(dirs);
	}


}

void con_md5(char* linea){
	char* lineas_leidas;
	char* file_aux = string_new();
	char* md5 = string_new();

	//obtener nombre del archivo
	char** dir_aux = string_split(linea,"/");
	int i = 0;
	for(i = 0; dir_aux[i]!= NULL; i++){}
	string_append(&file_aux,dir_aux[i-1]);

	char* arg = string_new();
	string_append_with_format(&arg,"%s%s",dir_actual,linea);
	//crear md5 para obtener_datos
	t_mdj_interface* mdj_interface = malloc(sizeof(t_mdj_interface));
	mdj_interface->path = strdup(arg);
	free(arg);
	mdj_interface->offset = 0;
	mdj_interface->size = 0;

	//validar que el archivo exista
	if(validar_archivo(mdj_interface)){
		lineas_leidas = obtener_datos(mdj_interface);
	}else {
		printf("El archivo no existe\n");
		return;
	}
	//crear archivo auxiliar
	FILE* file = fopen(file_aux,"a");
	fprintf(file,"%s",lineas_leidas);
	fclose(file);

	//md5 por consola
	string_append_with_format(&md5,"md5sum %s",file_aux);
	system(md5);
	free(md5);

	//Borrar archivo
	remove(file_aux);
	free(file_aux);

	free(mdj_interface->path);
	free(mdj_interface);
	free(lineas_leidas);

	int index = 0;
	while(dir_aux[index]!=NULL){
		free(dir_aux[index]);
		index++;
	}
	free(dir_aux);

}

void con_cat(char* linea){
	char* arg = string_new();
	char* lineas_leidas;
	string_append_with_format(&arg,"%s%s",dir_actual,linea);
	t_mdj_interface* mdj_interface = malloc(sizeof(t_mdj_interface));
	mdj_interface->path = strdup(arg);
	free(arg);
	mdj_interface->offset = 0;
	mdj_interface->size = 0;
	if(validar_archivo(mdj_interface)){
		lineas_leidas = obtener_datos(mdj_interface);
		printf("%s",lineas_leidas);
	}else {
		printf("El archivo no existe\n");
	}


	free(mdj_interface->path);
	free(mdj_interface);
	free(lineas_leidas);

}


