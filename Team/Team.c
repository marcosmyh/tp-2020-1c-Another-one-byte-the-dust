#include "Team.h"


int main(){
	leerArchivoDeConfiguracion();
	crearLogger();
	crearLoggerObligatorio();
	planificador = construirPlanificador();
	socket_servidor = iniciar_servidor(ip_team, puerto_team, logger);
	inicializarColas();
	inicializarEntrenadores();
	inicializarSemaforosEntrenadores();
	inicializarSemaforosTeam();
	inicializarHilosTeam();
	destruirSemaforosTeam();
	destruirColasTeam();
	return EXIT_SUCCESS;
}


//TODO revisar.
void destruirColasTeam(){
	list_destroy(colaNew);
	list_destroy(colaReady);
	list_destroy(colaExec);
	list_destroy(colaBlocked);
	list_destroy(colaExit);
	list_destroy(pokemonesAtrapados);
	list_destroy(pokemonesEnMapa);
	list_destroy(mensajesGET);
	list_destroy(mensajesCATCH);
	list_destroy(entrenadores);
	list_destroy(pokemonesLocalized);
	list_destroy(pokemonesAppeared);
}

void destruirSemaforosTeam(){
	sem_destroy(&semaforoReconexion);
	sem_destroy(&semaforoRespuestaCatch);
    sem_destroy(&conexionRecuperadaDeAppeared);
    sem_destroy(&conexionRecuperadaDeCaught);
    sem_destroy(&conexionRecuperadaDeLocalized);
    sem_destroy(&semaforoMetricas);
    sem_destroy(&semaforoAgregadoPokemones);
    sem_destroy(&semaforoPlanificacionExec);
    sem_destroy(&semaforoDeteccionDeadlocks);
    sem_destroy(&semaforoPlanificacionInicialReady);
    pthread_mutex_destroy(&mutexPokemonesEnMapa);
    pthread_mutex_destroy(&mutexEntrenadoresReady);
    pthread_mutex_destroy(&mutexDeadlock);
}

void inicializarHilosTeam(){
    pthread_create(&hiloMensajes,NULL,(void *)iniciarGestionMensajes,NULL);
    pthread_create(&hiloPlanificacionReady, NULL, (void*)planificadorALargoPlazo, NULL);
    pthread_create(&hiloPlanificacionEntrenadores,  NULL, (void*)planificadorACortoPlazo, NULL);
    pthread_create(&hiloRevisionDeadlocks, NULL, (void*)revisionDeadlocks, NULL);
    pthread_create(&impresionDeMetricas, NULL, (void*)imprimirMetricas, NULL);

    pthread_join(hiloMensajes,NULL);
    pthread_join(hiloPlanificacionReady, NULL);
    pthread_join(hiloPlanificacionEntrenadores,NULL);
    pthread_join(hiloRevisionDeadlocks,NULL);
    pthread_join(impresionDeMetricas,NULL);
}

