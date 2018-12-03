#include "types.h"
#include "plp.h"

extern bool correr;
extern PLP* plp;
extern PCP* pcp;
sem_t semaforo_resumir;

void* consola_safa(void* arg){
	while(correr){
		//signal(SIGINT, sig_handler);
		//signal(SIGTERM, sig_handler);
		//signal(SIGKILL, sig_handler);

		char *linea_leida = readline(">");

		add_history(linea_leida);

		ejecutar_linea(linea_leida);

		free(linea_leida);

	}

	return NULL;
}

void ejecutar_linea(char* linea){
    int argumento_entero;
	operacionConsolaSafa* op_consola = parsear_linea(linea);
	switch (op_consola->accion){
		case EJECUTAR:
			con_ejecutar(op_consola->argumento);
			destroy_operacion(op_consola);
			break;

		case STATUS:
		    argumento_entero = strtol(op_consola->argumento, NULL, 10);
            if (!argumento_entero) {
                printf("Error al parsear comando\n");
                destroy_operacion(op_consola);
                break;
            }

			con_status(argumento_entero);
			destroy_operacion(op_consola);
			break;

		case FINALIZAR:
            argumento_entero = strtol(op_consola->argumento, NULL, 10);
            if (!argumento_entero) {
                printf("Error al parsear comando\n");
                destroy_operacion(op_consola);
                break;
            }

            con_finalizar(argumento_entero);
			destroy_operacion(op_consola);
			break;

		case METRICAS:

		    if(strcmp(op_consola->argumento, "")) {
                argumento_entero = strtol(op_consola->argumento, NULL, 10);
                if (!argumento_entero) {
                    printf("Error al parsear comando\n");
                    destroy_operacion(op_consola);
                    break;
                }
            }else
                argumento_entero = 0;

            con_metricas(argumento_entero);
			destroy_operacion(op_consola);
			break;

	    case PAUSAR:
            con_pausar();
            destroy_operacion(op_consola);
	        break;

	    case RESUMIR:
	        con_resumir();
	        destroy_operacion(op_consola);
	        break;

		case EXIT:
			destroy_operacion(op_consola);
			correr = 0;
			break;

		default:

			printf("Operacion incorrecta\n");
			break;
	}
}

operacionConsolaSafa* parsear_linea(char* linea){
	operacionConsolaSafa* retorno = malloc(sizeof(operacionConsolaSafa));
	retorno->accion = 9999;
	retorno->argumento = string_new();
	int i = 0;
	char* word;

	while((word = strsep(&linea," ")) != NULL && retorno->accion != -1)
	{
		if (i == 0)
		{
			retorno->accion = string_to_accion(word);
		}
		else {
			string_append(&retorno->argumento, word);
            string_append(&retorno->argumento, " ");
		}

		i++;
	}
	return retorno;
}

void destroy_operacion(operacionConsolaSafa* op_safa){
    free(op_safa->argumento);
	free(op_safa);
	return;
}

tipo_accion_consola_safa string_to_accion(char* string){
	if(!strcmp(string,"ejecutar"))
			return EJECUTAR;
	if(!strcmp(string,"status"))
			return STATUS;
	if(!strcmp(string,"finalizar"))
			return FINALIZAR;
	if(!strcmp(string,"metricas"))
			return METRICAS;
	if(!strcmp(string,"pausar"))
	        return PAUSAR;
	if(!strcmp(string,"resumir"))
	        return RESUMIR;
	if(!strcmp(string,"exit"))
				return EXIT;
	return -1;
}


void con_ejecutar(char* ruta_escriptorio){
    MetricasDTB* metricas_dtb = malloc(sizeof(MetricasDTB));
    metricas_dtb->cantidad_instrucciones_dma = 0;
    metricas_dtb->cantidad_instrucciones_ejecutadas = 0;
    metricas_dtb->cantidad_instrucciones_new = 0;
    metricas_dtb->id_dtb = contador_id_dtb;
    metricas_dtb->en_new = 1;

    DTB* nuevo_dtb = crear_dtb(contador_id_dtb, 1);
	log_info(plp->logger, "Creando DTB %d", nuevo_dtb->id);
	string_append(&nuevo_dtb->path_script, ruta_escriptorio);
	list_add(plp->metricas_dtbs, metricas_dtb);

    agregar_a_new(plp, nuevo_dtb);
    contador_id_dtb++;
    printf("Ejecutando %s\n", ruta_escriptorio);
}

void con_status(int id_DTB){
    DTB* dtb_seleccionado;
    ArchivoAbierto* archivo_abierto;

    if(!id_DTB){
        imprimir_estado_plp(plp);
        imprimir_estado_pcp(pcp);
        return;
    }else{
        dtb_seleccionado = obtener_dtb_de_new(plp, id_DTB, false);

        if(dtb_seleccionado == NULL){
            printf("DTB %d no encontrado en NEW, buscando en BLOCK y EXEC\n", id_DTB);
            dtb_seleccionado = encontrar_dtb_pcp(pcp, id_DTB);
        }

        if(dtb_seleccionado == NULL){
            printf("DTB %d no encontrado\n", id_DTB);
            return;
        }
    }
    printf("---------\n");
    printf("Datos DTB: %d\n", id_DTB);
    printf("Path: %s\n", dtb_seleccionado->path_script);
    printf("Program Counter: %d\n", dtb_seleccionado->program_counter);
    printf("Inicializado: %d\n", dtb_seleccionado->inicializado);
    printf("Quantum: %d\n", dtb_seleccionado->quantum);
    printf("Status: %d\n", dtb_seleccionado->status);

    if(!list_size(dtb_seleccionado->archivos_abiertos))
        printf("Sin archivos abiertos\n");
    else{
        for(int i = 0; i<list_size(dtb_seleccionado->archivos_abiertos); i++){
            archivo_abierto = list_get(dtb_seleccionado->archivos_abiertos, i);
            printf("Archivo abierto %s (direccion %d)\n", archivo_abierto->path, archivo_abierto->direccion_memoria);
        }
    }
    printf("---------\n");
	return;
}

