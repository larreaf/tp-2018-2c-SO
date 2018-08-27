# ensalada shared library
1. Clonar e instalar las commons https://github.com/sisoputnfrba/so-commons-library
2. Clonar este repositorio e importarlo en eclipse

[![image](https://img.youtube.com/vi/v1-POeJtW3E/0.jpg)](https://www.youtube.com/watch?v=v1-POeJtW3E)

3. Compilar la biblioteca.

Para probarla pueden crear proyectos en el mismo directorio y linkear la biblioteca yendo a: 

propiedades del proyecto-> C/C++ General -> Paths and Symbols -> pestaña References -> marcar ensalada

![imagen](https://cdn.discordapp.com/attachments/422066205550182402/483738504682012683/unknown.png)

Luego:

run -> run configurations 

![imagen](https://cdn.discordapp.com/attachments/422066205550182402/483739590134464523/unknown.png)

-> C/C++ Application -> '$proceso que se quiere correr' -> pestaña Enviroment -> New...
![imagen](https://cdn.discordapp.com/attachments/422066205550182402/483740286510563329/unknown.png)
+ *Name: LD_LIBRARY_PATH*
+ *Value: ${workspace_loc}/tp-2018-2c-Ensalada-C-sar/ensalada/Debug*


Agregar la siguiente sentencia al final del archivo '.bashrc' para correr procesos desde la consola:
+ *export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/workspace/tp-2018-2c-Ensalada-C-sar/ensalada/Debug*

Una manera de  hacerlo es abrir una terminal y escribir:
1. *nano ~/.bashrc*
![imagen](https://cdn.discordapp.com/attachments/422066205550182402/483741049546604545/unknown.png)
2. Copiar la sentencia **_export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/workspace/tp-2018-2c-Ensalada-C-sar/ensalada/Debug_**

3. Ir al final del archivo y pegar la sentencia con ctrl+shift+c
4. Guardar con ctrl+o y luego enter
5. Salir con ctrl+x
6. Reiniciar la terminal para que los cambios surtan efecto