void inicializarSemaforosTeam(){
	sem_init(&semaforoReconexion,0,1);
	sem_init(&semaforoRespuestaCatch,0,0);
	sem_init(&conexionRecuperadaDeAppeared,0,0);
	sem_init(&conexionRecuperadaDeCaught,0,0);
	sem_init(&conexionRecuperadaDeLocalized,0,0);
	sem_init(&semaforoMetricas,0,0);
	sem_init(&semaforoAgregadoPokemones,0,1);
	sem_init(&semaforoPlanificacionExec,0,0);
	sem_init(&semaforoDeteccionDeadlocks,0,0);
	sem_init(&semaforoPlanificacionInicialReady,0,0);
	pthread_mutex_init(&mutexPokemonesEnMapa,NULL);
	pthread_mutex_init(&mutexEntrenadoresReady,NULL);
	pthread_mutex_init(&mutexDeadlock,NULL);
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

int cantOcurrencias(char *nombrePokemon,t_list *lista){
	int cantOcurrencias = 0;

	for(int i = 0; i < list_size(lista);i++){
		char *elem = list_get(lista,i);
		if(stringComparator(elem,nombrePokemon)){
			cantOcurrencias++;
		}
	}

	return cantOcurrencias;
}

bool tieneLosMismosElementos(t_list *pokemonesEntrenador,t_list *objetivos,bool(*comparator)(void *,void *)){
	bool resultado = false;
	for(int i = 0; i < list_size(pokemonesEntrenador);i++){
		char *pokemonAComparar = list_get(pokemonesEntrenador,i);

		int cantOcurrenciasEntrenador = cantOcurrencias(pokemonAComparar,pokemonesEntrenador);
		int cantOcurrenciasObjetivos = cantOcurrencias(pokemonAComparar,objetivos);

		if(cantOcurrenciasEntrenador != cantOcurrenciasObjetivos){
			return resultado;
		}
	}
	return !resultado;
}

bool stringComparator(void *unString, void *otroString){
	return string_equals_ignore_case(unString,otroString);
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
	//uint32_t idAAgregar;

	void* paqueteID = receiveAndUnpack(socket, tamanio);
	uint32_t id = unpackID(paqueteID);
	log_error(logger,"Llego un ID [%d]",id);
	int *idAAgregar = malloc(sizeof(uint32_t));
	*idAAgregar = id;

	t_operacion operacionAsociada = unpackOperacionID(paqueteID);
	switch (operacionAsociada) {

			case t_GET:;
				list_add(mensajesGET,idAAgregar);
				free(paqueteID);
				break;

			case t_CATCH:;
				list_add(mensajesCATCH,idAAgregar);
				free(paqueteID);
				sem_post(&semaforoRespuestaCatch);
				llegoRespuesta = true;
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
		    	destruirSemaforosTeam();
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
	if(identificadorProceso == NULL){
		identificadorProceso = unpackProceso(paqueteBroker);
		config_set_value(archivoConfig,"ID",identificadorProceso);
		config_save(archivoConfig);
		free(paqueteBroker);
	}else{
		free(paqueteBroker);
	}
	//identificadorProceso = unpackProceso(paqueteBroker);
	//config_set_value(archivoConfig,"ID",identificadorProceso);
	//config_save(archivoConfig);
	//log_info(logger,"ID RECIBIDO: %s",identificadorProceso);
	//free(identificadorProceso);
	//identificadorProceso = config_get_string_value(archivoConfig,"ID");
	//free(paqueteBroker);
}

void administrarSuscripcionesBroker(){

	suscripcionColaAppeared();

	if(conexionAppeared){
	suscripcionColaCaught();
	suscripcionColaLocalized();
	enviarPokemonesAlBroker();
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
	pthread_mutex_lock(&mutexPokemonesEnMapa);
	void *paqueteAppeared = infoAppeared->paquete;
	int socket = infoAppeared->socket;
    char* pokemonAppeared = unpackPokemonAppeared(paqueteAppeared);
    uint32_t ID_APPEARED= unpackID(paqueteAppeared);
    uint32_t tamanioPokemon = strlen(pokemonAppeared);
    uint32_t coordenadaX = unpackCoordenadaX_Appeared(paqueteAppeared,tamanioPokemon);
    uint32_t coordenadaY = unpackCoordenadaY_Appeared(paqueteAppeared,tamanioPokemon);
    //log_info(logger,"MENSAJE RECIBIDO. POKEMON: %s,X: %d,Y: %d",pokemonAppeared,coordenadaX,coordenadaY);

    if (estaEnElObjetivo(pokemonAppeared) && !yaFueAtrapado(pokemonAppeared)) {
    			t_pokemon* pokemonAAtrapar = malloc(sizeof(t_pokemon));
    			pokemonAAtrapar->nombrePokemon = pokemonAppeared;
    			pokemonAAtrapar->coordenadaX = coordenadaX;
    			pokemonAAtrapar->coordenadaY = coordenadaY;

    			sem_wait(&semaforoAgregadoPokemones);

    			list_add(pokemonesEnMapa, pokemonAAtrapar);
    			list_add(pokemonesAppeared,pokemonAAtrapar->nombrePokemon);
       			log_info(loggerObligatorio,"Pokemon agregado: %s, ubicado en X:%d  Y:%d",pokemonAppeared, coordenadaX, coordenadaY);

       			//enviarACK(
    			if(esSocketBroker(socket)){
    		    enviarACKBroker(ID_APPEARED, t_APPEARED);
    			}else{
    				envioDeACKGameBoy(socket,ID_APPEARED, t_APPEARED);
    			}

    			destruirInfoPaquete(infoAppeared);
    		    pthread_mutex_unlock(&mutexPokemonesEnMapa);

    		    //Esta validación se hace para que, una vez finalizadas las transicciones "NEW", no entorpezcan la ejecución.
    		    if(!list_is_empty(colaNew)){
    		    pthread_mutex_unlock(&mutexEntrenadoresReady);
    		    }

    		    //Este post, como se explicitó en planificarEntradaAReady, es para atacar la espera activa
    		    if(!hayPokemones){
    		    sem_post(&semaforoPlanificacionInicialReady);
    		    hayPokemones = true;
    		    }
    }
    else{
    	        if(esSocketBroker(socket)){
    	        	enviarACKBroker(ID_APPEARED,t_APPEARED);
    	        }
    	        destruirInfoPaquete(infoAppeared);
    			log_info(logger,"El mensaje de appeared del pokemon %s ya fue recibido o no necesita atraparse, queda descartado",pokemonAppeared);
    			free(pokemonAppeared);
    			pthread_mutex_unlock(&mutexPokemonesEnMapa);
    }

}


bool estaEnLocalizedOAppeared(char *pokemon){
	return existeID(pokemon,pokemonesAppeared,stringComparator) || existeID(pokemon,pokemonesLocalized,stringComparator);
}


void procedimientoMensajeLocalized(t_infoPaquete *infoLocalized){
	pthread_mutex_lock(&mutexPokemonesEnMapa);
	void *paqueteLocalized = infoLocalized->paquete;
	char* pokemonLocalized = unpackPokemonLocalized(paqueteLocalized);
	uint32_t IDCorrelativo = unpackIDCorrelativo(paqueteLocalized);
	log_info(logger,"ID CORRELATIVO LOCALIZED: %d",IDCorrelativo);
	uint32_t ID = unpackID(paqueteLocalized);
	uint32_t tamanioPokemon = strlen(pokemonLocalized);
	uint32_t cantidadPokemones =unpackCantidadParesCoordenadas_Localized(paqueteLocalized,tamanioPokemon);
	//PUEDE PASAR QUE UN LOCALIZED NO TENGA POKEMONES, ME ATAJO ACA ADENTRO
	if (cantidadPokemones == 0){
		enviarACKBroker(ID, t_LOCALIZED);
		log_info(logger,"El mensaje de localized/appeared del pokemon %s no contiene pokemones, queda descartado",pokemonLocalized);
		destruirInfoPaquete(infoLocalized);
		free(pokemonLocalized);
		pthread_mutex_unlock(&mutexPokemonesEnMapa);
	}
	//SI VERIFICA LAS MISMAS CONDICIONES QUE APPEARED Y ENCIMA ES DE UN ID CREADO POR UN MENSAJE GET ENTRA
	else if (estaEnElObjetivo(pokemonLocalized) && !yaFueAtrapado(pokemonLocalized) && correspondeAUnIDDe(mensajesGET, IDCorrelativo)
			 && !estaEnLocalizedOAppeared(pokemonLocalized)) {
		int index = list_get_index(mensajesGET,&IDCorrelativo,intComparator);
		uint32_t *IDABorrar = list_remove(mensajesGET,index);
		free(IDABorrar);
		uint32_t desplazamiento = tamanioPokemon + 4*sizeof(uint32_t);
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


			sem_wait(&semaforoAgregadoPokemones);

			list_add(pokemonesEnMapa, pokemonAAtrapar);
			list_add(pokemonesLocalized,pokemonAAtrapar->nombrePokemon);
			pthread_mutex_unlock(&mutexPokemonesEnMapa);

		    if(!list_is_empty(colaNew)){
		    pthread_mutex_unlock(&mutexEntrenadoresReady);
		    }

		    if(!hayPokemones){
		    sem_post(&semaforoPlanificacionInicialReady);
		    hayPokemones = true;
		    }

		}
		enviarACKBroker(ID, t_LOCALIZED);
		destruirInfoPaquete(infoLocalized);
	}
	else{
	enviarACKBroker(ID, t_LOCALIZED);
	log_info(logger,"El mensaje de localized/appeared del pokemon %s ya fue recibido o no necesita atraparse, queda descartado",pokemonLocalized);
	destruirInfoPaquete(infoLocalized);
	free(pokemonLocalized);
	pthread_mutex_unlock(&mutexPokemonesEnMapa);
	}
}

void procedimientoMensajeCaught(t_infoPaquete *infoCaught){
	void *paqueteCaught = infoCaught->paquete;
	uint32_t IDCorrelativo = unpackIDCorrelativo(paqueteCaught);
	uint32_t ID = unpackID(paqueteCaught);
	bool resultadoCaught = unpackResultado_Caught(paqueteCaught);

	log_info(logger,"BROKER ID %d y CORRELATIVE ID :%d",ID,IDCorrelativo);
	mostrarContenidoLista(mensajesCATCH,imprimirNumero);

	uint32_t *obtenerIDCatch(t_entrenador *entrenador){
		return &(entrenador->IdCatch);
	}

	if (correspondeAUnIDDe(mensajesCATCH, IDCorrelativo)) {
		 enviarACKBroker(ID,t_CAUGHT); //CONFIRMO LA LLEGADA DEL MENSAJE
		 //SI CORRESPONDE LE ASIGNO EL POKEMON AL ENTRENADOR Y LO DESBLOQUEO (PASA A READY)
		 //Hallar posicion elemento en la lista
		 t_list *idsCatchEntrenadores = list_map(colaBlocked,(void *)obtenerIDCatch);
		 int index = list_get_index(idsCatchEntrenadores,&IDCorrelativo,intComparator);
		 t_entrenador* entrenadorQueAtrapa = list_get(colaBlocked,index);
		 log_error(logger,"ID DEL ENTRENADOR QUE ATRAPA %d",entrenadorQueAtrapa->idEntrenador);
		 entrenadorQueAtrapa->llegoMensajeCaught = true;

		 if(entrenadorQueAtrapa->operacionEntrenador == 0){
			 completarCatch(entrenadorQueAtrapa,resultadoCaught);
		 }else{

			 contadorCaught++;
			 completarCatch(entrenadorQueAtrapa,1);
		 }

		 destruirInfoPaquete(infoCaught);

		 list_clean(idsCatchEntrenadores);
		 list_destroy(idsCatchEntrenadores);
	}
	else{
	enviarACKBroker(ID,t_CAUGHT);
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
	//char* logPath = "/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/Team/Team.log";
	char* nombreArch = "Team";
	logger = log_create(team_log_file, nombreArch, 0, LOG_LEVEL_INFO);
	log_info(logger, "El logger general se creo con exito!");
}


void crearLoggerObligatorio(){
	//char* logPath = "/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/Team/TeamObligatorio.log";
	char* nombreArch = "Log_obligatorio";
	loggerObligatorio = log_create(team_log_obligatorio_file, nombreArch, 1, LOG_LEVEL_INFO);
	log_info(logger, "El logger obligatorio se creo con exito");
}

void leerArchivoDeConfiguracion(){
	char* configPath = "Team.config";
	archivoConfig = config_create(configPath);
	if (archivoConfig == NULL){
		//log_error(logger,"Archivo de configuracion no encontrado");
	}
	setearValores(archivoConfig);
	//log_info(logger,"La configuracion fue cargada exitosamente");
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
	team_log_obligatorio_file = config_get_string_value(archivoConfig, "LOG_OBLIGATORIO_FILE");
	identificadorProceso = config_get_string_value(archivoConfig,"ID");
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
	if (resultadoEnvio != -1){
		log_info(logger, "El handshake para subscribirse a una cola se ha realizado con exito");
	}
	else{
		log_error(logger, "El handshake ha fallado");
	}

	free(paquete);
}

int conectarseAColaMensaje(int socket,char *identificador,t_operacion operacion){
	void *paquete = pack_Handshake(identificador,operacion);
	uint32_t tamPaquete = strlen(identificador) + 1 + sizeof(uint32_t) + sizeof(t_operacion);
	int resultado = packAndSend(socket,paquete,tamPaquete,t_HANDSHAKE);
	free(paquete);
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
	while(posiciones_entrenadores[i] != NULL){
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
	pokemonesLocalized = list_create();
	pokemonesAppeared = list_create();
	log_info(logger, "Se han inicializado todas las colas para la planificacion");
}

void inicializarEntrenadores(){

	void limpiarPunteroAPuntero(char** puntero){
		int i = 0;

		while(puntero[i] != NULL){
			free(puntero[i]);
			i++;
		}
		free(puntero);
	}

	int IDMAX = 0;
	int cantidadPokemonesEntrenador = 0;
	int cantidadEntrenadores = obtenerCantidadEntrenadores();
	for (int i=0; i<cantidadEntrenadores; i++){
		t_entrenador* entrenadorNuevo = malloc(sizeof(t_entrenador));
		entrenadorNuevo->idEntrenador = IDMAX;
		IDMAX++;
		entrenadorNuevo->completoObjetivo = false;
		entrenadorNuevo->ocupado = false;
		entrenadorNuevo->blockeado = false;
		entrenadorNuevo->rafagasEstimadas = 0;
		entrenadorNuevo->rafagasEjecutadas = 0;
		entrenadorNuevo->distancia = 0;
		entrenadorNuevo->seCalculoEstimacion = false;
		entrenadorNuevo->contadorRR = 0;
		entrenadorNuevo->distanciaAEntrenador = 0;
		entrenadorNuevo->llegoMensajeCaught = false;
		entrenadorNuevo->pokemonAAtrapar = NULL;
		entrenadorNuevo->entrenadorAIntercambiar = NULL;
		entrenadorNuevo->pokemonQueNecesito = NULL;
		char** coordenadas = string_split(posiciones_entrenadores[i], "|");
		int coordenadaX = atoi(coordenadas[0]);
		int coordenadaY = atoi(coordenadas[1]);
		limpiarPunteroAPuntero(coordenadas);
		entrenadorNuevo->coordenadaX = coordenadaX;
		entrenadorNuevo->coordenadaY = coordenadaY;
		entrenadorNuevo->pokemones = list_create();
		entrenadorNuevo->objetivo = list_create();
		entrenadorNuevo->pokemonesQueFaltan = list_create();
		entrenadorNuevo->operacionEntrenador = t_atraparPokemon;
		entrenadorNuevo->ciclosEjecutados = 0;

		int j = 0;
		int tamPokemonEntrenador = tamArray(pokemon_entrenadores);
		if (tamPokemonEntrenador != 0){
			if(cantidadPokemonesEntrenador < tamPokemonEntrenador){
				if (!charContains(pokemon_entrenadores[i], '|')){
					list_add(entrenadorNuevo->pokemones, pokemon_entrenadores[i]);
					list_add(pokemonesAtrapados,pokemon_entrenadores[i]);
				}
				else{
					char** pokemones = string_split(pokemon_entrenadores[i], "|");
					while(pokemones[j]!= NULL){
						char *pokemonAAgregar = string_duplicate(pokemones[j]);
						list_add(entrenadorNuevo->pokemones,pokemonAAgregar);
						list_add(pokemonesAtrapados,pokemonAAgregar);
						j++;
					}
					limpiarPunteroAPuntero(pokemones);
				}
			}
		}
		else{
			log_info(logger, "El entrenador no posee pokemones al momento de su creacion");
		}

		int k = 0;
		int tamObjetivoEntrenador = tamArray(objetivos_entrenadores);
		if(cantidadPokemonesEntrenador < tamObjetivoEntrenador){
			if (!charContains(objetivos_entrenadores[i], '|')){
				list_add(entrenadorNuevo->objetivo, objetivos_entrenadores[i]);
				list_add(objetivoTeam,objetivos_entrenadores[i]);
			}
			else{
				char** objetivos = string_split(objetivos_entrenadores[i], "|");
				while(objetivos[k]!=NULL){
					//VOY A ASUMIR QUE SI YO MANDO GETS REPETIDOS EL BROKER LOS VA A IGNORAR
					char *objetivo = string_duplicate(objetivos[k]);
					list_add(objetivoTeam, objetivo);
					list_add(entrenadorNuevo->objetivo,objetivo);
					k++;
				}
				limpiarPunteroAPuntero(objetivos);
			}
		}

		cantidadPokemonesEntrenador++;

		//AGREGO EL HILO DE CADA ENTRENADOR A LA COLA DE NEW
		list_add(colaNew, entrenadorNuevo);
		list_add(entrenadores, entrenadorNuevo);
		entrenadorNuevo->cantPokemonesPorAtrapar = list_size(entrenadorNuevo->objetivo) - list_size(entrenadorNuevo->pokemones);
		log_info(loggerObligatorio, "Se ha pasado al entrenador %d a la cola de NEW. RAZON: Creacion de entrenador",entrenadorNuevo->idEntrenador);

	}

	log_info(logger, "Se han cargado todos los entrenadores");
	log_info(logger, "Se ha definido el objetivo global del team");
}

int tamArray (char** puntero){
	int i = 0;

	while(puntero[i] != NULL){
		i++;
	}
	return i;
}

bool charContains(char *cadena,char caracter){
    int i = 0;
    bool resultado = false;
    for(i = 0; i < strlen(cadena);i++){
        if(cadena[i] == caracter){
            return !resultado;
        }
    }
    return resultado;
}



void quitarPokemonesInnecesarios(t_list *pokemones){

	for(int i = 0; i < list_size(entrenadores);i++){
		t_entrenador *entrenador = list_get(entrenadores,i);
		for(int j = 0; j < list_size(entrenador->pokemones);j++){
			char *pokemon = list_get(entrenador->pokemones,j);
			if(existeID(pokemon,pokemones,stringComparator)){
				int index = list_get_index(pokemones,pokemon,stringComparator);
				list_remove(pokemones,index);
			}
		}
	}

	for(int i = 0; i < list_size(pokemones);i++){
		char *pokemon = list_get(pokemones,i);
		int cantOcurr = cantOcurrencias(pokemon,pokemones);

		if(cantOcurr > 1){
			for(int j = 0; j < cantOcurr-1;j++){
				int index = list_get_index(pokemones,pokemon,stringComparator);
				list_remove(pokemones,index);
			}
		}

	}

}

void enviarPokemonesAlBroker(){
	//ESTO HACE UN CONNECT AL BROKER Y POR CADA TIPO DE POKEMON QUE ESTE EN EL OBJETIVO GLOBAL SE ENVIA UN GET Y SE CIERRA LA CONEXION
		t_list* pokemonsAPedir = list_duplicate(objetivoTeam);
		quitarPokemonesInnecesarios(pokemonsAPedir);
		mostrarContenidoLista(pokemonsAPedir,imprimirString);
		int cantPokemones = list_size(pokemonsAPedir);
		for(int i=0; i<cantPokemones; i++){
			char* pokemonAPedir = list_get(pokemonsAPedir, i);
			enviarGET(pokemonAPedir);
		}
		log_info(logger, "Se han enviado los GETs necesarios al broker");

		list_clean(pokemonsAPedir);
		list_destroy(pokemonsAPedir);

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
	free(paquete);
}

void enviarCATCH(char* pokemon, uint32_t coordenadaX, uint32_t coordenadaY){
	int socket = conectarse_a_un_servidor(ip_broker, puerto_broker, logger);
	uint32_t id = -1;
	void* paquete = pack_Catch(id, pokemon, coordenadaX, coordenadaY);
	uint32_t tamPaquete = sizeof(uint32_t)*4 + strlen(pokemon);
	packAndSend(socket, paquete, tamPaquete, t_CATCH);
	log_info(logger, "El envio del CATCH se realizo con exito");
	procedimientoMensajeID(socket);
	close(socket);
	free(paquete);
}


void envioDeACKGameBoy(int conexion, uint32_t id, t_operacion operacion){
	void* paquete = pack_Ack(id, operacion, "Team");
	uint32_t tamPaquete = sizeof(id) + sizeof(t_operacion) + strlen("Team") + 1 + sizeof(uint32_t);
	packAndSend(conexion, paquete, tamPaquete, t_ACK);
	log_info(loggerObligatorio, "El envio del ACK del mensaje con ID [%d] se realizo con exito",id);

	free(paquete);
}


void enviarACKBroker(uint32_t ID, t_operacion operacion){
	int socket = conectarse_a_un_servidor(ip_broker, puerto_broker, logger);
	void* paquete = pack_Ack(ID, operacion, identificadorProceso);
	uint32_t tamPaquete = sizeof(ID) + sizeof(t_operacion) + strlen(identificadorProceso) + 1 + sizeof(uint32_t);
	packAndSend(socket, paquete, tamPaquete, t_ACK);
	log_info(logger, "El envio del ACK del mensaje con ID [%d] se realizo con exito",ID);
	close(socket);
	free(paquete);
}

void inicializarSemaforosEntrenadores(){
	int cantidadEntrenadores = obtenerCantidadEntrenadores();
	int i;
	for (i=0; i<cantidadEntrenadores; i++){
		t_entrenador* unEntrenador = list_get(entrenadores, i);
		sem_t semaforo = unEntrenador->semaforoEntrenador;
		sem_init(&semaforo,0,0);
	}
	log_info(logger, "Se cargaron los semaforos de los entrenadores");
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
	//CONTROLO LAS APARICIONES DEL POKEMON EN EL OBJETIVO TEAM
	//Y LO VERIFICO CONTRA LAS APARICIONES DEL POKEMON EN CADA UNO DE LOS POKEMONES QUE TIENEN LOS ENTRENADORES
	//NO PUEDO ATRAPAR MAS POKEMONES DE UN TIPO QUE DE LOS QUE TENGO EN EL OBJETIVO DEL TEAM

	int ocurrenciasGlobales = cantOcurrencias(pokemon, objetivoTeam);
	t_list *nombrePokemonesEnMapa = list_map(pokemonesEnMapa,(void *)obtenerPokemon);
	int ocurrenciasEnMapa = cantOcurrencias(pokemon,nombrePokemonesEnMapa);
	int ocurrenciasEntrenadores = 0;
	int i;

	int cantEntrenadores = list_size(entrenadores);
	for (i=0; i<cantEntrenadores; i++){
		t_entrenador* unEntrenador = list_get(entrenadores,i);
		int ocurrenciasEntrenador = cantOcurrencias(pokemon, unEntrenador->pokemones);
		ocurrenciasEntrenadores += ocurrenciasEntrenador;
	}

	list_clean(nombrePokemonesEnMapa);
	list_destroy(nombrePokemonesEnMapa);

	printf("\n\n\n\n\nCantidad de ocurrencias: Globales %d EnMapa %d Entrenadores %d\n\n\n\n",ocurrenciasGlobales,ocurrenciasEnMapa,ocurrenciasEntrenadores);

	if(ocurrenciasGlobales == (ocurrenciasEnMapa + ocurrenciasEntrenadores)){
		return true;
	}
	else{
		return false;
	}

}

char* obtenerPokemon(t_pokemon* unPokemon){
	return unPokemon->nombrePokemon;
}

bool hayPokemonesParaAtrapar(){
	int pokemonesAtrapadoss = list_size(pokemonesAtrapados);
	int pokemonesPorAtrapar = list_size(objetivoTeam);
	return (pokemonesAtrapadoss < pokemonesPorAtrapar);
}

void capturaPokemonFIFO(t_entrenador *entrenadorAEjecutar){
	log_info(logger, "Se procedera a ejecutar al entrenador %d seleccionado por el algoritmo", entrenadorAEjecutar->idEntrenador);

	entrenadorAEjecutar->ocupado = true;

	//EJECUTAR SIN QUANTUM

	        /*
			if(entrenadorAEjecutar->pokemonAAtrapar == NULL){
				log_info(logger,"CANT POKEMONES ANTE DE HALLAR EL POKE MAS CERCANO %d",list_size(pokemonesEnMapa));
				pokemonAAtrapar = pokemonMasCercanoA(entrenadorAEjecutar);
				entrenadorAEjecutar->pokemonAAtrapar = pokemonAAtrapar;
			}
			*/


			int distanciaAPokemon = entrenadorAEjecutar->pokemonAAtrapar->distanciaAEntrenador;
			log_info(logger,"DISTANCIA POKEMON ANTES DEL LOG: %d",distanciaAPokemon);
			log_info(logger, "POKEMON A ATRAPAR %s  y distancia  %d",entrenadorAEjecutar->pokemonAAtrapar->nombrePokemon, distanciaAPokemon);
			moverEntrenador(entrenadorAEjecutar,entrenadorAEjecutar->pokemonAAtrapar);
			sleep((retardo_ciclo_cpu*distanciaAPokemon));
			entrenadorAEjecutar->rafagasEjecutadas = distanciaAPokemon; //Una rafaga por cada unidad de distancia que se mueve + una rafaga para mandar un mensaje al broker
			ciclosTotales += distanciaAPokemon;
			entrenadorAEjecutar->ciclosEjecutados += distanciaAPokemon;
			atraparPokemon(entrenadorAEjecutar,entrenadorAEjecutar->pokemonAAtrapar);
			//SI ESTA CONECTADO AL BROKER ESPERO LA RESPUESTA
			if(conexionAppeared){
				while(1){
					if(llegoRespuesta){
						//CUANDO LLEGA LA RESPUESTA HABILITO LA EJECUCION
						sem_wait(&semaforoRespuestaCatch);
						llegoRespuesta = false;
						//ME TRAIGO EL ULTIMO ELEMENTO DE LA LISTA DE MENSAJES CATCH, ADD AGREGA AL FINAL DE LA LISTA
						int sizeMensajesCatch = list_size(mensajesCATCH);
						int index = sizeMensajesCatch-1;
						uint32_t* IDCATCH = list_get(mensajesCATCH,index);
						entrenadorAEjecutar->IdCatch = *IDCATCH;
						list_remove(colaExec,0);
						entrenadorAEjecutar->blockeado = true;
						list_add(colaBlocked,entrenadorAEjecutar);
						log_info(loggerObligatorio, "Se cambió un entrenador de EXEC a BLOCKED, Razon: Esta a la espera de un mensaje CAUGHT");
						//CUANDO SALGO DE EXEC HAGO EL POST
						//sem_post(&semaforoPlanificacionExec);
						cambiosDeContexto++;
						}
					break;
				}
			log_info(logger,"sali del while .......");
			}
			else{
				//SI NO ESTA CONECTADO AL BROKER, ASUMO QUE FUE ATRAPADO
				completarCatch(entrenadorAEjecutar, 1);
			}

}

void capturaPokemonRR(t_entrenador *entrenadorAEjecutar){
	//EJECUTAR CON QUANTUM
	log_info(logger, "Se procedera a ejecutar al entrenador %d seleccionado por el algoritmo", entrenadorAEjecutar->idEntrenador);

	//t_pokemon *pokemonAAtrapar;
	entrenadorAEjecutar->ocupado = true;

	        /*
			if(entrenadorAEjecutar->pokemonAAtrapar == NULL){
				pokemonAAtrapar = pokemonMasCercanoA(entrenadorAEjecutar);
				entrenadorAEjecutar->pokemonAAtrapar = pokemonAAtrapar;
			}
			*/

			int distanciaAPokemon = entrenadorAEjecutar->pokemonAAtrapar->distanciaAEntrenador;
			log_info(logger,"DISTANCIA POKEMON ANTES DEL LOG: %d",distanciaAPokemon);
			log_info(logger, "POKEMON A ATRAPAR %s  y distancia  %d",entrenadorAEjecutar->pokemonAAtrapar->nombrePokemon, distanciaAPokemon);

			entrenadorAEjecutar->contadorRR += quantum;
			if(entrenadorAEjecutar->contadorRR >= distanciaAPokemon){
				//ES COMO SI ME MOVIERA UNA CANTIDAD DE POSICIONES IGUAL AL QUANTUM, SI NO LLEGO AL POKEMON DURANTE EL QUANTUM NO HAGO NADA
				//CUANDO MI CONTADOR SEA IGUAL O MAYOR A LA DISTANCIA RECIEN AHI PASO A ATRAPAR EL POKEMON
				moverEntrenador(entrenadorAEjecutar, entrenadorAEjecutar->pokemonAAtrapar);
				sleep(retardo_ciclo_cpu*distanciaAPokemon); //ASI NO ESPERO MAS DE LO QUE DEBO
				ciclosTotales += distanciaAPokemon;
				entrenadorAEjecutar->ciclosEjecutados += distanciaAPokemon;
				atraparPokemon(entrenadorAEjecutar, entrenadorAEjecutar->pokemonAAtrapar);
				if(conexionAppeared){
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
							//CUANDO SALGO DE EXEC HAGO EL POST
							sem_post(&semaforoPlanificacionExec);
							cambiosDeContexto++;
							break;
						}
					}
				}
				else{
					//SI NO ESTA CONECTADO AL BROKER, ASUMO QUE FUE ATRAPADO
					completarCatch(entrenadorAEjecutar, 1);
				}
			}
			else{
				//SI TODAVIA NO COMPLETE ESE DELAY LO MANDO DE NUEVO A READY
				list_remove(colaExec,0);
				list_add(colaReady,entrenadorAEjecutar);
				log_info(loggerObligatorio, "Se cambió un entrenador de EXEC a READY, Razon: Fin de quantum");
				//CUANDO SALGO DE EXEC HAGO EL POST
				//sem_post(&semaforoPlanificacionExec);
				//pthread_mutex_unlock(&mutexEntrenadoresReady);
				cambiosDeContexto++;
			}
}

void capturaPokemonSJFConDesalojo(t_entrenador *entrenadorAEjecutar){
	log_info(logger, "Se procedera a ejecutar al entrenador %d seleccionado por el algoritmo", entrenadorAEjecutar->idEntrenador);

		//t_pokemon* pokemonAAtrapar;
		entrenadorAEjecutar->ocupado = true;

		//EJECUTAR SIN QUANTUM

				int distanciaAPokemon = entrenadorAEjecutar->pokemonAAtrapar->distanciaAEntrenador;
				moverEntrenadorConDesalojo(entrenadorAEjecutar,entrenadorAEjecutar->pokemonAAtrapar);
				ciclosTotales += distanciaAPokemon;
				entrenadorAEjecutar->ciclosEjecutados += distanciaAPokemon;
				atraparPokemon(entrenadorAEjecutar,entrenadorAEjecutar->pokemonAAtrapar);
				//SI ESTA CONECTADO AL BROKER ESPERO LA RESPUESTA
				if(conexionAppeared){
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
							//CUANDO SALGO DE EXEC HAGO EL POST
							sem_post(&semaforoPlanificacionExec);
							cambiosDeContexto++;
							break;
							}
					}
				}
				else{
					//SI NO ESTA CONECTADO AL BROKER, ASUMO QUE FUE ATRAPADO
					completarCatch(entrenadorAEjecutar, 1);
				}

}

