//
// Created by utnso on 02/11/18.
//

#include "consola.h"

extern Memoria* memoria;

void ejecutar_consola_fm9(void){
    char* linea_leida;
    int id_dtb;

    while(1){

        linea_leida = readline(">");

        if(strlen(linea_leida) >= 6 && string_contains(linea_leida, "dump")) {
            if (string_contains(linea_leida, "dump")) {
                id_dtb = strtol(string_substring_from(linea_leida, 5), NULL, 10);
                dump(memoria, id_dtb);
            }
        }else{
            printf("Comando no reconocido\n");
        }

        free(linea_leida);
    }
}
