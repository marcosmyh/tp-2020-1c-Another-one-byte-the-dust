#include "GameCard.h"

int main(void){
	crearLogger();
	leerArchivoDeConfiguracion();
	//doy inicio a mi servidor y obtengo el socket
	socket_servidor = iniciar_servidor(ip_gc,puerto_gc,logger);
	//Servidor iniciado
	printf("Server ready for action! \n");

	while(1){
	int cliente = esperar_cliente(socket_servidor,logger);

	if(pthread_create(&thread,NULL,(void*)atender_cliente,&cliente) == 0){
		pthread_detach(thread);
		}

	}
	config_destroy(archivoConfig);
	log_destroy(logger);
	return 0;
}




void crearLogger(){
	char *path = "/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/GameCard/GameCardServer.log";
	char *nombreArchi = "GameCardServer";
	logger = log_create(path,nombreArchi,true, LOG_LEVEL_INFO);

}

int iniciar_servidor(char* ip, char* puerto, t_log* log){
	int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip, puerto, &hints, &servinfo);

    for (p=servinfo; p != NULL; p = p->ai_next)
    {
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            continue;
        }
        break;
    }

	listen(socket_servidor, SOMAXCONN);

    freeaddrinfo(servinfo);

    log_info(log, "Listo para escuchar a mi cliente");

    return socket_servidor;
}

int esperar_cliente(int socket_servidor, t_log* log){
	struct sockaddr_in dir_cliente;

	int tam_direccion = sizeof(struct sockaddr_in);
	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);
	log_info(log,"Se conecto un tipazo.");
	return socket_cliente;
}

void atender_cliente(int* socket){
	int cod_op;

	if(recv(*socket, &cod_op, sizeof(int), MSG_WAITALL) == -1)
		cod_op = -1;
	procesar_solicitud(cod_op, *socket);
}

void procesar_solicitud(int cod_op, int cliente_fd) {
	//int size;
	//void* msg;

		switch (cod_op) {
		case GET:
			log_info(logger,"Me llegaron mensajes de Suscriber get");
			//Hacer algo con get
			//msg = recibir_mensaje(cliente_fd, &size);
			//free(msg);
			break;

		case CATCH:
			log_info(logger,"Me llegaron mensajes de Suscriber Catch");
			//Hacer lo que debia hacerse cuando se recibiera un catch
			//msg = recibir_mensaje(cliente_fd, &size);
			//free(msg);
			break;
		case NEW:
			log_info(logger,"Me llegaron mensajes de Suscriber New");
			//Hacer lo que debiera hacerse con new
			//msg = recibir_mensaje(cliente_fd, &size);
			//free(msg);
			break;
		case 0:
			printf("Se desconecto.");
			pthread_exit(NULL);

		case -1:
			pthread_exit(NULL);
		}
}

void leerArchivoDeConfiguracion(void){
	char* path = "/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/GameCard/GameCard.config";
	archivoConfig = config_create(path);
	if (archivoConfig == NULL){
		log_error(logger,"Archivo de configuracion no encontrado");
	}
	setearValoresDeGame(archivoConfig);
	log_info(logger,"La configuracion fue cargada exitosamente");
}

void setearValoresDeGame(t_config *config){
	ip_gc = config_get_string_value(config,"IP_GAMECARD");
	puerto_gc = config_get_string_value(config,"PUERTO_GAMECARD");
	ip_broker = config_get_string_value(config,"IP_BROKER");
	puerto_broker = config_get_string_value(config,"PUERTO_BROKER");
	path_de_tallgrass = config_get_string_value(config,"PUNTO_MONTAJE_TALLGRASS");
	tiempo_de_reconexion = config_get_int_value(config,"TIEMPO_DE_REINTENTO_CONEXION");
	tiempo_de_reintento_operacion = config_get_int_value(config,"TIEMPO_DE_REINTENTO_OPERACION");
}