void intercambioPokemonSinQuantum(t_entrenador *entrenadorAEjecutar){

	bool recibePokemonQueLeFalta(char *pokemon,t_entrenador *entrenador){
		return existeID(pokemon,entrenador->pokemonesQueFaltan,stringComparator);
	}
	entrenadorAEjecutar->ocupado = true;
	/*
	if (entrenadorAEjecutar->pokemonQueNecesito == NULL){
		char *pokemonQueNecesito = list_remove(entrenadorAEjecutar->pokemonesQueFaltan,0);
		entrenadorAEjecutar->pokemonQueNecesito = pokemonQueNecesito;
	}
	*/
	t_entrenador* entrenadorAIntercambiar;
	char *pokemonQueYoNoNecesito;
	//EJECUTAR SIN QUANTUM
			//entrenadorAIntercambiar = obtenerEntrenadorQueTieneMiPokemon(entrenadorAEjecutar, entrenadorAEjecutar->pokemonQueNecesito);
			entrenadorAIntercambiar = entrenadorAEjecutar->entrenadorAIntercambiar;
			log_info(logger,"El entrenador %d  hara un intercambio con el entrenador %d", entrenadorAEjecutar->idEntrenador, entrenadorAIntercambiar->idEntrenador);
			pokemonQueYoNoNecesito = obtenerPokemonParaIntercambiar(entrenadorAEjecutar,entrenadorAIntercambiar);

			if(recibePokemonQueLeFalta(pokemonQueYoNoNecesito,entrenadorAIntercambiar)){
				int index = list_get_index(entrenadorAIntercambiar->pokemonesQueFaltan,pokemonQueYoNoNecesito,(void *) comparadorNombrePokemones);
				list_remove(entrenadorAIntercambiar->pokemonesQueFaltan,index);
			}

			int distanciaAEntrenador = (abs(entrenadorAEjecutar->coordenadaX - entrenadorAIntercambiar->coordenadaX) + abs(entrenadorAEjecutar->coordenadaY - entrenadorAIntercambiar->coordenadaY));

			moverEntrenadorParaIntercambio(entrenadorAEjecutar, entrenadorAIntercambiar);
			sleep((retardo_ciclo_cpu*distanciaAEntrenador));
			ciclosTotales += distanciaAEntrenador;
			entrenadorAEjecutar->ciclosEjecutados += distanciaAEntrenador;
			entrenadorAEjecutar->rafagasEjecutadas = distanciaAEntrenador;
			log_info(loggerObligatorio, "Se comenzará el intercambio entre el entrenador %d  y el entrenador %d", entrenadorAEjecutar->idEntrenador, entrenadorAIntercambiar->idEntrenador);
			log_info(logger, "El entrenador %d recibira a %s  y el entrenador %d recibira a %s", entrenadorAEjecutar->idEntrenador, entrenadorAEjecutar->pokemonQueNecesito,
					entrenadorAIntercambiar->idEntrenador, pokemonQueYoNoNecesito);
			sleep(retardo_ciclo_cpu*5);  //CONSUME 5 CICLOS DE CPU
			ciclosTotales += 5;
			entrenadorAEjecutar->ciclosEjecutados += 5;

			realizarIntercambio(entrenadorAEjecutar, entrenadorAIntercambiar,entrenadorAEjecutar->pokemonQueNecesito, pokemonQueYoNoNecesito);
			log_info(logger,"POKEMON QUE VA A ATRAPAR EL ENTRENADOR %d ES %s",entrenadorAEjecutar->idEntrenador,entrenadorAEjecutar->pokemonAAtrapar->nombrePokemon);
			log_info(logger,"POKEMON QUE VA A ATRAPAR EL ENTRENADOR %d ES %s",entrenadorAIntercambiar->idEntrenador,entrenadorAIntercambiar->pokemonAAtrapar->nombrePokemon);

			if(conexionAppeared){
				//INTERCAMBIO CON EL BROKER
				list_remove(colaExec,0);
				entrenadorAEjecutar->blockeado = true;
				log_error(logger,"POKEMON A ATRAPAR %s",entrenadorAEjecutar->pokemonAAtrapar->nombrePokemon);
				list_add(colaBlocked,entrenadorAEjecutar);
				log_info(loggerObligatorio, "Se cambió un entrenador de EXEC a BLOCKED, Razon: Esta a la espera de un mensaje CAUGHT");
				//CUANDO SALGO DE EXEC HAGO EL POST
				//sem_post(&semaforoPlanificacionExec);
				cambiosDeContexto++;
			}
			else{
				completarCatch(entrenadorAEjecutar, 1);
				contadorCaught = 2;
				completarCatch(entrenadorAIntercambiar, 1);
			}

}

