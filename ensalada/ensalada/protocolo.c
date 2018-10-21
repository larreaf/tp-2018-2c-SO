#include "protocolo.h"


void comprobar_error(int variable, const char* mensajeError){
    // TODO incluir al logger

    if(variable < 0){
        perror(mensajeError);
        exit(1);
    }
}