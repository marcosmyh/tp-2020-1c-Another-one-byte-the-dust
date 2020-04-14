#include "Team.h"

int main(){
	//LEVANTO EL SERVER
	crearLogger();
	crearLoggerTeams();
	leerArchivoDeConfiguracion();
	socket_servidor = iniciar_servidor(ip_team, puerto_team, logger);
	//ME CONECTO AL BROKER, HAY QUE AGREGARLE LA FUNCIONALIDAD PARA QUE RECONECTE SOLO
	conectarse_a_un_servidor(ip_broker, puerto_broker, logger);
	//ESPERO CLIENTES
	while(1){
		log_info(logger,"Esperando por clientess");
		socket_cliente = esperar_cliente(socket_servidor,logger);
		pthread_t hiloRecibirPaquetes;
		//SI SE CONECTA LO ATIENDO
		if(pthread_create(&hiloRecibirPaquetes, NULL, (void*)atenderCliente, &socket_cliente) == 0){
			pthread_detach(hiloRecibirPaquetes);
			log_info(logger, "Se creo el hilo recibirPaquetes correctamente");
		}
		else{
			log_error(logger, "No se ha podido crear el hilo recibirPaquetes");
		}
	};
	return EXIT_SUCCESS;
}

void crearLogger(){
	char* logPath = "/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/Team/Team.log";
	char* nombreArch = "Team";
	logger = log_create(logPath, nombreArch, 1, LOG_LEVEL_INFO);
	log_info(logger, "El logger general se creo con exito!");
}

void crearLoggerTeams(){
	char* nombreArch = "Log_teams";
	loggerTeams = log_create(team_log_file, nombreArch, 1, LOG_LEVEL_INFO);
	log_info(logger, "El logger de los teams se creo con exito");
}

void leerArchivoDeConfiguracion(){
	char* configPath = "/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/Team/Team.config";
	archivoConfig = config_create(configPath);
	if (archivoConfig == NULL){
		log_error(logger,"Archivo de configuracion no encontrado");
	}
	setearValores(archivoConfig);
	log_info(logger,"La configuracion fue cargada exitosamente");
}

void setearValores(t_config* archConfiguracion){
	posiciones_entrenadores = config_get_array_value(archivoConfig,"POSICIONES_ENTRENADORES");
	pokemon_entrenadores = config_get_array_value(archivoConfig,"POKEMON_ENTRENADORES");
	objetivos_entrenadores = config_get_array_value(archivoConfig,"OBJETIVOS_ENTRENADORES");
	tiempo_reconexion = config_get_int_value(archivoConfig,"TIEMPO_RECONEXION");
	retardo_ciclo_cpu = config_get_int_value(archivoConfig, "RETARDO_CICLO_CPU");
	algoritmo_planificacion = config_get_string_value(archivoConfig, "ALGORITMO_PLANIFICACION");
	quantum = config_get_int_value(archivoConfig, "QUANTUM");
	estimacion_inicial = config_get_int_value(archivoConfig, "ESTIMACION_INICIAL");
	ip_broker = config_get_string_value(archivoConfig, "IP_BROKER");
	puerto_broker = config_get_string_value(archivoConfig, "PUERTO_BROKER");
	ip_team = config_get_string_value(archivoConfig, "IP_TEAM");
	puerto_team = config_get_string_value(archivoConfig, "PUERTO_TEAM");
	team_log_file = config_get_string_value(archivoConfig, "LOG_FILE");
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

int esperar_cliente(int socket_servidor, t_log* logger){
	struct sockaddr_in dir_cliente;

	int tam_direccion = sizeof(struct sockaddr_in);
	log_info(logger, "Esperando...");

	int socket_cliente = accept(socket_servidor, &dir_cliente, &tam_direccion);

	log_info(logger, "Se conecto un cliente!");
	return socket_cliente;
}

int conectarse_a_un_servidor(char* ip, char* puerto, t_log* log){

	struct addrinfo hints;
	struct addrinfo* server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1){
		log_error(logger, "Error de conexion");
		//ACA FALTARIA AGREGAR EL TEMA DE LA RECONEXION CADA 30
		freeaddrinfo(server_info);
		return -1;
	}
	else{
		log_info(logger, "Conexion correcta");
	}
	freeaddrinfo(server_info);

	return socket_cliente;
}

void atenderCliente(int socket_cliente){
	/*
	log_info(logger, "Atendiendo a cliente, socket:%d", socket_cliente);
	while(1){
		Header headerRecibido;
		headerRecibido = recieveHeader(socket_cliente);
		log_info(logger, "Codigo de operacion:%i", headerRecibido.operaciones);
		log_info(logger, "Tamanio:%i", headerRecibido.tamanioMensaje);
		if(headerRecibido.operaciones == -1){
			log_error(logger, "Se desconecto el cliente");
			break;
		}
		uint32_t tamanio = headerRecibido.tamanioMensaje;
		switch(headerRecibido.operaciones){

		case t_NEW:;
			break;

		case t_LOCALIZED:;
			break;

		case t_GET:;
			break;

		case t_APPEARED:;
			break;

		case t_CATCH:;
			break;

		case t_CAUGHT:;
			break;

		default:
			log_error(logger, "No es un codigo de operacion conocido: %i", headerRecibido.operaciones);
			break;
		}
	}
	*/
}