void intercambioPokemonRR(t_entrenador *entrenadorAEjecutar){

	bool recibePokemonQueLeFalta(char *pokemon,t_entrenador *entrenador){
		return existeID(pokemon,entrenador->pokemonesQueFaltan,stringComparator);
	}
	entrenadorAEjecutar->ocupado = true;

	t_entrenador* entrenadorAIntercambiar;

	char *pokemonQueYoNoNecesito;

	//NO ESTOY MUY CONVENCIDO DE ESTA CONDICION
			int distanciaAEntrenador = (abs(entrenadorAEjecutar->coordenadaX - entrenadorAEjecutar->entrenadorAIntercambiar->coordenadaX) +
					abs(entrenadorAEjecutar->coordenadaY - entrenadorAEjecutar->entrenadorAIntercambiar->coordenadaY));

			//EJECUTAR CON QUANTUM
			entrenadorAIntercambiar = entrenadorAEjecutar->entrenadorAIntercambiar;
			pokemonQueYoNoNecesito = obtenerPokemonParaIntercambiar(entrenadorAEjecutar,entrenadorAIntercambiar);
			entrenadorAEjecutar->contadorRR += quantum;
			sleep(retardo_ciclo_cpu* quantum);
			ciclosTotales += quantum;
			entrenadorAEjecutar->ciclosEjecutados += quantum;
			if(entrenadorAEjecutar->contadorRR >= distanciaAEntrenador){
				//ES COMO SI ME MOVIERA UNA CANTIDAD DE POSICIONES IGUAL AL QUANTUM, SI NO LLEGO AL POKEMON DURANTE EL QUANTUM NO HAGO NADA
				//CUANDO MI CONTADOR SEA IGUAL O MAYOR A LA DISTANCIA RECIEN AHI PASO A ATRAPAR EL POKEMON
				moverEntrenadorParaIntercambio(entrenadorAEjecutar, entrenadorAIntercambiar);
				ciclosTotales += distanciaAEntrenador;
				entrenadorAEjecutar->ciclosEjecutados += distanciaAEntrenador;
				log_info(loggerObligatorio, "Se comenzará el intercambio entre el entrenador %d  y el entrenador %d", entrenadorAEjecutar->idEntrenador, entrenadorAIntercambiar->idEntrenador);
				log_info(logger, "El entrenador %d recibira a %s  y el entrenador %d recibira a %s", entrenadorAEjecutar->idEntrenador, entrenadorAEjecutar->pokemonQueNecesito,
						entrenadorAIntercambiar->idEntrenador, pokemonQueYoNoNecesito);
				sleep(retardo_ciclo_cpu*5);  //CONSUME 5 CICLOS DE CPU
				ciclosTotales += 5;
				entrenadorAEjecutar->ciclosEjecutados += 5;

				realizarIntercambio(entrenadorAEjecutar, entrenadorAIntercambiar, entrenadorAEjecutar->pokemonQueNecesito, pokemonQueYoNoNecesito);
				if(conexionAppeared){
					//INTERCAMBIO CON EL BROKER
					list_remove(colaExec,0);
					entrenadorAEjecutar->blockeado = true;
					list_add(colaBlocked,entrenadorAEjecutar);
					log_info(loggerObligatorio, "Se cambió un entrenador de EXEC a BLOCKED, Razon: Esta a la espera de un mensaje CAUGHT");
					//CUANDO SALGO DE EXEC HAGO EL POST
					//sem_post(&semaforoPlanificacionExec);
					cambiosDeContexto++;
				}
				else{
					completarCatch(entrenadorAEjecutar, 1);
					contadorCaught = 2;
					completarCatch(entrenadorAIntercambiar, 1);
				}
			}
			else{
				//SI TODAVIA NO COMPLETE ESE DELAY LO MANDO DE NUEVO A READY
				list_remove(colaExec,0);
				list_add(colaReady,entrenadorAEjecutar);
				log_info(loggerObligatorio, "Se cambió un entrenador de EXEC a READY, Razon: Fin de quantum");
				//CUANDO SALGO DE EXEC HAGO EL POST
				//sem_post(&semaforoPlanificacionExec);
				cambiosDeContexto++;
			}
}


t_planificador *construirPlanificador(){
	t_planificador *singletonPlanificador = malloc(sizeof(t_planificador));

	 if(string_equals_ignore_case(algoritmo_planificacion,"FIFO")){
		 singletonPlanificador->planificar = aplicarFIFO;
		 singletonPlanificador->ejecutarRutinaCapturaPokemon = capturaPokemonFIFO;
		 singletonPlanificador->ejecutarRutinaIntercambioPokemon = intercambioPokemonSinQuantum;
	 }else if(string_equals_ignore_case(algoritmo_planificacion,"SJF-SD")){
		 singletonPlanificador->planificar = aplicarSJF;
		 singletonPlanificador->ejecutarRutinaCapturaPokemon = capturaPokemonFIFO;
		 singletonPlanificador->ejecutarRutinaIntercambioPokemon = intercambioPokemonSinQuantum;
	 }else if(string_equals_ignore_case(algoritmo_planificacion,"RR")){
		 singletonPlanificador->planificar = aplicarRR;
		 singletonPlanificador->ejecutarRutinaCapturaPokemon = capturaPokemonRR;
		 singletonPlanificador->ejecutarRutinaIntercambioPokemon = intercambioPokemonRR;
	 }else{
		 singletonPlanificador->planificar = aplicarSJFConDesalojo;
		 singletonPlanificador->ejecutarRutinaCapturaPokemon = capturaPokemonSJFConDesalojo;
		 singletonPlanificador->ejecutarRutinaIntercambioPokemon = intercambioPokemonSinQuantum;
	 }

	 return singletonPlanificador;
}


void iniciarEjecucionEntrenador(){
	t_entrenador *entrenadorAEjecutar = list_get(colaExec,0);
	pthread_t hiloEntrenador = entrenadorAEjecutar->hiloEntrenador;
	pthread_create(&hiloEntrenador,NULL,(void*)ejecutarEntrenador,entrenadorAEjecutar);
	pthread_detach(hiloEntrenador);
}

void planificadorACortoPlazo(){
	while(1){
		sem_wait(&semaforoPlanificacionExec);

			if(planificacionCompleta && !list_is_empty(colaReady)){
				//Intercambios
				planificador->planificar();
				iniciarEjecucionEntrenador();
			}
			else if(!list_is_empty(colaReady) && list_is_empty(colaExec) && !planificacionCompleta && !list_is_empty(pokemonesEnMapa)){
				//Capturas
				planificador->planificar();
				iniciarEjecucionEntrenador();
			}

			if(list_size(entrenadores) == list_size(colaExit)){
				break;
			}
	   }

	pthread_exit(NULL);
}


int cantidadEntrenadoresBloqueadosOcupados(){
	t_list *entrenadores = list_filter(colaBlocked,(void *)estaOcupado);

	int cant = list_size(entrenadores);

	list_clean(entrenadores);
	list_destroy(entrenadores);

	return cant;
}

bool hayEntrenadoresBloqueadosOcupados(){
	return cantidadEntrenadoresBloqueadosOcupados() == list_size(colaBlocked);
}

bool llegoMensajeCaught(t_entrenador *entrenadorAEjecutar){
	return entrenadorAEjecutar->llegoMensajeCaught;
}

void ejecutarEntrenador(t_entrenador* entrenadorAEjecutar){

	if (entrenadorAEjecutar->operacionEntrenador == t_atraparPokemon){
		planificador->ejecutarRutinaCapturaPokemon(entrenadorAEjecutar);
	}
	else{
		planificador->ejecutarRutinaIntercambioPokemon(entrenadorAEjecutar);
	}

	entrenadorAEjecutar->seCalculoEstimacion = false;

	if(tipoPlanificacion == t_CATCH_STATE){

	if(list_is_empty(colaReady)){

		if(conexionAppeared){
        if(!list_is_empty(pokemonesEnMapa) && llegoMensajeCaught(entrenadorAEjecutar)){
        	pthread_mutex_unlock(&mutexEntrenadoresReady);
        }else if(list_is_empty(colaNew) && !hayEntrenadoresBloqueadosOcupados() && !planificacionCompleta){
        	pthread_mutex_unlock(&mutexEntrenadoresReady);
        }
    	}else{
			pthread_mutex_unlock(&mutexEntrenadoresReady);
		}

	}else if(list_is_empty(colaNew) && hayEntrenadoresBloqueadosOcupados()){
		sem_post(&semaforoPlanificacionExec);
	}else if(!hayEntrenadoresBloqueadosOcupados()){
		pthread_mutex_unlock(&mutexEntrenadoresReady);
	}else if(conexionAppeared && !list_is_empty(colaReady)){
		sem_post(&semaforoPlanificacionExec);
	}
	}

	if(stringComparator(algoritmo_planificacion,"RR") && tipoPlanificacion == t_EXCHANGE_STATE){
		if(!list_is_empty(colaReady)){
			sem_post(&semaforoPlanificacionExec);
		}
	}

	//Este if es para que,cuando, en la última iteración, no haya conexión con el Broker,el planificador de corto plazo se mate.
	if(list_size(entrenadores) == list_size(colaExit)){
		sem_post(&semaforoPlanificacionExec);
	}

}

t_pokemon *crearPokemon(char *nombrePokemon,uint32_t coordX,uint32_t coordY){
	t_pokemon *pokemon = malloc(sizeof(t_pokemon));
	pokemon->nombrePokemon = nombrePokemon;
	pokemon->coordenadaX = coordX;
	pokemon->coordenadaY = coordY;
	pokemon->distanciaAEntrenador = -1;

	return pokemon;
}

void realizarIntercambio(t_entrenador* entrenadorAEjecutar, t_entrenador* entrenadorAIntercambiar, char* pokemonQueElOtroNoNecesita, char* pokemonQueYoNoNecesito){
	t_pokemon *pokemonQueCedo = crearPokemon(pokemonQueYoNoNecesito,entrenadorAEjecutar->coordenadaX,entrenadorAEjecutar->coordenadaY);
	t_pokemon *pokemonQueRecibo = crearPokemon(pokemonQueElOtroNoNecesita,entrenadorAIntercambiar->coordenadaX,entrenadorAIntercambiar->coordenadaY);

	//entrenadorAEjecutar->pokemonAAtrapar = pokemonQueRecibo;
	//entrenadorAIntercambiar->pokemonAAtrapar = pokemonQueCedo;

	if(conexionAppeared){
		atraparPokemon(entrenadorAEjecutar,pokemonQueRecibo); //ATRAPO UN POKEMON QUE EL OTRO NO NECESITA
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
				break;
			}
		}
		//ELIMINO EL POKEMON QUE VOY A DAR AL OTRO
		int index2 = list_get_index(entrenadorAEjecutar->pokemones, pokemonQueYoNoNecesito, (void*)comparadorNombrePokemones);
		list_remove(entrenadorAEjecutar->pokemones, index2);

		atraparPokemon(entrenadorAIntercambiar,pokemonQueCedo); //EL OTRO ATRAPA UN POKEMON QUE YO NO NECESITO
		while(1){
			if(llegoRespuesta){
				//CUANDO LLEGA LA RESPUESTA HABILITO LA EJECUCION
				sem_wait(&semaforoRespuestaCatch);
				llegoRespuesta = 0;
				//ME TRAIGO EL ULTIMO ELEMENTO DE LA LISTA DE MENSAJES CATCH, ADD AGREGA AL FINAL DE LA LISTA
				int sizeMensajesCatch = list_size(mensajesCATCH);
				int index = sizeMensajesCatch-1;
				uint32_t* IDCATCH = list_get(mensajesCATCH,index);
				entrenadorAIntercambiar->IdCatch = *IDCATCH;
				break;
			}
		}
		//EL OTRO ENTRENADOR ELIMINA EL POKEMON QUE LE DIO AL OTRO ENTRENADOR
		int index3 = list_get_index(entrenadorAIntercambiar->pokemones, pokemonQueElOtroNoNecesita, (void*)comparadorNombrePokemones);
		//log_error(logger,"INDEX 3: %d y LIST SIZE POKE :%d",index3,list_size(entrenadorAEjecutar->pokemones));
		list_remove(entrenadorAIntercambiar->pokemones, index3);
	}
	else{
		log_info(loggerObligatorio, "No se puedo establecer conexion con el broker, el intercambio se hara segun la accion por default");

		//SACO EL POKEMON QUE YO NO NECESITO Y SE LO ASIGNO AL OTRO COMO POKEMON A ATRAPAR
		int index2 = list_get_index(entrenadorAEjecutar->pokemones, pokemonQueYoNoNecesito, (void*)comparadorNombrePokemones);
		list_remove(entrenadorAEjecutar->pokemones, index2);
		entrenadorAIntercambiar->pokemonAAtrapar = pokemonQueCedo;

		//EL OTRO ENTRENADOR SACA EL POKEMON QUE YA NO NECESITA Y SE LO ASIGNA EL PRIMERO COMO EL POKEMON A ATRAPAR
		int index3 = list_get_index(entrenadorAIntercambiar->pokemones, pokemonQueElOtroNoNecesita, (void*)comparadorNombrePokemones);
		list_remove(entrenadorAIntercambiar->pokemones, index3);
		entrenadorAEjecutar->pokemonAAtrapar = pokemonQueRecibo;
	}

}

