#include "consola.h"
#include "mdj_functions.h"
#include <unistd.h>

char* dir_mdj;
char* dir_fifa_abs;
char* dir_fifa_rel;
char* dir_actual;
extern cfg_mdj* configuracion;
extern char* punto_de_montaje_absoluto;

void consola_mdj(){

    dir_fifa_rel = string_new();
    string_append_with_format(&dir_fifa_rel,"%s/Archivos" ,configuracion->punto_montaje);
    dir_mdj = get_current_dir_name();
    //printf("%s\n", dir_mdj);
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
	t_mdj_interface* mdj_interface = malloc(sizeof(t_mdj_interface));
	char* bitmap_string;
	switch (op_consola->accion){
		case LS:
			if(verificar_path(op_consola->argumento, true)){
				con_ls(op_consola->argumento);
			} else {
				printf("Directorio invalido\n");
			}
			destroy_operacion(op_consola);
			break;

		case CD:
			/*if(verificar_path(op_consola->argumento, true)){*/
				con_cd(op_consola->argumento);
			/*} else {
				printf("Directorio invalido\n");
			}*/

			destroy_operacion(op_consola);
			break;

		case MD5_CONSOLA:
			mdj_interface->path = string_new();
			string_append_with_format(&mdj_interface->path,"%s%s",dir_actual,op_consola->argumento);
			if(op_consola->argumento != NULL && !string_is_empty(op_consola->argumento) && verificar_path(op_consola->argumento, false) && validar_archivo(mdj_interface)){
				con_md5(op_consola->argumento);
			} else {
				printf("Archivo invalido\n");
			}
			free(mdj_interface->path);
			destroy_operacion(op_consola);
			break;

		case CAT:
			mdj_interface->path = string_new();
			string_append_with_format(&mdj_interface->path,"%s%s",dir_actual,op_consola->argumento);
			if(op_consola->argumento != NULL && !string_is_empty(op_consola->argumento) && verificar_path(op_consola->argumento, false) && validar_archivo(mdj_interface)){
				con_cat(op_consola->argumento);
			} else {
				printf("Archivo invalido\n");
			}
			free(mdj_interface->path);
			destroy_operacion(op_consola);
			break;

		case BITMAP:
			 /*
			 * Visualizar bitmap
			 */
			if(string_equals_ignore_case(op_consola->argumento,"-c")){
				 bitmap_clean();
			 }
			bitmap_string = get_bitmap_to_string();
			printf("Bitmap: %s\n",bitmap_string);
			free(bitmap_string);
			break;

		case EXIT:
			exit(0);
			break;

		default:

			printf("Operacion incorrecta\n");
			break;
	}
	free(mdj_interface);
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
    if(!strcmp(string,"bitmap"))
        retorno = BITMAP;
    if(!strcmp(string,"exit"))
        retorno = EXIT;
    return retorno;
}

bool check_folder(char* folder){
	char* directorio = string_new();
	string_append_with_format(&directorio,"%s/%s%s",dir_fifa_abs,dir_actual,folder);
	char* path_absoluto = realpath(directorio, NULL);
	//   printf("realpath = %s\n punto_montaje = %s",path_absoluto, punto_de_montaje_absoluto);

	if (path_absoluto != NULL && string_contains(path_absoluto,punto_de_montaje_absoluto) && !string_equals_ignore_case(path_absoluto,punto_de_montaje_absoluto)){
		return true;
	}
	if(path_absoluto != NULL){
		free(path_absoluto);
	}
	free(directorio);
	return false;
}

bool verificar_path(char* linea, bool dir){
	char** path_split = string_split(linea,"/");
	char* carpeta = string_new();
	int i = 0;
	int longitud = 0;
	bool ret = true;
	while(path_split[i] != NULL ){
		if(!string_is_empty(path_split[i])){
		longitud++;
		}
		i++;
	}
	if(!dir){
		longitud--;
	}

	for(i = 0; i < longitud && ret;i++){
		string_append_with_format(&carpeta,"%s/",path_split[i]);
		ret = check_folder(carpeta);
	}
	i = 0;
	while(path_split[i] != NULL){
		free(path_split[i]);
		i++;
	}
	free(path_split);
	free(carpeta);
	return ret;

}

void con_ls(char* linea){
    char* arg = string_new();
    char* directorio = string_new();
    string_append_with_format(&directorio,"%s/%s%s",dir_fifa_abs,dir_actual,linea);
    string_append_with_format(&arg,"ls %s",directorio);

    system(arg);

    free(directorio);
    free(arg);
}


void mover_dir(char* dir){

    if(string_equals_ignore_case(dir,"..")){
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
    }else if(string_equals_ignore_case(dir,".")){

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
        free(lineas_leidas);
        //printf("\n");
    }else {
        printf("El archivo no existe\n");
    }

    free(mdj_interface->path);
    free(mdj_interface);


}

