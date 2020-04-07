#include "Team.h"

int main(){
	//LEVANTO EL SERVER
	crearLogger();
	leerArchivoDeConfiguracion();
	socket_servidor = iniciar_servidor();
	//ESPERO CLIENTES
	while(1){
		log_info(logger,"Esperando por clientes");
		socket_cliente = esperar_cliente(socket_servidor);
		pthread_t hiloRecibirPaquetes;
		if(pthread_create(&hiloRecibirPaquetes, NULL, (void*)atenderCliente, &socket_cliente) == 0){
			pthread_detach(&hiloRecibirPaquetes);
			log_info(logger, "Se creo el hilo recibirPaquetes correctamente");
		}
		else{
			log_error(logger, "No se ha podido crear el hilo recibirPaquetes");
		}
	}
	return 0;
}

void crearLogger(){
	char* logPath = "/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/Team/src/Team.log";
	char* nombreArch = "Team";
	logger = log_create(logPath, nombreArch, 1, LOG_LEVEL_INFO);
	log_info(logger, "El logger se creo con exito");
}

void leerArchivoDeConfiguracion(){
	char* configPath = "/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/Team/src/Team.cfg";
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
	team_log_file = config_get_string_value(archivoConfig, "LOG_FILE");
}

int iniciar_servidor(){
	int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip_broker, puerto_broker, &hints, &servinfo);

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

    log_info(logger, "Listo para escuchar a mi cliente");

    return socket_servidor;
}

int esperar_cliente(int socket_servidor){
	struct sockaddr_in dir_cliente;

	int tam_direccion = sizeof(struct sockaddr_in);
	log_info(logger, "Esperando...");

	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

	log_info(logger, "Se conecto un cliente!");
	return socket_cliente;
}

void atenderCliente(int socket_cliente){

}