bool comparadorNombrePokemones(char* unPokemon, char* otroPokemon){
	return string_equals_ignore_case(unPokemon,otroPokemon);
}

char *obtenerPokemonParaIntercambiar(t_entrenador *unEntrenador,t_entrenador *otroEntrenador){

	t_list *pokemonesUnEntrenador = unEntrenador->pokemones;

	for(int i = 0; i < list_size(pokemonesUnEntrenador);i++){
		char *pokemonAIntercambiar = list_get(pokemonesUnEntrenador,i);
		if(!estaEnObjetivo(pokemonAIntercambiar,unEntrenador) && estaEnObjetivo(pokemonAIntercambiar,otroEntrenador)){
			return pokemonAIntercambiar;
		}
	}

	return pokemonQueNoNecesito(unEntrenador);
}

bool estaRepetidoPokemon(char *pokemon,t_entrenador *entrenador){
	t_list *pokemones = entrenador->pokemones;

	return existeID(pokemon,pokemones,stringComparator);
}

char *pokemonQueNoNecesito(t_entrenador* unEntrenador){
	char *unPokemon;
	int cantPokemons = list_size(unEntrenador->pokemones);
	for(int i=0; i<cantPokemons; i++){
		unPokemon = list_get(unEntrenador->pokemones,i);
		log_info(logger,"Pokemon: %s",unPokemon);
		if(!estaEnObjetivo(unPokemon, unEntrenador)){
			return unPokemon;
		}
	}

	for(int i=0; i<cantPokemons; i++){
		unPokemon = list_get(unEntrenador->pokemones,i);
		if(estaRepetidoPokemon(unPokemon,unEntrenador)){
			return unPokemon;
		}
	}

	return NULL;
}

bool estaEnObjetivo(char* unPokemon, t_entrenador* unEntrenador){
	bool resultado = false;
	int cantPokemons = list_size(unEntrenador->pokemones);
	int i = 0;
	for(i = 0; i<cantPokemons; i++){
		char *pokemon = list_get(unEntrenador->objetivo,i);
		if (strcmp(unPokemon,pokemon) == 0){
			return !resultado;
		}
	}
	return resultado;
}

void moverEntrenadorParaIntercambio(t_entrenador* entrenadorAEjecutar, t_entrenador* otroEntrenador){
	log_info(loggerObligatorio, "Se movera al entrenador ID:%d de la posicion X:%d  Y:%d a la posicion  X:%d  Y:%d", entrenadorAEjecutar->idEntrenador,
			entrenadorAEjecutar->coordenadaX, entrenadorAEjecutar->coordenadaY, otroEntrenador->coordenadaX, otroEntrenador->coordenadaY);
	entrenadorAEjecutar->coordenadaX = otroEntrenador->coordenadaX;
	entrenadorAEjecutar->coordenadaY = otroEntrenador->coordenadaY;
}


void atraparPokemon(t_entrenador* entrenadorAEjecutar, t_pokemon* pokemonAAtrapar){
	entrenadorAEjecutar->pokemonAAtrapar = pokemonAAtrapar;
	if(conexionAppeared){
		enviarCATCH(pokemonAAtrapar->nombrePokemon, pokemonAAtrapar->coordenadaX, pokemonAAtrapar->coordenadaY);
	}
	else{
		log_info(logger, "La conexion al broker no ha podido realizarse, se procedera con la accion por default");
	}
	log_info(loggerObligatorio, "El entrenador %d  quiere atrapar a un pokemon. POKEMON:%s  COORDENADAS: X:%d Y:%d",entrenadorAEjecutar->idEntrenador, pokemonAAtrapar->nombrePokemon, pokemonAAtrapar->coordenadaX, pokemonAAtrapar->coordenadaY);
}

void moverEntrenador(t_entrenador* entrenadorAEjecutar, t_pokemon* pokemonAAtrapar){
	log_info(loggerObligatorio, "Se movera al entrenador ID:%d de la posicion X:%d  Y:%d a la posicion  X:%d  Y:%d para atrapar a %s", entrenadorAEjecutar->idEntrenador,
			entrenadorAEjecutar->coordenadaX, entrenadorAEjecutar->coordenadaY, pokemonAAtrapar->coordenadaX, pokemonAAtrapar->coordenadaY, pokemonAAtrapar->nombrePokemon);
	entrenadorAEjecutar->coordenadaX = pokemonAAtrapar->coordenadaX;
	entrenadorAEjecutar->coordenadaY = pokemonAAtrapar->coordenadaY;
}

void darUnPaso(t_entrenador *entrenador,t_pokemon *pokemon){
	pokemon->distanciaAEntrenador--;
	log_info(logger,"DISTANCIA DECREMENTADA: %d",pokemon->distanciaAEntrenador);
	entrenador->rafagasEjecutadas++;
}

void moverEntrenadorConDesalojo(t_entrenador* entrenadorAEjecutar, t_pokemon* pokemonAAtrapar){

	log_info(logger,"distancia :%d",pokemonAAtrapar->distanciaAEntrenador);
	if(pokemonAAtrapar->distanciaAEntrenador > 0){

	darUnPaso(entrenadorAEjecutar,pokemonAAtrapar);
	sleep(retardo_ciclo_cpu);

	if(!hayQueDesalojar){
	if(pokemonAAtrapar->distanciaAEntrenador != 0){
		moverEntrenadorConDesalojo(entrenadorAEjecutar,pokemonAAtrapar);
	}
	}else{
		pthread_exit(NULL);
	}
	}

}

void calcularDistanciaEntrenadorA(t_entrenador *entrenador,t_pokemon *pokemon){
	int distanciaAPokemon = (abs(entrenador->coordenadaX - pokemon->coordenadaX) + abs(entrenador->coordenadaY - pokemon->coordenadaY));
	log_error(logger,"DISTANCIA DEL ENTRENADOR %d AL POKEMON %s ES %d",entrenador->idEntrenador,pokemon->nombrePokemon,distanciaAPokemon);
			//CALCULO LA DISTANCIA AL ENTRENADOR Y LA GUARDO EN UNA VARIABLE DENTRO DEL POKEMON
	pokemon->distanciaAEntrenador = distanciaAPokemon;
}


t_pokemon* pokemonMasCercanoA(t_entrenador* unEntrenador){
	t_list* aux = list_create();
	t_pokemon* pokemonMasCercano;
	int cantPokemonesEnMapa = list_size(pokemonesEnMapa);
	if (cantPokemonesEnMapa == 1){
		pokemonMasCercano = list_get(pokemonesEnMapa,0);
		int distanciaAPokemon = (abs(unEntrenador->coordenadaX - pokemonMasCercano->coordenadaX) + abs(unEntrenador->coordenadaY - pokemonMasCercano->coordenadaY));
		log_error(logger,"DISTANCIA DEL ENTRENADOR %d AL POKEMON %s ES %d",unEntrenador->idEntrenador,pokemonMasCercano->nombrePokemon,distanciaAPokemon);
		//CALCULO LA DISTANCIA AL ENTRENADOR Y LA GUARDO EN UNA VARIABLE DENTRO DEL POKEMON
		pokemonMasCercano->distanciaAEntrenador = distanciaAPokemon;
		//list_add(aux, pokemonMasCercano);
	}
	else{
		for(int i = 0; i < cantPokemonesEnMapa; i++){
			t_pokemon* unPokemon = list_get(pokemonesEnMapa, i);
			int distanciaAPokemon = (abs(unEntrenador->coordenadaX - unPokemon->coordenadaX) + abs(unEntrenador->coordenadaY - unPokemon->coordenadaY));
			log_error(logger,"DISTANCIA DEL ENTRENADOR %d AL POKEMON %s ES %d",unEntrenador->idEntrenador,unPokemon->nombrePokemon,distanciaAPokemon);
			//CALCULO LA DISTANCIA AL ENTRENADOR Y LA GUARDO EN UNA VARIABLE DENTRO DEL POKEMON
			unPokemon->distanciaAEntrenador = distanciaAPokemon;
			list_add(aux, unPokemon);
		}
		//LUEGO DE CREAR LA LISTA AUX CON LOS POKEMONES CON LAS DISTANCIAS A ESE ENTRENADOR, LA ORDENO DE MENOR A MAYOR

		bool compararDistancia(void *unPokemon,void *otroPokemon){
			t_pokemon *pokemon1 = unPokemon;
			t_pokemon *pokemon2 = otroPokemon;

			return pokemon1->distanciaAEntrenador < pokemon2->distanciaAEntrenador;
		}

		list_sort(aux,compararDistancia);
		pokemonMasCercano = list_get(aux,0);
	}

	list_clean(aux);
	list_destroy(aux);
	return pokemonMasCercano;
}


void completarCatch(t_entrenador* unEntrenador, bool resultadoCaught){
	log_info(logger,"LLEGUE A COMPLETAR CATCH POR INTERCAMBIO");
	log_info(logger,"POKEMON QUE VOY A ATRAPAR: %s",unEntrenador->pokemonAAtrapar->nombrePokemon);

	t_pokemon* pokemonAtrapado = unEntrenador->pokemonAAtrapar;

	log_info(logger,"POKEMON ATRAPADO: %s y RESULTADO CAUGHt %d",pokemonAtrapado->nombrePokemon,resultadoCaught);
	char *nombrePokemon = pokemonAtrapado->nombrePokemon;

	if (resultadoCaught){
		//ESTE ES EL CASO DEL CAUGHT PARA ATRAPAR EL POKEMON NORMALMENTE
		if(unEntrenador->operacionEntrenador == 0){
			//sacarPokemonDelMapa(pokemonAtrapado);
			//semaforo que de el ok
			printf("Voy a sacar al pokemon: %s\n",pokemonAtrapado->nombrePokemon);
			sacarPokemonDelMapa(pokemonAtrapado);
			printf("Los pokemones en mapa son :%d despues del remove \n",list_size(pokemonesEnMapa));
			list_add(pokemonesAtrapados,nombrePokemon);
			log_info(logger,"CANT POK ATRAPADOS: %d",list_size(pokemonesAtrapados));
			list_add(unEntrenador->pokemones, nombrePokemon);
			unEntrenador->cantPokemonesPorAtrapar = unEntrenador->cantPokemonesPorAtrapar - 1;
			unEntrenador->contadorRR = 0;
			//unEntrenador->rafagasEjecutadas = 0;
			log_info(logger, "El pokemon %s pudo ser atrapado por el entrenador %d!",nombrePokemon, unEntrenador->idEntrenador);
			if(conexionAppeared){
				//SI HAY CONEXION PROCEDO NORMAL, CON EL ENTRENADOR BLOQUEADO
				completarCatchNormal(unEntrenador);
			}
			else{
				//SI NO HAY CONEXION PROCEDO POR DEFAULT, PASA DIRECTO DE EXEC A READY
				completarCatchSinBroker(unEntrenador);
			}
		}
		else{
			//ESTE CASO ES PARA EL CAUGHT CUANDO HACE EL INTERCAMBIO
			list_add(unEntrenador->pokemones, nombrePokemon);
			unEntrenador->contadorRR = 0;
			//unEntrenador->rafagasEjecutadas = 0;
			unEntrenador->entrenadorAIntercambiar = NULL;
			log_info(logger, "El pokemon %s pudo ser atrapado por el entrenador %d!",nombrePokemon, unEntrenador->idEntrenador);
			if(conexionAppeared){
				//SI HAY CONEXION PROCEDO NORMAL, CON EL ENTRENADOR BLOQUEADO
				completarIntercambioNormal(unEntrenador);
			}
			else{
				//SI NO HAY CONEXION PROCEDO POR DEFAULT, PASA DIRECTO DE EXEC A READY
				completarIntercambioSinBroker(unEntrenador);
			}
		}
	}
	else{
		//SI FALLA EL CATCH VA A READY
		log_info(logger, "El pokemon %s no pudo ser atrapado por el entrenador %d", nombrePokemon, unEntrenador->idEntrenador);
		int index = list_get_index(colaBlocked, unEntrenador, (void*)comparadorDeEntrenadores);
		t_entrenador* unEntrenador = list_remove(colaBlocked,index);
		unEntrenador->blockeado = false;
		unEntrenador->ocupado = false;
		unEntrenador->contadorRR = 0;
		//unEntrenador->rafagasEjecutadas = 0;
		unEntrenador->pokemonAAtrapar = NULL;
		list_add(colaReady,unEntrenador);
		log_info(loggerObligatorio, "Se cambió un entrenador de BLOCKED a READY, Razon: Fallo al atrapar un pokemon");
	}
}

