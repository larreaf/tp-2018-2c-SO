#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ensalada/com.h>
#include <ensalada/config-types.h>
#include <ensalada/mensaje.h>
#include <ensalada/protocolo.h>
#include <ensalada/servidor.h>
#include <ensalada/validacion.h>
#include <readline/readline.h>
#include <readline/history.h>

#ifndef FM9_FUNCIONES_H
#define FM9_FUNCIONES_H

/*
	Estructuras de datos
*/
	typedef struct {
		int idmensaje;
		int idproceso;
		int direccion_logica;
	}data_mje_buscar;

	typedef struct {
		int idmensaje;
		int idproceso;
		char** contenido;
	}data_mje_escribir;

	typedef struct {
		int idmensaje;
		int idproceso;
		int direccion_logica;
		char** contenido;
	}data_mje_modificar;

	typedef struct {
		int idproceso;
		t_list *direccion_fisica;
	}segmento;

/*
	Variables globales
*/

char *base_storage;
t_dictionary *diccionario_estado_lineas_storage;
t_log *logger;
cfg_fm9 *config_general_fm9;
ConexionesActivas conexiones_activas;
t_list *tabla_de_segmentos;

/*
	Cabeceras funciones
*/

/*
	Creada: 01/11/2018
	Autor: Julieta Ona

	Ult.mod: 01/11/2018
	Usuario mod.: Julieta Ona

	Inicializa la configuración de fm9 asociando arch de config
*/

void inicializar_config_fm9(int argc, char **argv);

/*
	Creada: 01/11/2018
	Autor: Julieta Ona

	Ult.mod: 01/11/2018
	Usuario mod.: Julieta Ona

	Inicializa las conexiones activas con fm9
*/

void inicializar_conexiones_activas_fm9();

/*
	Creada: 01/11/2018
	Autor: Julieta Ona

	Ult.mod: 01/11/2018
	Usuario mod.: Julieta Ona

	Inicializa el storage (memoria real) según el arch de config
*/

void inicializar_storage();

/*
	Creada: 01/11/2018
	Autor: Julieta Ona

	Ult.mod: 01/11/2018
	Usuario mod.: Julieta Ona

	Realiza reintentos de inicialización de storage según
	la variable "cantidad_de_reintentos_ini_storage"
	Dice si hay error de asignación de memoria para el storage
*/

void reintentar_inicializar_storage();

/*
	Creada: 02/11/2018
	Autor: Julieta Ona

	Ult.mod: 02/11/2018
	Usuario mod.: Julieta Ona

	Crea un diccionario del estado de las lineas del storage con el par (nro_linea -> estado)
	El estado puede ser 1 para ocupado y 0 para libre
*/

void crear_diccionario_estado_lineas_storage();

/*
	Creada: 02/11/2018
	Autor: Julieta Ona

	Ult.mod: 02/11/2018
	Usuario mod.: Julieta Ona

	Crea un diccionario del estado de las lineas del storage con 0
	Toma el tamaño total de la memoria y la divide en el valor de max de linea
	que esta en el archivo config de fm9
*/

void incializar_diccionario_estado_lineas_storage();

/*
	Creada: 01/11/2018
	Autor: Julieta Ona

	Ult.mod: 01/11/2018
	Usuario mod.: Julieta Ona

	Ejecuta todas las inicializaciones de FM9, crea el log e informa como se
	desarrollo la inicialización
*/

void inicializar_fm9(int argc, char **argv);

/*
	Creada: 01/11/2018
	Autor: Julieta Ona

	Ult.mod: 01/11/2018
	Usuario mod.: Julieta Ona

	Desempaqueta y retorna mensaje del tipo buscar
*/

data_mje_buscar* desempaquetar_mensaje_buscar(MensajeDinamico* mensaje);

/*
	Creada: 01/11/2018
	Autor: Julieta Ona

	Ult.mod: 01/11/2018
	Usuario mod.: Julieta Ona

	Desempaqueta y retorna mensaje del tipo escribir
*/

data_mje_buscar* desempaquetar_mensaje_escribir(MensajeDinamico* mensaje);

/*
	Creada: 01/11/2018
	Autor: Julieta Ona

	Ult.mod: 01/11/2018
	Usuario mod.: Julieta Ona

	Desempaqueta y retorna mensaje del tipo modificar
*/

data_mje_buscar* desempaquetar_mensaje_modificar(MensajeDinamico* mensaje);

char** buscar_en_memoria(data_mje_buscar* data);

char** escribir_en_memoria(data_mje_buscar* data);

char** modificar_en_memoria(data_mje_buscar* data);

char** buscar_en_memoria_seg(data_mje_buscar* data){
	return "hola";
}
char** buscar_en_memoria_tpi(data_mje_buscar* data){
	return "hola";
}
char** buscar_en_memoria_spa(data_mje_buscar* data){
	return "hola";
}

char** escribir_en_memoria_seg(data_mje_buscar* data);
char** escribir_en_memoria_tpi(data_mje_buscar* data);
char** escribir_en_memoria_spa(data_mje_buscar* data);
char** modificar_en_memoria_seg(data_mje_buscar* data);
char** modificar_en_memoria_tpi(data_mje_buscar* data);
char** modificar_en_memoria_spa(data_mje_buscar* data);

/*
	Creada: 01/11/2018
	Autor: Julieta Ona

	Ult.mod: 01/11/2018
	Usuario mod.: Julieta Ona

	Destruye variables del storage
*/

void destruir_storage();

/*
	Creada: 01/11/2018
	Autor: Julieta Ona

	Ult.mod: 01/11/2018
	Usuario mod.: Julieta Ona

	Se encarga de cerrar las conexiones y de destruir todas las variables
	en memoria de fm9
*/

void cerrar_fm9();

#endif //FM9_FUNCIONES_H
