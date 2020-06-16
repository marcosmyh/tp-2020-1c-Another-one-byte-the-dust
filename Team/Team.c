#include "Team.h"

int main(){
	//LEVANTO EL SERVER
	crearLogger();
	crearLoggerObligatorio();
	leerArchivoDeConfiguracion();
	socket_servidor = iniciar_servidor(ip_team, puerto_team, logger);
	inicializarColas();
	inicializarEntrenadores();
	inicializarSemaforosEntrenadores();

	sem_init(&semaforoReconexion,0,1);
	sem_init(&semaforoRespuestaCatch,0,0);
	sem_init(&conexionRecuperadaDeAppeared,0,0);
	sem_init(&conexionRecuperadaDeCaught,0,0);
	sem_init(&conexionRecuperadaDeLocalized,0,0);

    pthread_t hiloMensajes;

    pthread_create(&hiloMensajes,NULL,(void *)iniciarGestionMensajes,NULL);

    pthread_join(hiloMensajes,NULL);

	/*
	//CREO COLA DE PLANIFICACION A READY
	pthread_t hiloPlanificacionReady;
	if(pthread_create(&hiloPlanificacionReady, NULL, (void*)planificarEntradaAReady, NULL) == 0){
		pthread_detach(hiloPlanificacionReady);
		log_info(logger, "Se creo el hilo planificacionReady correctamente");
	}
	else{
		log_error(logger, "No se ha podido crear el hilo planificacionReady");
	}

	//CREO COLA DE PLANIFICACION DE LOS ENTRENADORES EN READY
	pthread_t hiloPlanificacionEntrenadores;
	if(pthread_create(&hiloPlanificacionEntrenadores, NULL, (void*)planificarEntrenadores, NULL) == 0){
		pthread_detach(hiloPlanificacionEntrenadores);
		log_info(logger, "Se creo el hilo planificacionEntrenadores correctamente");
	}
	else{
		log_error(logger, "No se ha podido crear el hilo planificacionEntrenadores");
	}

	*/

    //TODO: EL ALGORITMO DE DEADLOCKS DEBERIA SER UN HILO???
    //TODO: EL DEADLOCK DEBERIA VERIFICAR SI SE CUMPLIO EL OBJETIVO DEL TEAM CUANDO NO HAYA MAS ENTRENADORES PARA EJECUTAR, SINO QUE CORRA EL ALGORITMO DE DETECCION
    //TODO: NO OLVIDARSE DE INFORMAR T0DO LO QUE DICE EN LA PARTE DE DEADLOCK

	//DESTRUIR TODO AL FINAL
	sem_destroy(&semaforoReconexion);
	sem_destroy(&semaforoRespuestaCatch);
    sem_destroy(&conexionRecuperadaDeAppeared);
    sem_destroy(&conexionRecuperadaDeCaught);
    sem_destroy(&conexionRecuperadaDeLocalized);
	return EXIT_SUCCESS;
}

t_infoPaquete *crearInfoPaquete(int socket,void *paquete){
	t_infoPaquete *mensaje = malloc(sizeof(t_infoPaquete));
	mensaje->socket = socket;
	mensaje->paquete = paquete;
	return mensaje;
}

void destruirInfoPaquete(t_infoPaquete *infoPaquete){
	void *paquete = infoPaquete->paquete;
	free(paquete);
	free(infoPaquete);
}

void iniciarGestionMensajes(){
	pthread_create(&hiloSuscripcionBroker,NULL,(void *)administrarSuscripcionesBroker,NULL);
    pthread_create(&hiloAtencionAppeared,NULL,(void *)gestionMensajesAppeared,NULL);
    pthread_create(&hiloAtencionCaught,NULL,(void *)gestionMensajesCaught,NULL);
    pthread_create(&hiloAtencionLocalized,NULL,(void *)gestionMensajesLocalized,NULL);
	pthread_create(&hiloAtencionGameBoy,NULL,(void*)gestionMensajesGameBoy,&socket_servidor);

	pthread_join(hiloAtencionGameBoy,NULL);
	pthread_join(hiloSuscripcionBroker,NULL);
	pthread_join(hiloAtencionAppeared,NULL);
	pthread_join(hiloAtencionCaught,NULL);
	pthread_join(hiloAtencionLocalized,NULL);
}

void suscripcionColaAppeared(){
	conexionAColaAppeared();
	if(conexionAppeared){
		sem_wait(&semaforoReconexion);
	}
}

void suscripcionColaCaught(){
	while(1){
		if(conexionAppeared && identificadorProceso != NULL){
			conexionAColaCaught();
			break;
		}
	}
}

void suscripcionColaLocalized(){
	while(1){
		if(conexionCaught){
			conexionAColaLocalized();
			break;
		}
	}
}

void gestionMensajesLocalized(){
	pthread_t hiloLocalized;

	while(1){
		if(conexionLocalized){
			Header headerRecibido;
			headerRecibido = receiveHeader(socket_localized);
			if(headerRecibido.operacion != -1 && headerRecibido.tamanioMensaje != 0){
				uint32_t tamanio = headerRecibido.tamanioMensaje;
				switch(headerRecibido.operacion){
				case t_HANDSHAKE:
					log_info(loggerObligatorio,"Llego un HANDSHAKE del BROKER a través del socket localized!");
					recibirHandshake(socket_localized,tamanio);
					break;

				case t_LOCALIZED:
					log_info(loggerObligatorio,"Llego un mensaje LOCALIZED del BROKER");
					void *paqueteLocalized = receiveAndUnpack(socket_localized,tamanio);
					t_infoPaquete *infoLocalized = crearInfoPaquete(socket_localized,paqueteLocalized);
					pthread_create(&hiloLocalized,NULL,(void *)procedimientoMensajeLocalized,infoLocalized);
					pthread_detach(hiloLocalized);
					break;

				default:
					log_error(loggerObligatorio,"No es un codigo de operacion conocido: %i",headerRecibido.operacion);
					break;
				}
			}
		}else {
			sem_wait(&conexionRecuperadaDeLocalized);
			}
	}
}

