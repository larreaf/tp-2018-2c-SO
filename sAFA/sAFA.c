#include "types.h"


cfg_safa* configuracion;
t_config* cfg_file;
t_log* log_general;
pthread_t console_thread;

int main(int argc, char **argv) {
	validar_parametros(argc);
	cfg_file = validar_config(argv[1],safa);
	configuracion = asignar_config(cfg_file,safa);

	log_general = log_create("safa.log","S-AFA",1,LOG_LEVEL_TRACE);
	log_trace(log_general,"Archivo de configuracion correcto");


	pthread_create(&console_thread,NULL,(void* ) consola_safa, NULL);
	//pthread_detach(console_thread);
	pthread_join(console_thread,NULL);
	exit_gracefully();
	/*
	 * no hace nada
	 */
	/*for(;;){
	}
	return EXIT_SUCCESS;*/
}

void sig_handler(int signo){
  if (signo == SIGTERM || signo == SIGKILL || signo == SIGINT) {
	  exit_gracefully();
  }
}

void exit_gracefully(){
	log_trace(log_general,"Liberando Memoria");
	config_destroy(cfg_file);
	destroy_cfg(configuracion,safa);

	log_trace(log_general,"Finalizo correctamente");
	log_destroy(log_general);

	exit(EXIT_SUCCESS);
}
