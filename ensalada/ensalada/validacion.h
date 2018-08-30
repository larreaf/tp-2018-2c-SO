#ifndef REDISTINTO_VALIDACION_H_
#define REDISTINTO_VALIDACION_H_

	#include <stdio.h>
	#include <stdlib.h>
	#include <commons/config.h>
	#include <commons/string.h>
	#include "config-types.h"
	#include <string.h>
	#include <commons/collections/list.h>

	/*
	 * @NAME: validar_parametros
	 * @DESC: valida que la cantidad de parametros del proceso sea la correcta
	 */
	void validar_parametros(int argc);

	/*
	 * @NAME: validar_config
	 * @DESC: valida que se halla pasado el archivo de configuracion correctamente. Deberia llamarse
	 * antes que asignar_config.
	 * @RET: devuelve el archivo configuracion
	 */
	t_config* validar_config(char* path, t_process process);

	/*
	 * @NAME:asignar_config
	 * @DESC: retorna un tipo void del archivo configuracion
	 * Queda a criterio del programador usar el tipo correspondiente para recibir
	 * a la funcion [esi_config; planificador_config; coordinador_config; instancia_config]
	 */
	void* asignar_config(t_config* archivo_config, t_process process);

	/*
	 * @NAME: destroy_cfg
	 * @DESC: libera la memoria de los tipos (struct cfg_*)
	 */

	void destroy_cfg (void* cfg, t_process tipo);
	/*
	 * @NAME: atrapar_error
	 * @DESC: evalua el valor bool, en caso de ser igual a 0 imprime el mensaje_error y cierra el proceso
	 */
	void atrapar_error(bool valor, char* mensaje_error);

	/*
	 * @NAME: setClavesBloqueadas
	 * @DESC: obtiene del archivo de configuracion las claves que estarán bloqueadas y las retorna como una lista.
	 */
	t_list * setClavesBloqueadas(char * clavesBloqueadas);
#endif /* REDISTINTO_VALIDACION_H_ */
