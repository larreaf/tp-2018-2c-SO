/*
 * types.h
 *
 *  Created on: 31 ago. 2018
 *      Author: utnso
 */

#ifndef TYPES_H_
#define TYPES_H_
	typedef enum{
		EJECUTAR,
		STATUS,
		FINALIZAR,
		METRICAS
	} tipo_accion_consola_safa;

	typedef struct{
		tipo_accion_consola_safa accion;
		char* argumento;
	}operacionConsolaSafa;


	/*
	 * @NAME:string_to_accion
	 * @DESC: convierte el string en un entero que representa el tipo de operacion
	 */
	tipo_accion_consola_safa string_to_accion(char* string);
	/*
	 * @NAME: parsear_linea
	 * @ARG: linea que se quiere parsear
	 * @RET: estructura con la operacion parseada
	 */
	operacionConsolaSafa* parsear_linea(char* linea);

	/*
	 * @NAME: destroy_operacion
	 * @DESC: libera la memoria de la operacion
	 */
	void destroy_operacion(operacionConsolaSafa* op_safa);
	/*
	 * @NAME: ejecutar_linea
	 * @ARG: linea que se quiere ejecutar
	 * @DESC: llama a la funcion correspondiente a la operacion que se quiere realizar
	 * @RET: void
	 */
	void ejecutar_linea(char* linea);

	/*
	 * @NAME: consola_safa
	 * @DESC: recibir las operaciones por teclado y mostrar resultado
	 */
	void consola_safa();


	/*********************************
	 **** Funciones de la consola ****
	 *********************************/

	void con_ejecutar();

	void con_status();

	void con_finalizar();

	void con_metricas();



#endif /* TYPES_H_ */
