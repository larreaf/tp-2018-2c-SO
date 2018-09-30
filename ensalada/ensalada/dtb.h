#ifndef DTB_H
#define DTB_H

#include <commons/collections/list.h>

typedef struct{
    int id;
    char* path_script;
    int program_counter;
    int inicializado;
    t_dictionary* archivos_abiertos;
}DTB;

#endif //DTB_H