void procedimientoMensajeID(int socket){
	Header headerRecibido;
	headerRecibido = receiveHeader(socket);
	uint32_t tamanio = headerRecibido.tamanioMensaje;

	void* paqueteID = receiveAndUnpack(socket, tamanio);
	uint32_t id = unpackID(paqueteID);
	log_error(logger,"Llego un ID [%d]",id);
	t_operacion operacionAsociada = unpackOperacionID(paqueteID);
	switch (operacionAsociada) {

			case t_GET:;
				list_add(mensajesGET, &id);
				free(paqueteID);
				break;

			case t_CATCH:;
				list_add(mensajesCATCH, &id);
				free(paqueteID);
				sem_post(&semaforoRespuestaCatch);
				llegoRespuesta = 1;
				break;

			default:
				log_error(loggerObligatorio,"No se reciben IDs del codigo de operacion: %i",headerRecibido.operacion);
				free(paqueteID);
				break;
			}
}

void gestionMensajesCaught(){
	pthread_t hiloCaught;

	while(1){
		if(conexionCaught){
			Header headerRecibido;
			headerRecibido = receiveHeader(socket_caught);
			if(headerRecibido.operacion != -1 && headerRecibido.tamanioMensaje != 0){
				uint32_t tamanio = headerRecibido.tamanioMensaje;
				switch(headerRecibido.operacion){
				case t_HANDSHAKE:
					log_info(loggerObligatorio,"Llego un HANDSHAKE del BROKER a través del socket caught!");
					recibirHandshake(socket_caught,tamanio);
					break;

				case t_CAUGHT:
					log_info(loggerObligatorio,"Llego un mensaje CAUGHT del BROKER");
					void *paqueteCaught = receiveAndUnpack(socket_caught,tamanio);
					t_infoPaquete *infoCaught = crearInfoPaquete(socket_caught,paqueteCaught);
					pthread_create(&hiloCaught,NULL,(void *)procedimientoMensajeCaught,infoCaught);
					pthread_detach(hiloCaught);
					break;

				default:
					log_error(loggerObligatorio,"No es un codigo de operacion conocido: %i",headerRecibido.operacion);
					break;
				}
			}
		}else{
			sem_wait(&conexionRecuperadaDeCaught);
			}
	}
}

void gestionMensajesAppeared(){
	pthread_t hiloAppeared;

	while(1){
		if(conexionAppeared){
			Header headerRecibido;
		    headerRecibido = receiveHeader(socket_appeared);
		    if(headerRecibido.operacion != -1 && headerRecibido.tamanioMensaje != 0){
		    uint32_t tamanio = headerRecibido.tamanioMensaje;
		    switch(headerRecibido.operacion){
		    case t_HANDSHAKE:;
		                log_info(loggerObligatorio, "Llego un HANDSHAKE del BROKER a través del socket appeared!");
		    			recibirHandshake(socket_appeared,tamanio);
		    			break;

		    case t_APPEARED:;
	    	  log_info(loggerObligatorio, "Llego un mensaje APPEARED del BROKER!");
	    	  void* paqueteAppeared = receiveAndUnpack(socket_appeared, tamanio);
	    	  t_infoPaquete *infoAppeared = crearInfoPaquete(socket_appeared,paqueteAppeared);
	    	  pthread_create(&hiloAppeared,NULL,(void *)procedimientoMensajeAppeared,infoAppeared);
	    	  pthread_detach(hiloAppeared);

	    	  break;

		    default:;
		        log_error(loggerObligatorio,"No es un codigo de operacion conocido: %i",headerRecibido.operacion);
		    	break;
		    }
		    }
		    else{
		    	conexionAppeared = 0;
		    	sem_post(&semaforoReconexion);
		    	conexionCaught = 0;
		    	conexionLocalized = 0;
		    }
	}else{
    	sem_wait(&conexionRecuperadaDeAppeared);
		}
	}
}

void recibirHandshake(int socket,uint32_t tamanioPaquete){
	void *paqueteBroker = receiveAndUnpack(socket,tamanioPaquete);
	identificadorProceso = unpackProceso(paqueteBroker);
	log_info(logger,"ID RECIBIDO: %s",identificadorProceso);
}

void administrarSuscripcionesBroker(){

	suscripcionColaAppeared();

	if(conexionAppeared){
	enviarPokemonesAlBroker();
	suscripcionColaCaught();
	suscripcionColaLocalized();
	}

	while(1){
		sem_wait(&semaforoReconexion);
		reconexionColaAppeared();

		if(!seEnvioMensajeGET){
		enviarPokemonesAlBroker();
		}

		suscripcionColaCaught();

		suscripcionColaLocalized();
	}
}

void procedimientoMensajeAppeared(t_infoPaquete *infoAppeared){
	void *paqueteAppeared = infoAppeared->paquete;
	int socket = infoAppeared->socket;
    char* pokemonAppeared = unpackPokemonAppeared(paqueteAppeared);
    uint32_t ID_APPEARED= unpackID(paqueteAppeared);
    uint32_t tamanioPokemon = strlen(pokemonAppeared);
    uint32_t coordenadaX = unpackCoordenadaX_Appeared(paqueteAppeared,tamanioPokemon);
    uint32_t coordenadaY = unpackCoordenadaY_Appeared(paqueteAppeared,tamanioPokemon);
    log_info(logger,"MENSAJE RECIBIDO. POKEMON: %s,X: %d,Y: %d",pokemonAppeared,coordenadaX,coordenadaY);

    if (estaEnElObjetivo(pokemonAppeared) && !yaFueAtrapado(pokemonAppeared)) {
    			t_pokemon* pokemonAAtrapar = malloc(sizeof(t_pokemon));
    			pokemonAAtrapar->nombrePokemon = pokemonAppeared;
    			pokemonAAtrapar->coordenadaX = coordenadaX;
    			pokemonAAtrapar->coordenadaY = coordenadaY;
    			log_info(loggerObligatorio,"Pokemon agregado: %s, ubicado en X:%d  Y:%d",pokemonAppeared, coordenadaX, coordenadaY);
    			list_add(pokemonesEnMapa, pokemonAAtrapar);

    			if(esSocketBroker(socket)){
    		    enviarACK(ID_APPEARED, t_APPEARED);
    			}

    			destruirInfoPaquete(infoAppeared);
    }
    else{
    	        if(esSocketBroker(socket)){
    	        	enviarACK(ID_APPEARED,t_APPEARED);
    	        }
    	        destruirInfoPaquete(infoAppeared);
    			log_info(logger,"El mensaje de appeared del pokemon %s ya fue recibido o no necesita atraparse, queda descartado",pokemonAppeared);
    }

}