void completarCatchNormal(t_entrenador* unEntrenador){
	int index = list_get_index(colaBlocked, unEntrenador, (void*)comparadorDeEntrenadores);
	//SI LE QUEDAN POKEMONES POR ATRAPAR VA A READY
	if(unEntrenador->cantPokemonesPorAtrapar > 0){
		t_entrenador* unEntrenador = list_remove(colaBlocked,index);
		unEntrenador->blockeado = true;
		unEntrenador->ocupado = false;
		unEntrenador->llegoMensajeCaught = false;
		unEntrenador->pokemonAAtrapar = NULL;
		list_add(colaBlocked,unEntrenador);
		log_info(loggerObligatorio, "El entrenador %d terminó de atrapar un pokemon", unEntrenador->idEntrenador);
	}
	else{
		//SI NO LE QUEDAN POR ATRAPAR MAS VERIFICO SI CUMPLIÓ EL OBJETIVO O NO, SI CUMPLE MANDO A EXIT, SINO A BLOCKED
		if (cumplioObjetivo(unEntrenador)){
			t_entrenador* unEntrenador = list_remove(colaBlocked,index);
			//unEntrenador->blockeado = false;
			//unEntrenador->ocupado = false;
			unEntrenador->pokemonAAtrapar = NULL;
			unEntrenador->llegoMensajeCaught = false;
			list_add(colaExit, unEntrenador);
			log_info(loggerObligatorio, "Se cambió al entrenador %d de BLOCKED a EXIT, Razon: Termino de atrapar un pokemon y cumplio con su objetivo", unEntrenador->idEntrenador);
			}
		else{
			unEntrenador->pokemonAAtrapar = NULL;
			unEntrenador->blockeado = true;
			unEntrenador->ocupado = true;
			unEntrenador->llegoMensajeCaught = false;
			log_info(logger, "El entrenador %d atrapo el pokemon, no puede atrapar mas y no cumplio el objetivo, esperara al algoritmo de Deadlock", unEntrenador->idEntrenador);
			//LOS ENTRENADORES QUE NO COMPLETEN EL OBJETIVO Y PROBABLEMENTE ESTEN EN DEADLOCK QUEDAN BLOCKED Y EN ESTADO OCUPADO
			}
		//CUANDO SALGO DE EXEC HAGO EL POST
        /*
		if (strcmp(algoritmo_planificacion,"SJF-CD")!=0 && conexionAppeared){
			sem_post(&semaforoPlanificacionExec);
		}
		*/

	}
	//ESTO ME HABILITA LA DETECCION DE DL
	if(list_is_empty(pokemonesEnMapa) && !hayPokemonesParaAtrapar() && !planificacionCompleta){
		sem_post(&semaforoDeteccionDeadlocks);
	}

	//sem_post(&semaforoPlanificacionExec);
}

void completarCatchSinBroker(t_entrenador* unEntrenador){
	if(unEntrenador->cantPokemonesPorAtrapar > 0){
		t_entrenador* unEntrenador = list_remove(colaExec,0);
		unEntrenador->blockeado = true;
		unEntrenador->ocupado = false;
		unEntrenador->pokemonAAtrapar = NULL;
		list_add(colaBlocked,unEntrenador);
		log_info(loggerObligatorio, "Se cambió al entrenador %d de EXEC a BLOCKED, Razon: Termino de atrapar un pokemon con la rutina por default", unEntrenador->idEntrenador);
		//CUANDO SALGO DE EXEC HAGO EL POST

	}
	else{
		//SI NO LE QUEDAN POR ATRAPAR MAS VERIFICO SI CUMPLIÓ EL OBJETIVO O NO, SI CUMPLE MANDO A EXIT, SINO A BLOCKED
		if (cumplioObjetivo(unEntrenador)){
			t_entrenador* unEntrenador = list_remove(colaExec,0);
			unEntrenador->blockeado = false;
			unEntrenador->ocupado = false;
			unEntrenador->pokemonAAtrapar = NULL;
			list_add(colaExit, unEntrenador);
			log_info(loggerObligatorio, "Se cambió al entrenador %d de EXEC a EXIT, Razon: Termino de atrapar un pokemon y cumplio con su objetivo", unEntrenador->idEntrenador);
			//CUANDO SALGO DE EXEC HAGO EL POST
			//sem_post(&semaforoPlanificacionExec);
			}
		else{
			t_entrenador* unEntrenador = list_remove(colaExec,0);
			unEntrenador->pokemonAAtrapar = NULL;
			unEntrenador->ocupado = true;
			unEntrenador->blockeado = true;
			list_add(colaBlocked, unEntrenador);
			log_info(logger, "El entrenador %d atrapo el pokemon, no puede atrapar mas y no cumplio el objetivo se cambio de EXEC a BLOCKED donde esperara al algoritmo de Deadlock", unEntrenador->idEntrenador);
			}
	}
	//ESTO ME HABILITA LA DETECCION DE DL
	if(list_is_empty(pokemonesEnMapa) && !hayPokemonesParaAtrapar() && !planificacionCompleta){
		sem_post(&semaforoDeteccionDeadlocks);
	}
	//sem_post(&semaforoPlanificacionExec);
}

void completarIntercambioNormal(t_entrenador* unEntrenador){
	int index = list_get_index(colaBlocked, unEntrenador, (void*)comparadorDeEntrenadores);
	if (cumplioObjetivo(unEntrenador)){
		t_entrenador* unEntrenador = list_remove(colaBlocked,index);
		unEntrenador->blockeado = false;
		unEntrenador->ocupado = false;
		unEntrenador->pokemonAAtrapar = NULL;
		unEntrenador->pokemonQueNecesito = NULL;
		unEntrenador->llegoMensajeCaught = false;
		list_add(colaExit, unEntrenador);
		log_info(loggerObligatorio, "Se cambió al entrenador %d de BLOCKED a EXIT, Razon: Termino de atrapar un pokemon y cumplio con su objetivo", unEntrenador->idEntrenador);
	}
	else{
		unEntrenador->pokemonAAtrapar = NULL;
		unEntrenador->pokemonQueNecesito = NULL;
		unEntrenador->blockeado = true;
		unEntrenador->llegoMensajeCaught = false;
		unEntrenador->ocupado = true;
		log_info(logger, "El entrenador %d atrapo el pokemon, no puede atrapar mas y no cumplio el objetivo, esperara al algoritmo de Deadlock", unEntrenador->idEntrenador);
	}

	if(contadorCaught == 2 && comenzoDeteccionDeadlock){
		contadorCaught = 0;
		pthread_mutex_unlock(&mutexDeadlock);
	}
}

void completarIntercambioSinBroker(t_entrenador* unEntrenador){
	if (cumplioObjetivo(unEntrenador)){
		if(!list_is_empty(colaExec)){
		t_entrenador* unEntrenador = list_remove(colaExec,0);
		unEntrenador->blockeado = false;
		unEntrenador->ocupado = false;
		unEntrenador->pokemonAAtrapar = NULL;
		unEntrenador->pokemonQueNecesito = NULL;
		list_add(colaExit, unEntrenador);
		log_info(loggerObligatorio, "Se cambió al entrenador %d de EXEC a EXIT, Razon: Termino de atrapar un pokemon con la rutina por default y cumplio con su objetivo", unEntrenador->idEntrenador);
		//CUANDO SALGO DE EXEC HAGO EL POST
		//sem_post(&semaforoPlanificacionExec);
		//pthread_mutex_unlock(&mutexEntrenadoresReady);
		}
		else{
			int index = list_get_index(colaBlocked,unEntrenador,(void*)comparadorDeEntrenadores);
			list_remove(colaBlocked,index);
			list_add(colaExit,unEntrenador);
			log_info(logger, "Se cambió el entrenador %d de BLOCKED a EXIT, Razón: Termino de atrapar un pokemon con la rutina por default y cumplio con su objetivo", unEntrenador->idEntrenador);
		}
	}
	else{
		if(!list_is_empty(colaExec)){
			t_entrenador* unEntrenador = list_remove(colaExec,0);
			unEntrenador->pokemonAAtrapar = NULL;
			unEntrenador->blockeado = true;
			unEntrenador->ocupado = true;
			list_add(colaBlocked, unEntrenador);
			log_info(logger, "El entrenador %d atrapo el pokemon, no puede atrapar mas y no cumplio el objetivo, se cambio de EXEC a BLOCKED donde esperara al algoritmo de Deadlock", unEntrenador->idEntrenador);
			//sem_post(&semaforoPlanificacionExec);  //PORQUE ACA PASO DE EXEC A BLOQUEADO
			//pthread_mutex_unlock(&mutexEntrenadoresReady);
		}else{
			unEntrenador->pokemonAAtrapar = NULL;
			unEntrenador->blockeado = true;
			unEntrenador->ocupado = true;
			log_info(logger, "El entrenador %d atrapo el pokemon, no puede atrapar mas y no cumplio el objetivo, permanece en BLOCKED donde esperara al algoritmo de Deadlock", unEntrenador->idEntrenador);
		}
	}


	if(contadorCaught == 2){
		contadorCaught = 0;
		pthread_mutex_unlock(&mutexDeadlock);
	}

}

void mostrarContenidoLista(t_list* lista,void(*printer)(void *)){
	list_iterate(lista,printer);
}

void imprimirString(void *contenidoAMostrar){
	printf("ULTIMO ACCESO: %s \n",(char *) contenidoAMostrar);
}

bool cumplioObjetivo(t_entrenador* unEntrenador){

	t_list *nombresPokemonesEntrenador = unEntrenador->pokemones;
	t_list *nombresPokemonesObjetivos = unEntrenador->objetivo;

	return tieneLosMismosElementos(nombresPokemonesEntrenador,nombresPokemonesObjetivos,stringComparator);
}

void sacarPokemonDelMapa(t_pokemon* unPokemon){
	int index = list_get_index(pokemonesEnMapa, unPokemon, (void*)comparadorPokemones);
	//log_error(logger,"EL POKEMON QUE VOY A BORRAR %s SE ENCUENTRA EN LA POS: %d",unPokemon->nombrePokemon,index);
	//pthread_mutex_lock(&mutexPokemonesEnMapa);
	list_remove(pokemonesEnMapa, index);

	//mostrarContenidoLista(list_map(pokemonesEnMapa,(void *)obtenerPokemon),imprimirString);

}

bool comparadorPokemones(t_pokemon* unPokemon, t_pokemon* otroPokemon){

	bool resultadoComparacionNombre = string_equals_ignore_case(unPokemon->nombrePokemon,otroPokemon->nombrePokemon);

	if(resultadoComparacionNombre){
		return unPokemon->coordenadaX == otroPokemon->coordenadaX && unPokemon->coordenadaY == otroPokemon->coordenadaY;
	}else{
		return false;
	}
}

void aplicarFIFO(){
	log_info(logger,"Se aplicara el algoritmo de planificacion FIFO");
	if(list_is_empty(colaExec) && (!list_is_empty(colaReady))){
		log_info(logger,"IDS ENTRENADORES EN READY ANTES DE SACAR UN ENTRENADOR");
		t_list *losReady = list_map(colaReady,(void *)obtenerIDEntrenador);
		mostrarContenidoLista(losReady,imprimirNumero);
		t_entrenador* entrenadorAEjecutar = (t_entrenador*) list_remove(colaReady, 0);
		list_add(colaExec, entrenadorAEjecutar);
		log_info(loggerObligatorio, "Se cambió un entrenador %d de READY a EXEC, Razon: Elegido del algoritmo de planificacion", entrenadorAEjecutar->idEntrenador);
		cambiosDeContexto++;
		list_clean(losReady);
		list_destroy(losReady);
	}
}

void aplicarRR(){
	log_info(logger,"Se aplicara el algoritmo de planificacion RR, con quantum de %d", quantum);
	if(list_is_empty(colaExec) && (!list_is_empty(colaReady))){
		t_entrenador* entrenadorAEjecutar = (t_entrenador*) list_remove(colaReady, 0);
		list_add(colaExec, entrenadorAEjecutar);
		log_info(loggerObligatorio, "Se cambió un entrenador %d de READY a EXEC, Razon: Elegido del algoritmo de planificacion", entrenadorAEjecutar->idEntrenador);
		//cambiosDeContexto++;
	}
}

