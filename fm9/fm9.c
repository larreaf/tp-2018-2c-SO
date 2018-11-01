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
#include "funciones.h"

int main(int argc, char **argv) {

	inicializar_fm9(argc, argv);

	while (1){
		MensajeDinamico* mensaje;
		mensaje = esperar_mensajes(conexiones_activas);

		switch (mensaje->header) {
		//todo agregar los nuevos casos de header importante!!
			case STRING_DIEGO_FM9_BUSCAR:

				data_mje_buscar *data_buscar = desempaquetar_mensaje_buscar(mensaje);
				return buscar_en_memoria(data_buscar);

			break;
			case STRING_DIEGO_FM9_ESCRIBIR:

				data_mje_escribir *data_escribir = desempaquetar_mensaje_escribir(mensaje);
				return escribir_en_memoria(data_escribir);
			break;
			case STRING_DIEGO_FM9_MODIFICAR:

				data_mje_modificar *data_modificar = desempaquetar_mensaje_modificar(mensaje);
				return escribir_en_memoria(data_modificar);
			break;
			case STRING_DIEGO_FM9_CERRAR:

				cerrar_fm9();
			break;
			case CONEXION_CERRADA:
				cerrar_fm9();
			break;
			default:
				//log_info(logger, "Se desconoce esta instrucción...", datos_fm9->instruccion);
				/*if(!strcmp(str, "exit"))
					cerrar_fm9();*/
			break;
		}

		destruir_mensaje(mensaje);
	}

	return 1;
}
