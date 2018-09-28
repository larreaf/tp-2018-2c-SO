#ifndef DTB_H
#define DTB_H

typedef struct{
    int id;
    char* path_script;
    int program_counter;
    int inicializado;
    //TODO agregar tabla de archivos abiertos
}DTB;

#endif //DTB_H
