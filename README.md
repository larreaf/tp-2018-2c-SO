COMPILAR LA BIBLIOTECA "ensalada"

Agregar la siguiente sentencia al final del archivo '.bashrc' para correr procesos desde la consola.
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/workspace/tp-2018-2c-Ensalada-C-sar/ensalada/Debug

Para correr desde eclipse ir a run -> run configurations -> C/C++ Application -> '$proceso que se quiere correr' -> pestaña Enviroment -> New...
            Name: LD_LIBRARY_PATH
            Value: ${workspace_loc}/tp-2018-2c-Ensalada-C-sar/ensalada/Debug
