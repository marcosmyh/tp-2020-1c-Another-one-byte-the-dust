#include "Team.h"

int main(){
	//LEVANTO EL SERVER
	crearLogger();
	crearLoggerObligatorio();
	leerArchivoDeConfiguracion();
	socket_servidor = iniciar_servidor(ip_team, puerto_team, logger);
	//ME CONECTO A LAS COLAS
	int conexionColas = conectarseAColasMensajes(ip_broker, puerto_broker, logger);
	//SI FALLA CREO UN HILO PARA HACER LA RECONEXION CADA X SEGUNDOS
	if (conexionColas == -1){
		log_info(loggerObligatorio, "Se procederá a realizar la reconexion con el broker cada %d segundos", tiempo_reconexion);
		pthread_t hiloReconexion;
		if(pthread_create(&hiloReconexion, NULL, (void*)reconectarseAColasMensajes, NULL) == 0){
			pthread_detach(hiloReconexion);
			log_info(logger, "Se creo el hilo de reconexion correctamente");
		}
		else{
			log_error(logger, "No se ha podido crear el hilo de reconexion");
		}
	}
	//ESPERO CLIENTES
	while(1){
		log_info(logger,"Esperando por clientes");
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
	//INICIALIZO LAS COLAS
	inicializarColas();
	//INICIALIZO LOS ENTRENADORES
	inicializarEntrenadores();
	//MANDO UN GET AL BROKER PARA CADA ESPECIE POKEMON QUE ESTA EN EL OBJETIVO
	enviarPokemonesAlBroker();

	return EXIT_SUCCESS;
}

void crearLogger(){
	char* logPath = "/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/Team/Team.log";
	char* nombreArch = "Team";
	logger = log_create(logPath, nombreArch, 1, LOG_LEVEL_INFO);
	log_info(logger, "El logger general se creo con exito!");
}

void crearLoggerObligatorio(){
	char* nombreArch = "Log_teams";
	loggerObligatorio = log_create(team_log_file, nombreArch, 1, LOG_LEVEL_INFO);
	log_info(logger, "El logger obligatorio se creo con exito");
	/*
	ATENCION, LAS ACCIONES A LOGUEAR EN ESTE ARCHIVO SON:

	1-Cambio de un entrenador de cola de planificación (indicando la razón del porqué).
	2-Movimiento de un entrenador (indicando la ubicación a la que se movió).
	3-Operación de atrapar (indicando la ubicación y el pokemon a atrapar).
	4-Operación de intercambio (indicando entrenadores involucrados).
	5-Inicio de algoritmo de detección de deadlock.
	6-Resultado de algoritmo de detección de deadlock.
	7-Llegada de un mensaje (indicando el tipo del mismo y sus datos).
	8-Resultado del Team (especificado anteriormente).
	9-Errores de comunicación con el Broker (indicando que se realizará la operación por default).
	10-Inicio de proceso de reintento de comunicación con el Broker.
	11-Resultado de proceso de reintento de comunicación con el Broker.
	*/

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

void reconectarseAColasMensajes(){
	while (1){
		sleep(tiempo_reconexion);
		int reconexion = conectarseAColasMensajes(ip_broker,puerto_broker,logger);
		if(reconexion == 0){
			log_info(loggerObligatorio, "La reconexion fue realizada con exito, el proceso team se conecto a todas las colas de mensajes");
			break;
		}
		log_info(loggerObligatorio, "Fallo la reconexion, se volvera a intentar en %d segundos",tiempo_reconexion);
	}
}

int conectarseAColasMensajes(char* ip, char* puerto, t_log* log){
	socket_appeared = conectarse_a_un_servidor(ip,puerto,log);
	socket_caught = conectarse_a_un_servidor(ip,puerto,log);
	socket_localized = conectarse_a_un_servidor(ip,puerto,log);
	if(socket_appeared == -1 || socket_caught == -1 || socket_localized == -1){  //Si me falla alguno reconecto todos
		log_info(loggerObligatorio, "Fallo la conexion al broker, se procederá a realizar la accion por default");
		return -1;
	}
	else {
		//Si todos conectan entonces mando handshakes a cada una de las colas
		packAndSend_Handshake(socket_appeared, "Team", t_APPEARED);
		packAndSend_Handshake(socket_caught, "Team", t_CAUGHT);
		packAndSend_Handshake(socket_localized, "Team", t_LOCALIZED);
		return 0;
	}
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

int obtenerCantidadEntrenadores(){
	//SEGUN LA CANT DE ELEMENTOS DE LA LISTA POKEMON ENTRENADORES SACO LA CANTIDAD DE ENTRENADORES
	int i = 0;
	int cantidadEntrenadores = 0;
	while(pokemon_entrenadores[i] != NULL){
		cantidadEntrenadores++;
		i++;
	}
	return cantidadEntrenadores;
}

void inicializarEntrenadores(){
	int IDMAX = 0;
	int cantidadEntrenadores = obtenerCantidadEntrenadores();
	for (int i=0; i<cantidadEntrenadores; i++){
		t_entrenador* entrenadorNuevo = malloc(sizeof(t_entrenador));
		entrenadorNuevo->idEntrenador = IDMAX;
		IDMAX++;
		entrenadorNuevo->completoObjetivo = false;
		char** coordenadas = string_split(posiciones_entrenadores[i], "|");
		int coordenadaX = atoi(coordenadas[0]);
		int coordenadaY = atoi(coordenadas[1]);
		entrenadorNuevo->coordenadaX = coordenadaX;
		entrenadorNuevo->coordenadaY = coordenadaY;
		entrenadorNuevo->pokemones = list_create();
		entrenadorNuevo->objetivo = list_create();
		int j = 0;
		char** pokemones = string_split(pokemon_entrenadores[i], "|");
		while(pokemones[j]!= NULL){
			list_add(entrenadorNuevo->pokemones, pokemones[j]);
			j++;
		}
		int k = 0;
		char** objetivos = string_split(objetivos_entrenadores[i], "|");
		while(objetivos[k]!=NULL){
			//VOY A ASUMIR QUE SI YO MANDO GETS REPETIDOS EL BROKER LOS VA A IGNORAR
			list_add(objetivoTeam, objetivos[k]);
			list_add(entrenadorNuevo->objetivo, objetivos[k]);
			k++;
		}
		//AGREGO EL HILO DE CADA ENTRENADOR A LA COLA DE NEW
		list_add(colaNew, entrenadorNuevo->hiloEntrenador);
		log_info(loggerObligatorio, "Se ha pasado al entrenador %d a la cola de NEW. RAZON: Creacion de entrenador",entrenadorNuevo->idEntrenador);

	}
	log_info(logger, "Se han cargado todos los entrenadores");
	log_info(logger, "Se ha definido el objetivo global del team");
}

void enviarPokemonesAlBroker(){
	//ESTO HACE UN CONNECT AL BROKER Y POR CADA TIPO DE POKEMON QUE ESTE EN EL OBJETIVO GLOBAL SE ENVIA UN GET Y SE CIERRA LA CONEXION
	t_list* pokemonsAPedir = list_create();
	pokemonsAPedir = objetivoTeam;
	int cantPokemones = list_size(pokemonsAPedir);
	for(int i=0; i<cantPokemones; i++){
		char* pokemonAPedir = list_get(pokemonsAPedir, i);
		int socket = conectarse_a_un_servidor(ip_broker, puerto_broker, logger);
		int resultadoGet = packAndSend_Get(socket, pokemonAPedir);
		if (resultadoGet == -1){
			log_info(logger, "El envio del GET ha fallado");
			close(socket);
		}
		log_info(logger, "El envio del GET se realizó con exito");
		close(socket);
	}
	log_info(logger, "Se han enviado los GETs necesarios al broker");
}

void planificarEntrenadores(){
	log_info(logger, "Se comenzara la planificacion de los entrenadores");

}

void inicializarColas(){
	colaNew = list_create();
	colaReady = list_create();
	colaExec = list_create();
	colaBlocked = list_create();
	colaExit = list_create();
	pokemonesAtrapados = list_create();
	pokemonesEnMapa = list_create();
	objetivoTeam = list_create();
	log_info(logger, "Se han inicializado todas las colas para la planificacion");
}

bool necesitaAtraparse(char* pokemon){
	//Controlar globales y ya atrapados
	int pokemonesObjetivoGlobal = list_size(objetivoTeam);
	for(int i=0; i<pokemonesObjetivoGlobal; i++ ){
		char* unPokemon = list_get(objetivoTeam,i);
		if(strcmp(unPokemon, pokemon) == 0){
			return true;
		}
	}
	t_list* pokemonesAtrapados = list_map(pokemonesAtrapados,(void*)obtenerPokemon); //esto esta bien
	int cantPokemonesAtrapados = list_size(pokemonesAtrapados);
	for(int j=0; j<cantPokemonesAtrapados; j++){
		char* unPokemon = list_get(pokemonesAtrapados, j);
		if(strcmp(unPokemon, pokemon) == 0){
			return true;
		}
	}
	return false;
}

char* obtenerPokemon(t_pokemon* unPokemon){
	return unPokemon->nombrePokemon;
}

void atenderCliente(int socket_cliente){
	log_info(logger, "Atendiendo a cliente, socket:%d", socket_cliente);
	while(1){
		Header headerRecibido;
		headerRecibido = receiveHeader(socket_cliente);
		log_info(logger, "Codigo de operacion:%i", headerRecibido.operacion);
		log_info(logger, "Tamanio:%i", headerRecibido.tamanioMensaje);
		if(headerRecibido.operacion == -1){
			log_error(logger, "Se desconecto el cliente");
			break;
		}
		uint32_t tamanio = headerRecibido.tamanioMensaje;
		switch(headerRecibido.operacion){

		case t_NEW:;
			//ESTE SE USA
			break;

		case t_LOCALIZED:;
			//ESTE SE USA
			break;

		case t_GET:;
			//ESTE NO SE USA
			log_error(logger, "No es un codigo de operacion conocido: %i", headerRecibido.operacion);
			break;

		case t_APPEARED:;
			//ESTE SE USA
			void* paqueteAppeared = receiveAndUnpack(socket_cliente, tamanio);
			char* pokemonAppeared = unpackPokemon(paqueteAppeared);
			uint32_t tamanioPokemon = sizeof(pokemonAppeared);
			uint32_t coordenadaX = unpackCoordenadaX_Appeared(paqueteAppeared, tamanioPokemon);
			uint32_t coordenadaY = unpackCoordenadaY_Appeared(paqueteAppeared, tamanioPokemon);
			log_info(loggerObligatorio, "Llego un mensaje de APPEARED");
			log_info(loggerObligatorio, "Pokemon: %s, ubicado en X:%d  Y:%d", pokemonAppeared, coordenadaX, coordenadaY);
			if(necesitaAtraparse(pokemonAppeared)){
				t_pokemon* pokemonAAtrapar = malloc(sizeof(t_pokemon));
				pokemonAAtrapar->nombrePokemon = pokemonAppeared;
				pokemonAAtrapar->coordenadaX = coordenadaX;
				pokemonAAtrapar->coordenadaY = coordenadaY;
				list_add(pokemonesEnMapa,pokemonAAtrapar);

				//ACA TALVES TENDRIA QUE ACTIVAR LA PLANIFICACION DE NEW A READY

			}
			log_info(logger, "El mensaje de appeared recibido refiere a un pokemon que no necesita atraparse, queda descartado");
			break;

		case t_CATCH:;
			//ESTE NO SE USA
			log_error(logger, "No es un codigo de operacion conocido: %i", headerRecibido.operacion);
			break;

		case t_CAUGHT:;
			//ESTE SE USA
			break;

		default:
			log_error(logger, "No es un codigo de operacion conocido: %i", headerRecibido.operacion);
			break;
		}
	}
}