void con_finalizar(int id_DTB){
    DTB* dtb_seleccionado;

    if(!id_DTB){
        printf("No es posible finalizar el DTB DUMMY!\n");
        return;
    }

    pthread_mutex_lock(&pcp->mutex_block);
    dtb_seleccionado = encontrar_dtb_en_lista(pcp->lista_block, id_DTB, true);
    pthread_mutex_unlock(&pcp->mutex_block);

    if(dtb_seleccionado == NULL){
        printf("DTB %d no encontrado en BLOCK\n", id_DTB);

        pthread_mutex_lock(&pcp->mutex_exec);
        dtb_seleccionado = encontrar_dtb_en_lista(pcp->lista_exec, id_DTB, false);
        pthread_mutex_unlock(&pcp->mutex_exec);

        if(dtb_seleccionado == NULL) {
            printf("DTB %d encontrado en EXEC\n", id_DTB);
            pcp->finalizar_dtb = id_DTB;
            return;
        }
    }else{
        printf("DTB %d encontrado en BLOCK, finalizando\n", id_DTB);
        destruir_dtb(dtb_seleccionado);
        sem_post(&plp->semaforo_multiprogramacion);
    }

	return;
}

void con_metricas(int id_DTB){
    int lista_size, acumulador_instr_ejecutadas = 0, acumulador_instr_dma = 0, i = 0,
    acumulador_tiempos_respuesta = 0, j;
    float cantidad_instrucciones_promedio, cantidad_instrucciones_dma_promedio, porcentaje_dma, tiempo_respuesta_prom;
    MetricasDTB* metricas;

    pthread_mutex_lock(&plp->mutex_metricas);
    if(!id_DTB){
        lista_size = list_size(plp->metricas_dtbs);

        for(; i<lista_size; i++) {
            metricas = list_get(plp->metricas_dtbs, i);

            acumulador_instr_ejecutadas += metricas->cantidad_instrucciones_ejecutadas;
            acumulador_instr_dma += metricas->cantidad_instrucciones_dma;
        }

        cantidad_instrucciones_promedio = (float)acumulador_instr_ejecutadas/(float)i;
        cantidad_instrucciones_dma_promedio = (float)acumulador_instr_dma/(float)i;
        porcentaje_dma = ((float)acumulador_instr_dma/(float)acumulador_instr_ejecutadas)*100;

        for(j = 0; j<TIEMPOS_RESPUESTA_SIZE; j++){
            if(pcp->tiempos_respuesta[j] == -1)
                break;

            acumulador_tiempos_respuesta += pcp->tiempos_respuesta[j];
        }

        if(j)
            tiempo_respuesta_prom = (float)acumulador_tiempos_respuesta/(float)j;
        else
            tiempo_respuesta_prom = -1;

        printf("--------------\n");
        printf("Metricas generales:\n");
        printf("Cantidad de instrucciones ejecutadas promedio para que el DTB termine en exit: %.1f\n",
                cantidad_instrucciones_promedio);
        printf("Cantidad de instrucciones promedio que usaron a El Diego: %.1f\n", cantidad_instrucciones_dma_promedio);
        printf("Porcentaje de instrucciones que usaron a El Diego: %1.f%%\n", porcentaje_dma);
        printf("Tiempo de respuesta promedio: %1.fs\n", tiempo_respuesta_prom);
        printf("--------------\n");
    }else{
        metricas = encontrar_metricas_en_lista(plp->metricas_dtbs, id_DTB, false);

        if(metricas != NULL){
            printf("--------------\n");
            printf("Metricas DTB %d:\n", id_DTB);
            printf("Cantidad instrucciones ejecutadas: %d\n", metricas->cantidad_instrucciones_ejecutadas);
            printf("Cantidad instrucciones DMA: %d\n", metricas->cantidad_instrucciones_dma);
            printf("Cantidad instrucciones esperadas en NEW: %d\n", metricas->cantidad_instrucciones_new);
            printf("--------------\n");
        } else {
            log_warning(plp->logger, "DTB %d no encontrado en metricas", id_DTB);
            printf("DTB %d no encontrado en metricas\n", id_DTB);
        }
    }
    pthread_mutex_unlock(&plp->mutex_metricas);

	return;
}

void con_pausar(){
    pthread_mutex_lock(&pcp->mutex_pausa);
    printf("PCP pausado\n");
}

void con_resumir(){
    pthread_mutex_unlock(&pcp->mutex_pausa);
    printf("PCP resumido\n");
}