void aplicarSJFConDesalojo(){
	if(!list_is_empty(colaExec)){

		t_entrenador* entrenadorEnExec = list_get(colaExec,0);
		calcularEstimacion(entrenadorEnExec);
		log_info(logger, "LA ESTIMACION DEL ENTRENADOR %d EJECUTANDO ES: %f",entrenadorEnExec->idEntrenador, entrenadorEnExec->rafagasEstimadas);
		t_list* aux = list_map(colaReady, (void*)calcularEstimacion);
		list_sort(aux, (void*)comparadorDeRafagas);
		t_entrenador* entrenadorEnReady = (t_entrenador*) list_remove(aux,0);
		log_info(logger, "LA MENOR ESTIMACION DE READY ES DEL ENTRENADOR %d, QUE ES: %f", entrenadorEnReady->idEntrenador, entrenadorEnReady->rafagasEstimadas);

		if(entrenadorEnReady->rafagasEstimadas < entrenadorEnExec->rafagasEstimadas){
			//SACO AL QUE EJECUTA
			hayQueDesalojar = true;
			list_remove(colaExec,0);
			list_add(colaReady, entrenadorEnExec);
			log_info(logger,"ESTOY ACA PORQUE TENGO QUE DESALOJAR");
			log_info(loggerObligatorio, "Se cambió un entrenador %d de EXEC a READY, Razon: Elegido del algoritmo de planificacion", entrenadorEnExec->idEntrenador);
			cambiosDeContexto++;
			//METO AL QUE QUIERE EJECUTAR
			int index = list_get_index(colaReady,entrenadorEnReady,(void*)comparadorDeEntrenadores);
			t_entrenador* entrenadorAEjecutar = list_remove(colaReady,index);
			list_add(colaExec, entrenadorAEjecutar);
			//CUANDO LO VOY A EJECUTAR LE LEVANTO EL SEMAFORO
			log_error(logger,"PRINTEO SJF CON DESALOJO");
			log_info(loggerObligatorio, "Se cambió un entrenador %d de READY a EXEC, Razon: Elegido del algoritmo de planificacion", entrenadorAEjecutar->idEntrenador);
			//cambiosDeContexto++;
			hayQueDesalojar = false;
		}
	}
	else{
		hayQueDesalojar = false;
		aplicarSJF();
		//sem_post(&semaforoPlanificacionExec);

	}
}

void aplicarSJF(){

	if(list_is_empty(colaExec) && (!list_is_empty(colaReady))){

		float *obtenerRafaga(t_entrenador *entrenador){
			return &(entrenador->rafagasEstimadas);
		}

		void imprimirFloat(void *numero){
			printf("RAFAGA: %f \n",*(float*)numero);
		}


		t_list* aux = list_map(colaReady, (void*)calcularEstimacion);

		list_sort(aux, (void*)comparadorDeRafagas);
		log_info(logger,"IDS ENTRENADORES EN READY ORDENADOS");
		mostrarContenidoLista(list_map(aux,(void *)obtenerIDEntrenador),imprimirNumero);
		log_info(logger,"SUS RAFAGAS SON");
		mostrarContenidoLista(list_map(aux,(void *) obtenerRafaga),imprimirFloat);

		t_entrenador* entrenadorAux = (t_entrenador*) list_remove(aux,0);
		int index = list_get_index(colaReady,entrenadorAux,(void*)comparadorDeEntrenadores);
		t_entrenador* entrenadorAEjecutar = list_remove(colaReady,index);
		list_add(colaExec, entrenadorAEjecutar);
		log_info(loggerObligatorio, "Se cambió un entrenador %d de READY a EXEC, Razon: Elegido del algoritmo de planificacion", entrenadorAEjecutar->idEntrenador);
		cambiosDeContexto++;
	}
}

