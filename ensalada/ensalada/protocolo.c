#include "protocolo.h"


void comprobar_error(int variable, const char* mensajeError){
    if(variable < 0){
        perror(mensajeError);
        exit(1);
    }
}