void procedimientoMensajeLocalized(t_infoPaquete *infoLocalized){
	void *paqueteLocalized = infoLocalized->paquete;

	char* pokemonLocalized = unpackPokemonLocalized(paqueteLocalized);
	uint32_t IDCorrelativo = unpackIDCorrelativo(paqueteLocalized);
	uint32_t ID = unpackID(paqueteLocalized);
	//SI VERIFICA LAS MISMAS CONDICIONES QUE APPEARED Y ENCIMA ES DE UN ID CREADO POR UN MENSAJE GET ENTRA
	if (!estaEnElMapa(pokemonLocalized)&& estaEnElObjetivo(pokemonLocalized) && !yaFueAtrapado(pokemonLocalized) && correspondeAUnIDDe(mensajesGET, IDCorrelativo)) {
		uint32_t tamanioPokemon = strlen(pokemonLocalized);
		uint32_t cantidadPokemones =unpackCantidadParesCoordenadas_Localized(paqueteLocalized,tamanioPokemon);
		uint32_t desplazamiento = tamanioPokemon + 2*sizeof(uint32_t);
		for (int i = 0; i < cantidadPokemones; i++) {
			t_pokemon* pokemonAAtrapar = malloc(sizeof(t_pokemon));
			pokemonAAtrapar->nombrePokemon = pokemonLocalized;
			uint32_t coordenadaX = unpackCoordenadaX_Localized(paqueteLocalized, desplazamiento);
			pokemonAAtrapar->coordenadaX = coordenadaX;
			desplazamiento += sizeof(uint32_t);
			uint32_t coordenadaY = unpackCoordenadaY_Localized(paqueteLocalized, desplazamiento);
			pokemonAAtrapar->coordenadaY = coordenadaY;
			desplazamiento += sizeof(uint32_t);
			log_info(loggerObligatorio,"Pokemon agregado: %s, ubicado en X:%d  Y:%d",pokemonLocalized, coordenadaX, coordenadaY);
			list_add(pokemonesEnMapa, pokemonAAtrapar);
		}
		enviarACK(ID, t_LOCALIZED);
		destruirInfoPaquete(infoLocalized);
	}
	else{
	enviarACK(ID, t_LOCALIZED);
	log_info(logger,"El mensaje de localized/appeared del pokemon %s ya fue recibido o no necesita atraparse, queda descartado",pokemonLocalized);
	destruirInfoPaquete(infoLocalized);
	}
}

void procedimientoMensajeCaught(t_infoPaquete *infoCaught){
	void *paqueteCaught = infoCaught->paquete;
	uint32_t IDCorrelativo = unpackIDCorrelativo(paqueteCaught);
	uint32_t ID = unpackID(paqueteCaught);
	bool resultadoCaught = unpackResultado_Caught(paqueteCaught);
	if (correspondeAUnIDDe(mensajesCATCH, IDCorrelativo)) {
		 enviarACK(ID,t_CAUGHT); //CONFIRMO LA LLEGADA DEL MENSAJE
		 //SI CORRESPONDE LE ASIGNO EL POKEMON AL ENTRENADOR Y LO DESBLOQUEO (PASA A READY)
		 int index = list_get_index(entrenadores,&IDCorrelativo,(void*)comparadorIdCatch);
		 t_entrenador* entrenadorQueAtrapa = list_get(entrenadores,index);
		 completarCatch(entrenadorQueAtrapa,resultadoCaught);
		 destruirInfoPaquete(infoCaught);
	}
	else{
	enviarACK(ID,t_CAUGHT);
	destruirInfoPaquete(infoCaught);
	log_info(logger,"El mensaje recibido no se corresponde con un mensaje CATCH, queda descartado");
	}
}

void conexionAColaAppeared(){
	socket_appeared = conectarse_a_un_servidor(ip_broker,puerto_broker,logger);
	if(socket_appeared != -1){
	if (identificadorProceso == NULL){
	conectarseAColaMensaje(socket_appeared,"Team",t_APPEARED);
	conexionAppeared = 1;
	}
	else{
		conectarseAColaMensaje(socket_appeared,identificadorProceso,t_APPEARED);
		conexionAppeared = 1;
		log_info(logger,"ME RECONECTE AL BROKER. LE MANDE EL ID: %s",identificadorProceso);
	}

	sem_post(&conexionRecuperadaDeAppeared);
	}
}

void conexionAColaCaught(){
	socket_caught = conectarse_a_un_servidor(ip_broker,puerto_broker,logger);

	if(socket_caught != -1){
		conectarseAColaMensaje(socket_caught,identificadorProceso,t_CAUGHT);
		conexionCaught = 1;
		sem_post(&conexionRecuperadaDeCaught);
	}

}


void conexionAColaLocalized(){
	socket_localized = conectarse_a_un_servidor(ip_broker,puerto_broker,logger);
	if(socket_localized != -1){
		conectarseAColaMensaje(socket_localized,identificadorProceso,t_LOCALIZED);
		conexionLocalized = 1;
		sem_post(&conexionRecuperadaDeLocalized);
	}

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
	alpha = config_get_double_value(archivoConfig, "ALPHA");
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

    log_info(log, "Listo para recibir gente");

    return socket_servidor;
}

void gestionMensajesGameBoy(int* socket_servidor){
	struct sockaddr_in dir_cliente;

	int tam_direccion = sizeof(struct sockaddr_in);
	log_info(logger, "Esperando al GameBoy..");

	while(1){
	pthread_t hiloAtencionCliente;

	int socket = accept(*socket_servidor, (void*) &dir_cliente, &tam_direccion);

	log_info(logger, "Se conectó el GameBoy!");

	Header headerRecibido;
    headerRecibido = receiveHeader(socket);
    uint32_t tamanio = headerRecibido.tamanioMensaje;
    log_info(loggerObligatorio, "Llego un mensaje del GAMEBOY!");
	void* paqueteAppeared = receiveAndUnpack(socket, tamanio);
	t_infoPaquete *infoAppeared = crearInfoPaquete(socket,paqueteAppeared);

	if(socket != -1){
		if(pthread_create(&hiloAtencionCliente,NULL,(void*)procedimientoMensajeAppeared,infoAppeared) == 0){
		   pthread_detach(hiloAtencionCliente);
	    }
	}
	}
}