t_entrenador* calcularEstimacion(t_entrenador* unEntrenador){

	if(!unEntrenador->seCalculoEstimacion){
		unEntrenador->seCalculoEstimacion = true;
	if(unEntrenador->rafagasEjecutadas == 0){
		unEntrenador->rafagasEstimadas = estimacion_inicial;
	}else{

	unEntrenador->rafagasEstimadas = (alpha * unEntrenador->rafagasEstimadas)
			+ ((1- alpha) * (unEntrenador->rafagasEjecutadas));
	}
	}

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

t_list *obtenerEntrenadoresDisponibles(){
	t_list* entrenadoresLibres = list_create();

	if(!list_is_empty(colaNew)){
		for(int i = 0; i < list_size(colaNew); i++){
			t_entrenador* unEntrenador = list_get(colaNew, i);
			list_add(entrenadoresLibres,unEntrenador); //esto esta igual que en SJF, deberia estar bien
		}
	}

	bool estaLibre(t_entrenador* unEntrenador){
		return !estaOcupado(unEntrenador);
	}

	if(!list_is_empty(colaBlocked)){
		t_list *entrenadoresBloqueadosDisponibles = list_filter(colaBlocked, (void*)estaLibre);
		log_error(logger,"ENTRENADORES BLOQUEADOS");
		mostrarContenidoLista(list_map(entrenadoresBloqueadosDisponibles,(void *) obtenerIDEntrenador),imprimirNumero);
		list_add_all(entrenadoresLibres,entrenadoresBloqueadosDisponibles);
		list_clean(entrenadoresBloqueadosDisponibles);
		list_destroy(entrenadoresBloqueadosDisponibles);
	}

	return entrenadoresLibres;
}

void imprimirNumero(void *contenidoAMostrar){
    printf("ID: %d \n",*(uint32_t *) contenidoAMostrar);
}

void transicionAReady(t_entrenador *entrenador,t_FLAG estado){
	if(estado == t_NEW_STATE){
		transicionDeNewAReady(entrenador);
	}

	if(estado == t_BLOCKED_STATE){
		transicionDeBlockedAReady(entrenador);
	}
}


t_entrenador *entrenadorQueVaAReady(t_list *entrenadoresLibres,t_FLAG flag){
	t_entrenador *entrenador;

	if(flag == t_CATCH_STATE){
		entrenador = entrenadorQueVaAReadyNormal(entrenadoresLibres);
	}

	if(flag == t_EXCHANGE_STATE){
		entrenador = entrenadorQueVaAReadyConDeadlock(entrenadoresLibres);
	}

	return entrenador;
}


void calcularDistanciaAEntrenadores(t_list *entrenadoresLibres){

	for(int i = 0; i < list_size(entrenadoresLibres);i++){
		t_entrenador *entrenador = list_get(entrenadoresLibres,i);

		char *pokemon = list_get(entrenador->pokemonesQueFaltan,0);

		t_entrenador *entrenadorQueTieneMiPokemon = obtenerEntrenadorQueTieneMiPokemon(entrenador,pokemon);

		int distanciaAEntrenador = (abs(entrenador->coordenadaX - entrenadorQueTieneMiPokemon->coordenadaX) + abs(entrenador->coordenadaY - entrenadorQueTieneMiPokemon->coordenadaY));

		entrenador->distanciaAEntrenador = distanciaAEntrenador;

		entrenador->entrenadorAIntercambiar = entrenadorQueTieneMiPokemon;

		entrenador->pokemonQueNecesito = pokemon;
	}

}

t_entrenador *entrenadorQueVaAReadyConDeadlock(t_list *entrenadoresLibres){

	bool comparadorDeDistancia(t_entrenador* unEntrenador, t_entrenador* otroEntrenador){
		return unEntrenador->distanciaAEntrenador <= otroEntrenador->distanciaAEntrenador;
	}

	calcularDistanciaAEntrenadores(entrenadoresLibres);

	list_sort(entrenadoresLibres,(void *)comparadorDeDistancia);

	t_entrenador *entrenador = list_remove(entrenadoresLibres,0);

	return entrenador;
}

t_entrenador *entrenadorQueVaAReadyNormal(t_list *entrenadoresLibres){
	t_entrenador *entrenadorAux;

	t_list* pokemones = list_duplicate(pokemonesEnMapa);
	t_pokemon* unPokemon = list_remove(pokemones,list_size(pokemonesEnMapa)-1);

	//CALCULO LA DISTANCIA DE TODOS A ESE POKEMON
	calcularDistanciaA(entrenadoresLibres,unPokemon);

	//ORDENO LA LISTA EN BASE A ESA DISTANCIA DE MENOR A MAYOR
	list_sort(entrenadoresLibres, (void*)comparadorDeDistancia);

	int *obtenerDistanciaEntrenador(t_entrenador *entrenador){
		return &(entrenador->distancia);
	}

	//SACO EL PRIMERO DE ESA LISTA ORDENADA
	entrenadorAux = list_remove(entrenadoresLibres, 0);

	entrenadorAux->pokemonAAtrapar = unPokemon;
	sem_post(&semaforoAgregadoPokemones);
	calcularDistanciaEntrenadorA(entrenadorAux,unPokemon);

	list_clean(pokemones);
	list_destroy(pokemones);

	return entrenadorAux;
}

void transicionDeNewAReady(t_entrenador *entrenador){
	int index = list_get_index(colaNew,entrenador,(void*)comparadorDeEntrenadores);
	t_entrenador* entrenadorElegido = (t_entrenador*) list_remove(colaNew, index);
	list_add(colaReady,entrenadorElegido);//esto esta igual que en SJF, deberia estar bien
	log_info(loggerObligatorio, "Se pasó al entrenador %d de NEW a READY, Razon: Elegido por el planificador de entrada a ready", entrenadorElegido->idEntrenador);

	if(string_equals_ignore_case(algoritmo_planificacion,"SJF-CD")){
		if(!list_is_empty(colaExec)){
			aplicarSJFConDesalojo();
		}
	}

	sem_post(&semaforoPlanificacionExec);

}

void transicionDeBlockedAReady(t_entrenador *entrenador){
	int index = list_get_index(colaBlocked,entrenador,(void*)comparadorDeEntrenadores);
	t_entrenador* entrenadorElegido = (t_entrenador*) list_remove(colaBlocked, index);
	entrenadorElegido->blockeado = false;
	list_add(colaReady,entrenadorElegido); //esto esta igual que en SJF, deberia estar bien
	log_info(loggerObligatorio, "Se pasó al entrenador %d de BLOCKED a READY, Razon: Elegido por el planificador de entrada a ready", entrenadorElegido->idEntrenador);


	if(string_equals_ignore_case(algoritmo_planificacion,"SJF-CD")){
		if(!list_is_empty(colaExec)){
			aplicarSJFConDesalojo();
		}
	}

	sem_post(&semaforoPlanificacionExec);
}

void planificadorALargoPlazo(){
	//ESTO PLANIFICA DE NEW A READY Y DE BLOCKED A READY
	log_info(logger, "Se empezará la planificacion a Ready de entrendores");

	//Este semáforo se utiliza para disminuir la espera activa
	sem_wait(&semaforoPlanificacionInicialReady);
	while(1){
		t_entrenador *entrenador;
		t_list *entrenadoresLibres;

		pthread_mutex_lock(&mutexEntrenadoresReady);

		//Este if permite, cuando hay conexión con el Broker, hacer un signal al planificador de corto plazo
		//para que se mate
		if(list_size(entrenadores) == list_size(colaExit)){
			sem_post(&semaforoPlanificacionExec);
			break;
		}

		entrenadoresLibres = obtenerEntrenadoresDisponibles();

		if(list_is_empty(entrenadoresLibres)){
			sem_post(&semaforoAgregadoPokemones);
		}else{

			if(tipoPlanificacion == t_CATCH_STATE){
				entrenador = entrenadorQueVaAReady(entrenadoresLibres,t_CATCH_STATE);
			}else{
				entrenador = entrenadorQueVaAReady(entrenadoresLibres,t_EXCHANGE_STATE);
			}

			if(entrenador->blockeado){
				transicionAReady(entrenador,t_BLOCKED_STATE);
			}else{
				transicionAReady(entrenador,t_NEW_STATE);
			}

			list_clean(entrenadoresLibres);
			list_destroy(entrenadoresLibres);

		}
	}

	pthread_exit(NULL);

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

 /* FUNCION PRINCIPAL DE DEADLOCKS */

void revisionDeadlocks(){
	sem_wait(&semaforoDeteccionDeadlocks);
	//ESTE SEM BLOQUEA HASTA QUE SE DE LA CONDICION DONDE PUEDO COMENZAR A CORRER EL ALGORITMO DE DL
	while (1){
		if(list_size(pokemonesAtrapados) == list_size(objetivoTeam)){

				if(cumplioObjetivoTeam()){
					log_info(loggerObligatorio, "El %s cumplio el objetivo, se pasara a imprimir sus metricas",identificadorProceso);
					break;
				}
				else{
				//SINO, ME PONGO A BUSCAR DEADLOCKS
				planificacionCompleta = true;
			    tipoPlanificacion = t_EXCHANGE_STATE;
				log_info(loggerObligatorio, "El %s NO cumplio el objetivo, se pasara a detectar deadlocks",identificadorProceso);

				//A LOS ENTRENADORES QUE NO CUMPLIERON LOS OBJETIVOS, LES GENERO UNA LISTA DE POKEMONES QUE LES FALTAN
				generarPokemonesFaltantes();

				//ME PONGO A BUSCAR DEADLOCKS
				comenzoDeteccionDeadlock = 1;
				detectarDeadlocks();
				break;
				}
		}
	}

	sem_post(&semaforoMetricas);
	pthread_exit(NULL);

}

bool cumplioObjetivoTeam(){
	bool resultado = false;
	int cantEntrenadores = list_size(entrenadores);
	int i;
	for(i=0; i<cantEntrenadores; i++){
		t_entrenador* unEntrenador = list_get(entrenadores,i);
		if(!cumplioObjetivo(unEntrenador)){
			return resultado;
		}
	}
	return !resultado;
}

bool noCumplioObjetivo(t_entrenador* unEntrenador){
	return !cumplioObjetivo(unEntrenador);
}

bool tienePokemon(char *pokemon,t_entrenador *entrenador){
	return existeID(pokemon,entrenador->pokemones,stringComparator);
}


bool existeID(void *ID_a_buscar, t_list *lista,bool(*comparator)(void *,void *)){
	bool resultado = false;

	for (int i = 0; i < list_size(lista);i++){
		void *ID_a_comparar = list_get(lista,i);

		if(comparator(ID_a_buscar,ID_a_comparar)){
			resultado = true;
			break;
		}
	}

	return resultado;
}


t_entrenador *obtenerEntrenadorQueTieneElPokemon(char *pokemon,t_list *entrenadoresEnDeadlock,t_entrenador *entrenador){
	t_list *aux = list_duplicate(entrenadoresEnDeadlock);
	int index = list_get_index(aux,entrenador,(void *)comparadorDeEntrenadores);
	list_remove(aux,index);

	for(int i = 0; i < list_size(aux);i++){
		t_entrenador *entrenador = list_get(aux,i);

		if(tienePokemon(pokemon,entrenador)){
			return entrenador;
		}
	}

	return NULL;
}

int *obtenerIDEntrenador(t_entrenador *entrenador){
	return &(entrenador->idEntrenador);
}

bool intComparator(void *unNumero,void *otroNumero){
	uint32_t *valor = unNumero;
	uint32_t *otroValor = otroNumero;

	return *valor == *otroValor;
}

bool estaEnBlocked(t_entrenador *entrenador){
	t_list *idsEntrenadoresEnBlocked = list_map(colaBlocked,(void *) obtenerIDEntrenador);

	return existeID(&(entrenador->idEntrenador),idsEntrenadoresEnBlocked,intComparator);

}


void cambiarEstadoEntrenadores(t_list *entrenadores,int operacion){
	for(int i = 0; i < list_size(entrenadores);i++){
		t_entrenador *entrenador = list_get(entrenadores,i);
		entrenador->operacionEntrenador = operacion;
		entrenador->ocupado = false;
	}
}


void modificarContadorDeadlocks(int cantDeadlocks){
	deadlocksProducidos+=cantDeadlocks;
}

void quienesEstanEnDeadlock(t_list *entrenadoresSinObjetivo,t_list*entrenadoresDL,int posDeLista,int cantidadDeIteraciones){


	printf("\n\n\n -----------------EMPIEZO DL \n\n\n ");
	if(list_size(entrenadoresSinObjetivo) <= 1) return;
	if(cantidadDeIteraciones >= list_size(entrenadoresSinObjetivo)) return;

	//Obtenemos al primer entrenador y su primer pokemon que le falta

	t_entrenador *entrenador = list_get(entrenadoresSinObjetivo,posDeLista);
	char *pokemon = list_get(entrenador->pokemonesQueFaltan,0);

	// Obtenemos al entrenador2 que posee el pokemon que le falta al entrenador1
	t_entrenador *entrenadorQueTieneMiPokemon = obtenerEntrenadorQueTieneMiPokemon(entrenador,pokemon);

	printf("Busco al que tiene mi pokemon\n");


	//Pregunto si hay elementos en la lista entrenadores en deadlock
	if(!list_is_empty(entrenadoresDL)){
		// Obtengo al primer entrenador de la lista de DL
		t_entrenador *primerIDdeb = list_get(entrenadoresDL,0);
		// Consulto si al que intento agregar coincide con el primero en la lista deadlock, si es así hay un ciclo
			if(entrenador->idEntrenador == primerIDdeb->idEntrenador){
				printf("Hay una espera circular\n");
				return;
			}

			// SI la lista de DL es mayor que la de entrenadores sin objetivo y el entrenador que quiero agregar no coincide con el primero de la lista
			// por lo tanto no hay ciclo y reinicio la lista y paso al siguiente de la lista entrenadoresSinObjetivo utilizando la cantidad de iteraciones

			if(list_size(entrenadoresDL) > list_size(entrenadoresSinObjetivo) && entrenador->idEntrenador != primerIDdeb->idEntrenador){
				printf("No se complio la espera circular, se procede a ver el próximo de la lista\n");
				list_clean(entrenadoresDL);
				cantidadDeIteraciones++;
				return quienesEstanEnDeadlock(entrenadoresSinObjetivo,entrenadoresDL,cantidadDeIteraciones,cantidadDeIteraciones);
			}
	}

	if(entrenadorQueTieneMiPokemon != NULL){
		log_info(logger,"AL ENTRENADOR %d LE FALTA AL POKEMON %s QUE LO TIENE %d",entrenador->idEntrenador,pokemon,entrenadorQueTieneMiPokemon->idEntrenador);
		list_add(entrenadoresDL,&(entrenador->idEntrenador));

		posDeLista = list_get_index(entrenadoresSinObjetivo,entrenadorQueTieneMiPokemon,(void *)comparadorDeEntrenadores);
		log_info(logger,"Paso la posicion del entrenador que tiene mi pokemon %d",posDeLista);
	}else{
		posDeLista++;
		cantidadDeIteraciones++;
		list_clean(entrenadoresDL);
		log_info(logger,"No hay entrenador que posea mi pokemon, paso al siguiente de la lista: %d",posDeLista);
	}

	return quienesEstanEnDeadlock(entrenadoresSinObjetivo,entrenadoresDL,posDeLista,cantidadDeIteraciones);

}

void detectarDeadlocks(){

	while(1){
		pthread_mutex_lock(&mutexDeadlock);
		if(!cumplioObjetivoTeam()){

		t_list *entrenadoresQueNoCumplieronObjetivo = list_filter(entrenadores, (void*)noCumplioObjetivo);

		t_list *entrenadoresEnDeadlockBis = list_create();

		quienesEstanEnDeadlock(entrenadoresQueNoCumplieronObjetivo,entrenadoresEnDeadlockBis,0,0);

		//mostrar entrenadores que no cumplieron y los que están en deadock t0do

		log_error(logger,"ENTRENADORES QUE NO CUMPLIERON EL OBJETIVO!");
		mostrarContenidoLista(list_map(entrenadoresQueNoCumplieronObjetivo,(void *)obtenerIDEntrenador),imprimirNumero);

		log_error(logger,"ENTRENADORES EN DLs");
		mostrarContenidoLista(entrenadoresEnDeadlockBis,imprimirNumero);

		cambiarEstadoEntrenadores(entrenadoresQueNoCumplieronObjetivo,t_intercambiarPokemon);

		//t_entrenador *entrenador = list_remove(colaBlocked,0);
		//TODO fijate de mandar primero al que tenga el entrenador mas cerca asi se porta mas como el plani a ready
		deadlocksProducidos++;


		pthread_mutex_unlock(&mutexEntrenadoresReady);
		//entrenador->blockeado = false;
		//entrenador->ocupado = false;

		//list_add(colaReady,entrenador);
		//log_info(loggerObligatorio, "Se cambio un entrenador %d de BLOCKED a READY, Razon: Seleccionado por el algoritmo de deteccion de deadlocks para que lo resuelva",entrenador->idEntrenador);
		deadlocksResueltos++;

		list_clean(entrenadoresQueNoCumplieronObjetivo);
		list_destroy(entrenadoresQueNoCumplieronObjetivo);
		list_clean(entrenadoresEnDeadlockBis);
		list_destroy(entrenadoresEnDeadlockBis);

		}else{
			break;
		}

	}
}

t_entrenador* obtenerEntrenadorQueTieneMiPokemon(t_entrenador* unEntrenador, char* unPokemon){
	//ME INFORMA QUE ENTRENADOR TIENE EL POKEMON QUE NECESITO
	t_list* aux = list_filter(entrenadores, (void*)noCumplioObjetivo);
	log_error(loggerObligatorio,"List size: %d",list_size(aux));
	int indexEntrenador = list_get_index(aux, unEntrenador, (void*)comparadorDeEntrenadores);
	log_error(loggerObligatorio,"IndexEntrenador: %d",indexEntrenador);

	list_remove(aux,indexEntrenador);
	int cantEntrenadores = list_size(aux);
	int i;
	for(i = 0; i<cantEntrenadores; i++){
		t_entrenador* entrenadorEnDeadlock = list_get(aux, i);
		int j;
		int cantPokemones = list_size(entrenadorEnDeadlock->pokemones);
		for(j=0; j<cantPokemones; j++){
			char *unPokemonDeOtroEntrenador = list_get(entrenadorEnDeadlock->pokemones,j);
			if(strcmp(unPokemon,unPokemonDeOtroEntrenador)==0){

				list_clean(aux);
				list_destroy(aux);

				return entrenadorEnDeadlock;
			}
		}
	}

	list_clean(aux);
	list_destroy(aux);
	return NULL;
}


void generarPokemonesFaltantes(){
	t_list* entrenadoresEnDeadlock = list_filter(entrenadores, (void*)noCumplioObjetivo);
		int cantEntrenadoresEnDeadlock = list_size(entrenadoresEnDeadlock);
		int i;
		for(i = 0; i<cantEntrenadoresEnDeadlock; i++){
			t_entrenador* entrenadorEnDeadlock = list_get(entrenadoresEnDeadlock, i);
			int index = list_get_index(entrenadores, entrenadorEnDeadlock, (void*)comparadorDeEntrenadores);
			//MODIFICO EL ENTRENADOR QUE ESTA EN LA LISTA DE ENTRENADORES ORIGINAL
			entrenadorEnDeadlock = list_get(entrenadores,index);
			agregarPokemonesQueFaltan(entrenadorEnDeadlock);
		}

	list_clean(entrenadoresEnDeadlock);
	list_destroy(entrenadoresEnDeadlock);
}

void agregarPokemonesQueFaltan(t_entrenador* unEntrenador){
	int cantPokemones = list_size(unEntrenador->pokemones);
	int i;
	for (i=0; i<cantPokemones; i++){
		char *unPokemonObjetivo = list_get(unEntrenador->objetivo, i);
		bool estaAtrapado = false;
		int j;
		for (j=0; j<cantPokemones; j++){
			char* unPokemonAtrapado = list_get(unEntrenador->pokemones, i);
			if(strcmp(unPokemonObjetivo,unPokemonAtrapado)==0){
				estaAtrapado = true;
			}
		}
		if (!estaAtrapado){
			list_add(unEntrenador->pokemonesQueFaltan, unPokemonObjetivo);
		}
	}
}

void masacreDeHilos(){
	pthread_cancel(hiloSuscripcionBroker);
	pthread_cancel(hiloAtencionAppeared);
	pthread_cancel(hiloAtencionCaught);
	pthread_cancel(hiloAtencionLocalized);
	pthread_cancel(hiloAtencionGameBoy);
	pthread_cancel(hiloMensajes);
	//pthread_cancel(hiloPlanificacionReady);
	//pthread_cancel(hiloPlanificacionEntrenadores);
	pthread_cancel(hiloRevisionDeadlocks);
}

void imprimirMetricas(){

	sem_wait(&semaforoMetricas);
	while(1){
		if(list_size(entrenadores) == list_size(colaExit)){
			masacreDeHilos();
			metricas();

			//Este unlock se utiliza para matar al hilo de planificación.
			pthread_mutex_unlock(&mutexEntrenadoresReady);
			break;
		}
	}

	pthread_exit(NULL);
}

void metricas(){
	log_info(loggerObligatorio, "El team ha finalizado su ejecucion, se pasara a imprimir las metricas");
	log_info(loggerObligatorio, "1. Cantidad de ciclos de CPU totales ejecutados: %d", ciclosTotales);
	log_info(loggerObligatorio, "2. Cantidad de cambios de contexto realizados: %d", cambiosDeContexto);
	log_info(loggerObligatorio, "3. Cantidad de ciclos de CPU ejecutados por entrenador:");
	int i;
	int cantEntrenadores = list_size(entrenadores);
	for(i=0; i<cantEntrenadores; i++){
		t_entrenador* entrenador = list_get(entrenadores,i);
		log_info(loggerObligatorio, "Entrenador %d: %d Ciclos",entrenador->idEntrenador, entrenador->ciclosEjecutados);
	}
	log_info(loggerObligatorio, "4. Cantidad de deadlocks producidos: %d", deadlocksProducidos);
	log_info(loggerObligatorio, "5. Cantidad de deadlocks resueltos: %d", deadlocksResueltos);
}

