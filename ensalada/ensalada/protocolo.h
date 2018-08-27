/*
 * protocolo.h
 *
 *  Created on: 27 ago. 2018
 *      Author: utnso
 */

#ifndef REDISTINTO_PROTOCOLO_H_
#define REDISTINTO_PROTOCOLO_H_
	#include <sys/socket.h>
	typedef enum {
		example1,
		example2
	}protocolo_header;




	int enviar_mensaje();

	int recibir_mensaje();

#endif /* REDISTINTO_PROTOCOLO_H_ */