void reconexionColaAppeared(){
	while (1){
		log_info(loggerObligatorio, "Fallo la reconexion, se volvera a intentar en %d segundos",tiempo_reconexion);
		sleep(tiempo_reconexion);
		conexionAColaAppeared();
		if(conexionAppeared){
			log_info(loggerObligatorio, "La reconexion al Broker se realizo con exito");
			conexionAppeared = 1;
			break;
		}


		}
}

void enviarHandshake (int socket, char* identificador, t_operacion operacion){
	void* paquete = pack_Handshake(identificador,operacion);
	uint32_t tamPaquete = strlen(identificador) + 1 + sizeof(uint32_t) + sizeof(t_operacion);
	log_info(logger,"TAMANIO PAQUETE: %d",tamPaquete);
	int resultadoEnvio = packAndSend(socket,paquete,tamPaquete,t_HANDSHAKE);
	log_info(logger,"RESULTADO ENVIO: %d",resultadoEnvio);
	log_info(logger,"LLEGUE HASTA CON TEAM");
	if (resultadoEnvio != -1){
		log_info(logger, "El handshake para subscribirse a una cola se ha realizado con exito");
	}
	else{
		log_error(logger, "El handshake ha fallado");
	}
}

int conectarseAColaMensaje(int socket,char *identificador,t_operacion operacion){
	void *paquete = pack_Handshake(identificador,operacion);
	uint32_t tamPaquete = strlen(identificador) + 1 + sizeof(uint32_t) + sizeof(t_operacion);
	int resultado = packAndSend(socket,paquete,tamPaquete,t_HANDSHAKE);
	return resultado;

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
		log_error(logger, "Falló la conexión con el Broker");
		//ACA FALTARIA AGREGAR EL TEMA DE LA RECONEXION CADA 30
		freeaddrinfo(server_info);
		return -1;
	}
	else{
		log_info(logger, "Conexion establecida con el Broker");
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

void inicializarColas(){
	colaNew = list_create();
	colaReady = list_create();
	colaExec = list_create();
	colaBlocked = list_create();
	colaExit = list_create();
	pokemonesAtrapados = list_create();
	pokemonesEnMapa = list_create();
	objetivoTeam = list_create();
	mensajesGET = list_create();
	mensajesCATCH = list_create();
	entrenadores = list_create();
	log_info(logger, "Se han inicializado todas las colas para la planificacion");
}

void inicializarEntrenadores(){
	int IDMAX = 0;
	int cantidadEntrenadores = obtenerCantidadEntrenadores();
	for (int i=0; i<cantidadEntrenadores; i++){
		t_entrenador* entrenadorNuevo = malloc(sizeof(t_entrenador));
		entrenadorNuevo->idEntrenador = IDMAX;
		IDMAX++;
		entrenadorNuevo->completoObjetivo = false;
		entrenadorNuevo->ocupado = false;
		entrenadorNuevo->rafagasEstimadas = 0;
		entrenadorNuevo->rafagasEjecutadas = 0;
		entrenadorNuevo->distancia = 0;
		entrenadorNuevo->contadorRR = 0;
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
		list_add(colaNew, entrenadorNuevo);
		list_add(entrenadores, entrenadorNuevo);
		entrenadorNuevo->cantPokemonesPorAtrapar = list_size(entrenadorNuevo->objetivo) - list_size(entrenadorNuevo->pokemones);
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
			enviarGET(pokemonAPedir);
		}
		log_info(logger, "Se han enviado los GETs necesarios al broker");

	seEnvioMensajeGET = true;
}

void enviarGET(char* pokemon){
	int socket = conectarse_a_un_servidor(ip_broker, puerto_broker, logger);
	uint32_t id = -1;
	void* paquete = pack_Get(id, pokemon);
	uint32_t tamPaquete = sizeof(uint32_t) + strlen(pokemon) + sizeof(uint32_t);
	packAndSend (socket, paquete, tamPaquete, t_GET);
	log_info(logger, "El envio del mensaje GET para el pokemon %s se realizo con exito",pokemon);
	procedimientoMensajeID(socket);
	close(socket);
}

void enviarCATCH(char* pokemon, uint32_t coordenadaX, uint32_t coordenadaY){
	int socket = conectarse_a_un_servidor(ip_broker, puerto_broker, logger);
	uint32_t id = -1;
	void* paquete = pack_Catch(id, pokemon, coordenadaX, coordenadaY);
	uint32_t tamPaquete = sizeof(uint32_t)*4 + strlen(pokemon);
	packAndSend(socket, paquete, tamPaquete, t_CATCH);
	log_info(logger, "El envio del CATCH se realizo con exito");
	close(socket);
}

void enviarACK(uint32_t ID, t_operacion operacion){
	int socket = conectarse_a_un_servidor(ip_broker, puerto_broker, logger);
	void* paquete = pack_Ack(ID, operacion, identificadorProceso);
	uint32_t tamPaquete = sizeof(ID) + sizeof(t_operacion) + strlen(identificadorProceso) + 1 + sizeof(uint32_t);
	packAndSend(socket, paquete, tamPaquete, t_ACK);
	log_info(logger, "El envio del ACK del mensaje con ID [%d] se realizo con exito",ID);
	close(socket);
}

void inicializarSemaforosEntrenadores(){
	int cantidadEntrenadores = obtenerCantidadEntrenadores();
	int i;
	for (i=0; i<cantidadEntrenadores; i++){
		t_entrenador* unEntrenador = list_get(entrenadores, i);
		sem_t semaforo = unEntrenador->semaforoEntrenador;
		sem_init(&semaforo,0,0);
	}
}

bool estaEnElObjetivo(char* pokemon){
	int pokemonesObjetivoGlobal = list_size(objetivoTeam);
	for(int i=0; i<pokemonesObjetivoGlobal; i++){
		char* unPokemon = list_get(objetivoTeam,i);
		if(strcmp(unPokemon, pokemon) == 0){
			return true;
		}
	}
	return false;
}
bool yaFueAtrapado(char* pokemon){
	t_list* nombresPokemonesAtrapados = list_map(pokemonesAtrapados,(void*)obtenerPokemon); //esto esta bien
	int cantPokemonesAtrapados = list_size(nombresPokemonesAtrapados);
	for(int j=0; j<cantPokemonesAtrapados; j++){
		char* unPokemon = list_get(nombresPokemonesAtrapados, j);
		if(strcmp(unPokemon, pokemon) == 0){
			return true;
		}
	}
	return false;
}

char* obtenerPokemon(t_pokemon* unPokemon){
	return unPokemon->nombrePokemon;
}

void planificarEntrenadores(){
	log_info(logger, "Se comenzara la planificacion de los entrenadores");

	while(1){
		if(!list_is_empty(colaReady) && list_is_empty(colaExec)){
			if(strcmp(algoritmo_planificacion,"FIFO")==0){
				aplicarFIFO();
				t_entrenador* entrenadorAEjecutar = list_get(colaExec, 0);
				entrenadorAEjecutar->operacionEntrenador = t_atraparPokemon;
				pthread_t hiloEntrenador = entrenadorAEjecutar->hiloEntrenador;
				pthread_create(&hiloEntrenador,NULL,(void*)ejecutarEntrenador,entrenadorAEjecutar);
			}
			else if(strcmp(algoritmo_planificacion,"RR")==0){
				aplicarRR();
				t_entrenador* entrenadorAEjecutar = list_get(colaExec, 0);
				entrenadorAEjecutar->operacionEntrenador = t_atraparPokemon;
				pthread_t hiloEntrenador = entrenadorAEjecutar->hiloEntrenador;
				pthread_create(&hiloEntrenador,NULL,(void*)ejecutarEntrenador,entrenadorAEjecutar);
			}
			else if(strcmp(algoritmo_planificacion,"SJF-CD")==0){
				aplicarSJFConDesalojo();
				t_entrenador* entrenadorAEjecutar = list_get(colaExec, 0);
				entrenadorAEjecutar->operacionEntrenador = t_atraparPokemon;
				pthread_t hiloEntrenador = entrenadorAEjecutar->hiloEntrenador;
				pthread_create(&hiloEntrenador,NULL,(void*)ejecutarEntrenador,entrenadorAEjecutar);
				sem_t semaforo = entrenadorAEjecutar->semaforoEntrenador;
				sem_wait(&semaforo);
			}
			else if(strcmp(algoritmo_planificacion,"SJF-SD")==0){
				aplicarSJF();
				t_entrenador* entrenadorAEjecutar = list_get(colaExec, 0);
				entrenadorAEjecutar->operacionEntrenador = t_atraparPokemon;
				pthread_t hiloEntrenador = entrenadorAEjecutar->hiloEntrenador;
				pthread_create(&hiloEntrenador,NULL,(void*)ejecutarEntrenador,entrenadorAEjecutar);
			}
		}
	}

}

void ejecutarEntrenador(t_entrenador* entrenadorAEjecutar){
	if (entrenadorAEjecutar->operacionEntrenador == 0){
		capturarPokemon(entrenadorAEjecutar);
	}
	else{
		//TODO: OPERACION DE INTERCAMBIO, EL ENTRENADOR ELEGIDO TIENE QUE GENERARSE CON QUIEN VA A INTERCAMBIAR
		//intercambiarPokemon(entrenadorAEjecutar);
	}
}

void capturarPokemon(t_entrenador* entrenadorAEjecutar){
	log_info(logger, "Se procedera a ejecutar al entrenador seleccionado por el algoritmo");

	t_pokemon* pokemonAAtrapar;
	entrenadorAEjecutar->ocupado = true;

	if (entrenadorAEjecutar->pokemonAAtrapar == NULL){
		pokemonAAtrapar = pokemonMasCercanoA(entrenadorAEjecutar); //ESTO ES EN EL CASO DE QUE SEA APROPIATIVO EL ALGORITMO
	}

	int distanciaAPokemon = (abs(entrenadorAEjecutar->coordenadaX - pokemonAAtrapar->coordenadaX) + abs(entrenadorAEjecutar->coordenadaY - pokemonAAtrapar->coordenadaY));

	if(strcmp(algoritmo_planificacion,"RR")==0){
		//EJECUTAR CON QUANTUM
		if (entrenadorAEjecutar->pokemonAAtrapar == NULL){
			//ESTO ME PERMITE RECORDAR EL NOMBRE DEL POKEMON QUE ESTOY ATRAPANDO EN RR
			entrenadorAEjecutar->pokemonAAtrapar = pokemonMasCercanoA(entrenadorAEjecutar);
		}
		entrenadorAEjecutar->contadorRR += quantum;
		sleep(retardo_ciclo_cpu);
		if(entrenadorAEjecutar->contadorRR >= distanciaAPokemon){
			//ES COMO SI ME MOVIERA UNA CANTIDAD DE POSICIONES IGUAL AL QUANTUM, SI NO LLEGO AL POKEMON DURANTE EL QUANTUM NO HAGO NADA
			//CUANDO MI CONTADOR SEA IGUAL O MAYOR A LA DISTANCIA RECIEN AHI PASO A ATRAPAR EL POKEMON
			moverEntrenador(entrenadorAEjecutar, entrenadorAEjecutar->pokemonAAtrapar);
			atraparPokemon(entrenadorAEjecutar, entrenadorAEjecutar->pokemonAAtrapar);
			while(1){
				if(llegoRespuesta){
					//CUANDO LLEGA LA RESPUESTA HABILITO LA EJECUCION
					sem_wait(&semaforoRespuestaCatch);
					llegoRespuesta = 0;
					//ME TRAIGO EL ULTIMO ELEMENTO DE LA LISTA DE MENSAJES CATCH, ADD AGREGA AL FINAL DE LA LISTA
					int sizeMensajesCatch = list_size(mensajesCATCH);
					int index = sizeMensajesCatch-1;
					uint32_t* IDCATCH = list_get(mensajesCATCH,index);
					entrenadorAEjecutar->IdCatch = *IDCATCH;
					list_remove(colaExec,0);
					entrenadorAEjecutar->blockeado = true;
					list_add(colaBlocked,entrenadorAEjecutar);
					log_info(loggerObligatorio, "Se cambió un entrenador de EXEC a BLOCKED, Razon: Esta a la espera de un mensaje CAUGHT");
					break;
				}
			}
		}
		//SI TODAVIA NO COMPLETE ESE DELAY LO MANDO DE NUEVO A READY
		list_remove(colaExec,0);
		list_add(colaReady,entrenadorAEjecutar);
		log_info(loggerObligatorio, "Se cambió un entrenador de EXEC a READY, Razon: Fin de quantum");


	}
	else{
		//EJECUTAR SIN QUANTUM
		moverEntrenador(entrenadorAEjecutar, pokemonAAtrapar);
		sleep(retardo_ciclo_cpu);
		entrenadorAEjecutar->rafagasEjecutadas = distanciaAPokemon + 1; //Una rafaga por cada unidad de distancia que se mueve + una rafaga para mandar un mensaje al broker
		atraparPokemon(entrenadorAEjecutar, pokemonAAtrapar);
		while(1){
			if(llegoRespuesta){
				//CUANDO LLEGA LA RESPUESTA HABILITO LA EJECUCION
				sem_wait(&semaforoRespuestaCatch);
				llegoRespuesta = 0;
				//ME TRAIGO EL ULTIMO ELEMENTO DE LA LISTA DE MENSAJES CATCH, ADD AGREGA AL FINAL DE LA LISTA
				int sizeMensajesCatch = list_size(mensajesCATCH);
				int index = sizeMensajesCatch-1;
				uint32_t* IDCATCH = list_get(mensajesCATCH,index);
				entrenadorAEjecutar->IdCatch = *IDCATCH;
				list_remove(colaExec,0);
				entrenadorAEjecutar->blockeado = true;
				list_add(colaBlocked,entrenadorAEjecutar);
				log_info(loggerObligatorio, "Se cambió un entrenador de EXEC a BLOCKED, Razon: Esta a la espera de un mensaje CAUGHT");
				break;
			}
		}
	}
}

void atraparPokemon(t_entrenador* entrenadorAEjecutar, t_pokemon* pokemonAAtrapar){
	entrenadorAEjecutar->pokemonAAtrapar = pokemonAAtrapar;
	enviarCATCH(pokemonAAtrapar->nombrePokemon, pokemonAAtrapar->coordenadaX, pokemonAAtrapar->coordenadaY);
	log_info(loggerObligatorio, "Se quiere atrapar a un pokemon. POKEMON:%s  COORDENADAS: X:%d Y:%d",pokemonAAtrapar->nombrePokemon, pokemonAAtrapar->coordenadaX, pokemonAAtrapar->coordenadaY);
}

void moverEntrenador(t_entrenador* entrenadorAEjecutar, t_pokemon* pokemonAAtrapar){
	log_info(loggerObligatorio, "Se movera al entrenador ID:%d de la posicion X:%d  Y:%d a la posicion  X:%d  Y:%d", entrenadorAEjecutar->idEntrenador,
			entrenadorAEjecutar->coordenadaX, entrenadorAEjecutar->coordenadaY, pokemonAAtrapar->coordenadaX, pokemonAAtrapar->coordenadaY);
	entrenadorAEjecutar->coordenadaX = pokemonAAtrapar->coordenadaX;
	entrenadorAEjecutar->coordenadaY = pokemonAAtrapar->coordenadaY;
}

t_pokemon* pokemonMasCercanoA(t_entrenador* unEntrenador){
	t_list* aux = list_create();
	int cantPokemonesEnMapa = list_size(pokemonesEnMapa);
	for(int i = 0; i < cantPokemonesEnMapa; i++){
		t_pokemon* unPokemon = list_get(pokemonesEnMapa, i);
		int distanciaAPokemon = (abs(unEntrenador->coordenadaX - unPokemon->coordenadaX) + abs(unEntrenador->coordenadaY - unPokemon->coordenadaY));
		list_add(aux, &distanciaAPokemon);
	}
	int menorDistancia = elMenorNumeroDe(aux);
	int posicionMenorDistancia = list_get_index(aux,&menorDistancia,(void*)comparadorPosiciones);
	t_pokemon* pokemonMasCercano = list_get(pokemonesEnMapa,posicionMenorDistancia);

	return pokemonMasCercano;
}

bool comparadorPosiciones(int unaPosicion, int otraPosicion){
	return unaPosicion == otraPosicion;
}

int elMenorNumeroDe(t_list* aux){
	//TODO: NO TIRA MAS WARNINGS, PERO DE IGUAL FORMA HAY QUE CONTROLAR LA LOGICA
	int tamAux = list_size(aux);
	int* elMenor = list_get(aux,0);
	for(int i=1; i<tamAux; i++){
		int* unNumero = list_get(aux,i);
		if (*unNumero < *elMenor){
			elMenor = list_get(aux,i);
		}
	}
	return *elMenor;
}

void completarCatch(t_entrenador* unEntrenador, bool resultadoCaught){
	if (resultadoCaught){
		t_pokemon* pokemonAtrapado = unEntrenador->pokemonAAtrapar;
		char *nombrePokemon = pokemonAtrapado->nombrePokemon;
		sacarPokemonDelMapa(pokemonAtrapado);
		list_add(pokemonesAtrapados, pokemonAtrapado);
		list_add(unEntrenador->pokemones, pokemonAtrapado);
		unEntrenador->cantPokemonesPorAtrapar = unEntrenador->cantPokemonesPorAtrapar - 1;
		unEntrenador->contadorRR = 0;
		log_info(logger, "El pokemon %s pudo ser atrapado!",nombrePokemon);
		int index = list_get_index(colaBlocked, unEntrenador, (void*)comparadorDeEntrenadores);
		//SI LE QUEDAN POKEMONES POR ATRAPAR VA A READY
		if(unEntrenador->cantPokemonesPorAtrapar > 0){
			t_entrenador* unEntrenador = list_remove(colaBlocked,index);
			unEntrenador->blockeado = false;
			unEntrenador->ocupado = false;
			unEntrenador->pokemonAAtrapar = NULL;
			list_add(colaReady,unEntrenador);
			log_info(loggerObligatorio, "Se cambió un entrenador de BLOCKED a READY, Razon: Termino de atrapar un pokemon");
		}
		else{
			//SI NO LE QUEDAN POR ATRAPAR MAS VERIFICO SI CUMPLIÓ EL OBJETIVO O NO, SI CUMPLE MANDO A EXIT, SINO A BLOCKED
			if (cumplioObjetivo(unEntrenador)){
				t_entrenador* unEntrenador = list_remove(colaBlocked,index);
				unEntrenador->blockeado = false;
				unEntrenador->ocupado = false;
				unEntrenador->pokemonAAtrapar = NULL;
				list_add(colaExit, unEntrenador);
				log_info(loggerObligatorio, "Se cambió un entrenador de BLOCKED a EXIT, Razon: Termino de atrapar un pokemon y cumplio con su objetivo");
			}
			else{
				unEntrenador->pokemonAAtrapar = NULL;
				log_info(logger, "El entrenador atrapo el pokemon, no puede atrapar mas y no cumplio el objetivo, esperara al algoritmo de Deadlock");
				//LOS ENTRENADORES QUE NO COMPLETEN EL OBJETIVO Y PROBABLEMENTE ESTEN EN DEADLOCK QUEDAN BLOCKED Y EN ESTADO OCUPADO
			}
		}
	}
	else{
		//SI FALLA EL CATCH VA A READY
		log_info(logger, "El pokemon no pudo ser atrapado");
		int index = list_get_index(colaBlocked, unEntrenador, (void*)comparadorDeEntrenadores);
		t_entrenador* unEntrenador = list_remove(colaBlocked,index);
		unEntrenador->blockeado = false;
		unEntrenador->ocupado = false;
		unEntrenador->contadorRR = 0;
		unEntrenador->pokemonAAtrapar = NULL;
		list_add(colaReady,unEntrenador);
		log_info(loggerObligatorio, "Se cambió un entrenador de BLOCKED a READY, Razon: Fallo al atrapar un pokemon");
	}
}

bool cumplioObjetivo(t_entrenador* unEntrenador){
	int cantPokemonesAtrapados = list_size(unEntrenador->pokemones);
	int cantCoincidencias = 0;
	int i = 0;
	int j = 0;

	for (i = 0; i<cantPokemonesAtrapados; i++){
		//POR CADA POKEMON ATRAPADO VERIFICO SI ES IGUAL A ALGUNO DE LOS POKEMONES QUE ESTAN EN SU OBJETIVO
		t_list* auxObjetivo = unEntrenador->objetivo;
		t_pokemon* unPokemonAtrapado = list_get(unEntrenador->pokemones,i);
		for(j = 0; j<(cantPokemonesAtrapados - cantCoincidencias); j++){
			t_pokemon* unPokemonObjetivo = list_get(auxObjetivo ,i);
			if(strcmp(unPokemonAtrapado->nombrePokemon, unPokemonObjetivo->nombrePokemon) == 0){
				//SI COINCIDEN, AGREGO UNA COINCIDENCIA Y SACO ESE POKEMON DE LA LISTA AUXILIAR DE OBJETIVOS PARA EVITAR REPETIDOS, TAMBIEN CAMBIO EL RANGO DEL FOR CON ESTO
				cantCoincidencias += 1;
				list_remove(auxObjetivo,j);
			}
		}
		j = 0;
	}
	return cantCoincidencias == cantPokemonesAtrapados;
}

void sacarPokemonDelMapa(t_pokemon* unPokemon){
	int index = list_get_index(pokemonesEnMapa, unPokemon, (void*)comparadorPokemones);
	list_remove(pokemonesEnMapa, index);
}

bool comparadorPokemones(t_pokemon* unPokemon, t_pokemon* otroPokemon){
	if (strcmp(unPokemon->nombrePokemon,otroPokemon->nombrePokemon)== 0){
		if (unPokemon->coordenadaX == otroPokemon->coordenadaX && unPokemon->coordenadaY == otroPokemon->coordenadaY){
			return true;
		}
		else{
			return false;
		}
	}
	else{
		return false;
	}

}

void aplicarFIFO(){
	log_info(logger,"Se aplicara el algoritmo de planificacion FIFO");
	if(list_is_empty(colaExec) && (!list_is_empty(colaReady))){
		t_entrenador* entrenadorAEjecutar = (t_entrenador*) list_remove(colaReady, 0);
		list_add(colaExec, entrenadorAEjecutar);
		log_info(loggerObligatorio, "Se cambió un entrenador de READY a EXEC, Razon: Elegido del algoritmo de planificacion");
	}
}

void aplicarRR(){
	log_info(logger,"Se aplicara el algoritmo de planificacion RR, con quantum de %d", quantum);
	if(list_is_empty(colaExec) && (!list_is_empty(colaReady))){
		t_entrenador* entrenadorAEjecutar = (t_entrenador*) list_remove(colaReady, 0);
		list_add(colaExec, entrenadorAEjecutar);
		log_info(loggerObligatorio, "Se cambió un entrenador de READY a EXEC, Razon: Elegido del algoritmo de planificacion");
	}
}

void aplicarSJFConDesalojo(){
	log_info(logger,"Se aplicara el algoritmo de planificacion SJF con desalojo");
	if(!list_is_empty(colaExec)){
		t_entrenador* entrenadorEnEjecucion = list_get(colaExec,0);
		//ANTES DE SACAR AL ENTRENADOR LE BAJO EL SEMAFORO
		sem_t semaforo = entrenadorEnEjecucion->semaforoEntrenador;
		sem_wait(&semaforo);
		list_remove(colaExec,0);
		list_add(colaReady, entrenadorEnEjecucion);
		log_info(loggerObligatorio,"Se cambio un entrenador de EXEC a READY, Razon: Desalojado por el algoritmo de planificacion");
	}
	aplicarSJF();
}

void aplicarSJF(){
	log_info(logger,"Se aplicara el algoritmo de planificacion SJF sin desalojo");
	if(list_is_empty(colaExec) && (!list_is_empty(colaReady))){
		t_list* aux = list_map(colaReady, (void*)calcularEstimacion);
		list_sort(aux, (void*)comparadorDeRafagas);
		t_entrenador* entrenadorAux = (t_entrenador*) list_remove(aux,0);
		int index = list_get_index(colaReady,entrenadorAux,(void*)comparadorDeEntrenadores);
		t_entrenador* entrenadorAEjecutar = list_remove(colaReady,index);
		sem_t semaforo = entrenadorAEjecutar->semaforoEntrenador;
		//CUANDO LO VOY A EJECUTAR LE LEVANTO EL SEMAFORO
		sem_post(&semaforo);
		list_add(colaExec, entrenadorAEjecutar);
		log_info(loggerObligatorio, "Se cambió un entrenador de READY a EXEC, Razon: Elegido del algoritmo de planificacion");
	}
}

t_entrenador* calcularEstimacion(t_entrenador* unEntrenador){
	unEntrenador->rafagasEstimadas = (alpha * estimacion_inicial)
			+ ((1- alpha) * (unEntrenador->rafagasEjecutadas));
	return unEntrenador;
}


bool comparadorDeEntrenadores(t_entrenador* unEntrenador, t_entrenador* otroEntrenador){
	return unEntrenador->idEntrenador == otroEntrenador->idEntrenador;
}

bool comparadorDeRafagas(t_entrenador* unEntrenador, t_entrenador* otroEntrenador){
	return unEntrenador->rafagasEstimadas <= otroEntrenador->rafagasEstimadas;
}

bool comparadorIdCatch(t_entrenador* unEntrenador, uint32_t unIdCatch){
	return unIdCatch == unEntrenador->IdCatch;
}

int list_get_index(t_list* self, void* elemento, bool (*comparator) (void*,void*)){
	int longitudLista = list_size(self);
	int i;
	int cont = 0;
	for(i=0; i<longitudLista; i++){
		if(!comparator(list_get(self,i),elemento)){
			cont++;
		}
		else{
			break;
		}
	}
	return cont;
}

bool estaEnElMapa(char* unPokemon){
	int cantPokemons = list_size(pokemonesEnMapa);
	int i = 0;
	for(i = 0; i<cantPokemons; i++){
		t_pokemon* pokemon = list_get(pokemonesEnMapa,i);
		char* nombrePokemon = obtenerPokemon(pokemon);
		if (strcmp(unPokemon,nombrePokemon) == 0){
			return true;
		}
	}
	return false;
}

void planificarEntradaAReady(){
	//ESTO PLANIFICA DE NEW A READY Y DE BLOCKED A READY
	while(1){
		t_list* pokemones = pokemonesEnMapa;
		t_list* entrenadoresLibres = list_filter(colaBlocked, (void*)estaOcupado);
		if(!list_is_empty(colaNew) || !list_is_empty(entrenadoresLibres)){
			int entrenadoresNew = list_size(colaNew);
			//JUNTO LAS LISTAS DE BLOCKED(POR INACTIVIDAD) Y DE NEW
			for (int i=0; i<entrenadoresNew; i++){
			t_entrenador* unEntrenador = (t_entrenador*) list_get(colaNew, i);
			list_add(entrenadoresLibres,unEntrenador); //esto esta igual que en SJF, deberia estar bien
			}

		//SACO EL PRIMER POKEMON DE LA LISTA
		t_pokemon* unPokemon = list_remove(pokemones, 0);

		//CALCULO LA DISTANCIA DE TODOS A ESE POKEMON
		calcularDistanciaA(entrenadoresLibres,unPokemon);

		//ORDENO LA LISTA EN BASE A ESA DISTANCIA DE MENOR A MAYOR
		list_sort(entrenadoresLibres, (void*)comparadorDeDistancia);

		//SACO EL PRIMERO DE ESA LISTA ORDENADA
		t_entrenador* entrenadorAux = list_remove(entrenadoresLibres, 0);

		//SI ESTA BLOCKEADO LO VOY A BUSCAR A BLOCKEADO, LO SACO Y LO MANDO A READY
		if(entrenadorAux->blockeado){
			int index = list_get_index(colaBlocked,entrenadorAux,(void*)comparadorDeEntrenadores);
			t_entrenador* entrenadorElegido = (t_entrenador*) list_remove(colaBlocked, index);
			list_add(colaReady,entrenadorElegido); //esto esta igual que en SJF, deberia estar bien
			log_info(loggerObligatorio, "Se pasó un entrenador de BLOCKED a READY, Razon: Elegido por el planificador de entrada a ready");
		}

		//SI NO ESTA BLOCKEADO, ESTA EN NEW Y PROCEDO IGUAL
		int index = list_get_index(colaNew,entrenadorAux,(void*)comparadorDeEntrenadores);
		t_entrenador* entrenadorElegido = (t_entrenador*) list_remove(colaNew, index);
		list_add(colaReady,entrenadorElegido); //esto esta igual que en SJF, deberia estar bien
		log_info(loggerObligatorio, "Se pasó un entrenador de NEW a READY, Razon: Elegido por el planificador de entrada a ready");
		}

	}
}

void calcularDistanciaA(t_list* listaEntrenadores, t_pokemon* unPokemon){
	//CREE UNA VARIABLE DISTANCIA PARA PODER HACER ESTO, ESTA ES LA UNICA FUNCION QUE LA TOCA
	int cantEntrenadores = list_size(listaEntrenadores);
	int i;
	for(i=0; i<cantEntrenadores; i++){
		t_entrenador* unEntrenador = list_get(listaEntrenadores,i);
		uint32_t coordenadaXEntrenador = unEntrenador->coordenadaX;
		uint32_t coordenadaYEntrenador = unEntrenador->coordenadaY;
		uint32_t coordenadaXPokemon = unPokemon->coordenadaX;
		uint32_t coordenadaYPokemon = unPokemon->coordenadaY;
		int distanciaAPokemon = (abs(coordenadaXEntrenador - coordenadaXPokemon) + abs(coordenadaYEntrenador - coordenadaYPokemon));
		unEntrenador->distancia = distanciaAPokemon;
	}
}

bool comparadorDeDistancia(t_entrenador* unEntrenador, t_entrenador* otroEntrenador){
	return unEntrenador->distancia <= otroEntrenador->distancia;
}

bool estaOcupado(t_entrenador* unEntrenador){
	return unEntrenador->ocupado;
}

bool esSocketBroker(int socket){
	return socket == socket_appeared || socket == socket_caught || socket == socket_localized;
}

bool correspondeAUnIDDe(t_list* colaDeIDS, uint32_t IDCorrelativo){
	int cantidadIDs = list_size(colaDeIDS);
	int i = 0;
	for(i = 0; i<cantidadIDs; i++){
		uint32_t* unID = list_get(colaDeIDS, i);
		if (*unID == IDCorrelativo){
			return true;
		}
	}
	return false;
}
