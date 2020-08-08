#include "GameCard.h"

int main(int argc,char* argv[]){
	setearValoresDefaultfs(argc,argv);

	sem_init(&semDeBloques,0,1);
	sem_init(&aperturaDeArchivo,0,1);
	sem_init(&reconexion,0,1);
	sem_init(&conexionRecuperadaDeNew,0,0);
	sem_init(&conexionRecuperadaDeCatch,0,0);
	sem_init(&conexionRecuperadaDeGet,0,0);


	leerArchivoDeConfiguracion();

	crearLoggerObligatorio();

	socket_servidor = iniciar_servidor(ip_gc,puerto_gc,loggerObligatorio);
	//Servidor iniciado
	log_info(loggerObligatorio,"Server ready for action!");

	iniciar_punto_de_montaje(path_de_tallgrass);
	//mostrarBitmap(); // ---------- SACAR

    pthread_t hiloMensajes;

    pthread_create(&hiloMensajes,NULL,(void *)iniciarGestionMensajesGC,NULL);

    pthread_join(hiloMensajes,NULL);


    sem_destroy(&conexionRecuperadaDeNew);
    sem_destroy(&conexionRecuperadaDeCatch);
    sem_destroy(&conexionRecuperadaDeGet);
    sem_destroy(&semDeBloques);
    sem_destroy(&aperturaDeArchivo);
    sem_destroy(&reconexion);
	return 0;
}

void setearValoresDefaultfs(int cantidadDeArgumentos,char* argumentos[]){
	if(cantidadDeArgumentos == 3){
		// Primero nos van a enviar la cantidad de bloques y luego el tamaño
		blocksdefault = argumentos[1];
		blocksizedefault = argumentos[2];
	}
}

t_infoPack *crearInfoDePaquete(int socket,void *paquete){
	t_infoPack *mensaje = malloc(sizeof(t_infoPack));
	mensaje->socket = socket;
	mensaje->paquete = paquete;
	return mensaje;
}

void destruirInfoPaquete(t_infoPack *infoPaquete){
	void *paquete = infoPaquete->paquete;
	free(paquete);
	free(infoPaquete);
}


void iniciarGestionMensajesGC(){
	pthread_create(&hiloSuscripcionABroker,NULL,(void *)administrarSuscripcionesBroker,NULL);
    pthread_create(&hiloAtencionNew,NULL,(void *)gestionMensajesNew,NULL);
    pthread_create(&hiloAtencionCatch,NULL,(void *)gestionMensajesCatch,NULL);
    pthread_create(&hiloAtencionGet,NULL,(void *)gestionMensajesGet,NULL);
	pthread_create(&hiloAtencionGB,NULL,(void*)gestionMensajesGB,&socket_servidor);

	pthread_join(hiloAtencionGB,NULL);
	pthread_join(hiloSuscripcionABroker,NULL);
	pthread_join(hiloAtencionNew,NULL);
	pthread_join(hiloAtencionCatch,NULL);
	pthread_join(hiloAtencionGet,NULL);
}

void suscripcionColaNew(){
	conexionAColaNew();
	if(conexionNew) sem_wait(&reconexion);
}

void suscripcionColaCatch(){

	while(1){
		if(conexionNew && identificadorProcesoGC != NULL){;
			conexionAColaCatch();

			break;
		}
	}
}

void suscripcionColaGet(){
	while(1){
		if(conexionNew && conexionCatch && identificadorProcesoGC){
			conexionAColaGet();
			break;
		}
	}
}

void gestionMensajesGet(){
	pthread_t hiloGet;

	while(1){
		if(conexionGet){
			Header headerRecibido;
			headerRecibido = receiveHeader(socket_get);
			if(headerRecibido.operacion != -1 && headerRecibido.tamanioMensaje != 0){
				uint32_t tamanio = headerRecibido.tamanioMensaje;
				switch(headerRecibido.operacion){
				case t_HANDSHAKE:
					log_info(loggerObligatorio,"Llego un HANDSHAKE del BROKER a través del socket Get!");
					recibirHandshake(socket_get,tamanio);
					break;

				case t_GET:
					log_info(loggerObligatorio,"Llego un mensaje GET del BROKER");
					void *paqueteGet = receiveAndUnpack(socket_get,tamanio);
					t_infoPack *infoGet = crearInfoDePaquete(socket_get,paqueteGet);

					pthread_create(&hiloGet,NULL,(void *)procedimientoMensajeGet,infoGet);
					pthread_detach(hiloGet);
					break;

				default:
					log_error(loggerObligatorio,"No es un codigo de operacion conocido: %i",headerRecibido.operacion);
					break;
				}
			}

		}else {
			sem_wait(&conexionRecuperadaDeGet);}
	}
}

void gestionMensajesCatch(){
	pthread_t hiloCatch;

	while(1){
		if(conexionCatch){
			Header headerRecibido;
			headerRecibido = receiveHeader(socket_catch);
			if(headerRecibido.operacion != -1 && headerRecibido.tamanioMensaje != 0){
				uint32_t tamanio = headerRecibido.tamanioMensaje;
				switch(headerRecibido.operacion){
				case t_HANDSHAKE:
					log_info(loggerObligatorio,"Llego un HANDSHAKE del BROKER a través del socket Catch!");
					recibirHandshake(socket_catch,tamanio);
					break;

				case t_CATCH:
					log_info(loggerObligatorio,"Llego un mensaje CATCH del BROKER");
					void *paqueteCatch = receiveAndUnpack(socket_catch,tamanio);
					t_infoPack *infoCatch = crearInfoDePaquete(socket_catch,paqueteCatch);
					pthread_create(&hiloCatch,NULL,(void *)procedimientoMensajeCatch,infoCatch);
					pthread_detach(hiloCatch);
					break;

				default:
					log_error(loggerObligatorio,"No es un codigo de operacion conocido: %i",headerRecibido.operacion);
					break;
				}
			}

		}else{
			sem_wait(&conexionRecuperadaDeCatch);
			}
	}
}

void gestionMensajesNew(){
	pthread_t hiloNew;

	while(1){
			if(conexionNew){
				Header headerRecibido;
			    headerRecibido = receiveHeader(socket_new);
			    if(headerRecibido.operacion != -1 && headerRecibido.tamanioMensaje != 0){
			    uint32_t tamanio = headerRecibido.tamanioMensaje;
			    switch(headerRecibido.operacion){
			    case t_HANDSHAKE:
			                log_info(loggerObligatorio, "Llego un HANDSHAKE del BROKER a través del socket NEW!");
			    			recibirHandshake(socket_new,tamanio);
			    			break;

			    case t_NEW:
		    	  log_info(loggerObligatorio, "Llego un mensaje NEW del BROKER!");
		    	  void* paqueteNew = receiveAndUnpack(socket_new, tamanio);
		    	  t_infoPack *infoNew = crearInfoDePaquete(socket_new,paqueteNew);
		    	  pthread_create(&hiloNew,NULL,(void *)procedimientoMensajeNew,infoNew);
		    	  pthread_detach(hiloNew);

		    	  break;

			    default:
			        log_error(loggerObligatorio,"No es un codigo de operacion conocido: %i",headerRecibido.operacion);
			    	break;
			    }
			    }
			    else{
			    	conexionNew= 0;
			    	sem_post(&reconexion);
			    	conexionCatch = 0;
			    	conexionGet = 0;
			    }
			}else{
		    	sem_wait(&conexionRecuperadaDeNew);
			}
		}
}

void recibirHandshake(int socket,uint32_t tamanioPaquete){
	void *paqueteBroker = receiveAndUnpack(socket,tamanioPaquete);

	if(identificadorProcesoGC == NULL){
		identificadorProcesoGC = unpackProceso(paqueteBroker);
		config_set_value(archivoConfig,"ID",identificadorProcesoGC);
		config_save(archivoConfig);
		free(paqueteBroker);
		log_info(loggerObligatorio,"ID RECIBIDO: %s",identificadorProcesoGC);
	}else{
		free(paqueteBroker);
	}
}

void administrarSuscripcionesBroker(){

	suscripcionColaNew();
	if(conexionNew){
	suscripcionColaCatch();
	suscripcionColaGet();
	}

	while(1){
		sem_wait(&reconexion);
		reconexionColaNew();
		suscripcionColaCatch();
		suscripcionColaGet();

	}
}

void procedimientoMensajeNew(t_infoPack *infoNew){
	void *paqueteNew = infoNew->paquete;
	int socket = infoNew->socket;

	char* pokemon = unpackPokemonNew(paqueteNew);
	int tamanioPokemon = strlen(pokemon);


	uint32_t id = unpackID(paqueteNew);
	uint32_t posX = unpackCoordenadaX_New(paqueteNew, tamanioPokemon);
	uint32_t posY = unpackCoordenadaY_New(paqueteNew, tamanioPokemon);
	uint32_t cantPokemon = unpackCantidadPokemons_New(paqueteNew, tamanioPokemon);

		if(esSocketDeBroker(socket)){
	    envioDeACK(id, t_NEW);
		}

	int resultadoNew = procedimientoNEW(pokemon,posX,posY,cantPokemon);
	//log_info(loggerObligatorio,"Pokemon: %s posX: %d posY: %d cantidad: %d id: %d",pokemon,posX,posY,cantPokemon, id);
	//mostrarBitmap();
	//
	// Procedimiento NEW casi terminado
	//
	if(resultadoNew == 0){
	envioDeMensajeAppeared(pokemon,posX,posY,id); // Prueba
	}else if(resultadoNew == -1)log_error(loggerObligatorio,"No hay espacio en disco");
	// Preguntar que mensaje enviar en el caso que no haya espacio en disco
	destruirInfoPaquete(infoNew);
	free(pokemon);

}

void procedimientoMensajeGet(t_infoPack *infoGet){
	void *paqueteGet= infoGet->paquete;
	int socket = infoGet->socket;
	char* pokemon = unpackPokemonGet(paqueteGet);

	uint32_t id = unpackID(paqueteGet);

	if(esSocketDeBroker(socket)){
    envioDeACK(id, t_GET);
	}

	procedimientoGET(id,pokemon);

	// TODAVIA NO FUNCIONA  // Prueba
	destruirInfoPaquete(infoGet);
	free(pokemon);
}

void procedimientoMensajeCatch(t_infoPack *infoCatch){
	void *paqueteCatch= infoCatch->paquete;
	int socket = infoCatch->socket;
	char* pokemon = unpackPokemonCatch(paqueteCatch);
	uint32_t tamanioPokemon = strlen(pokemon);
	uint32_t id = unpackID(paqueteCatch);
	uint32_t posX = unpackCoordenadaX_Catch(paqueteCatch, tamanioPokemon);
	uint32_t posY = unpackCoordenadaY_Catch(paqueteCatch, tamanioPokemon);

	if(esSocketDeBroker(socket)){
    envioDeACK(id, t_CATCH);
	}

	int resultadoCatch = procedimientoCATCH(pokemon,posX,posY);

	//log_info(loggerObligatorio,"Pokemon: %s posX: %d posY: %d id: %d",pokemon,posX,posY,id);

//
	//
	// ACA DEBERIA IR LO QUE DEBE HACERSE CON CATCH Y PASAR LAS VARIABLES A ESA FUNCION
	//
	uint32_t resultado; // Prueba

	if(resultadoCatch == -1) resultado = 0;
	else resultado = 1;

	if(resultadoCatch != -2){
	int seEnvioCaught = envioDeMensajeCaught(resultado,id); // Prueba
	if(seEnvioCaught == -1) log_error(loggerObligatorio,"Error al enviar el mensaje");
	}
	free(pokemon);
	destruirInfoPaquete(infoCatch);
}

void conexionAColaNew(){
	socket_new = crear_conexion(ip_broker,puerto_broker);
	if(socket_new != -1){
	if (identificadorProcesoGC == NULL){
	conectarseAColaMensajes(socket_new,"Gamecard",t_NEW);
	conexionNew = 1;
	}
	else{
		conectarseAColaMensajes(socket_new,identificadorProcesoGC,t_NEW);
		conexionNew = 1;

		log_info(loggerObligatorio,"ME RECONECTE AL BROKER. LE MANDE EL ID: %s",identificadorProcesoGC);
	}

	sem_post(&conexionRecuperadaDeNew);
	}
}

void conexionAColaCatch(){
	socket_catch = crear_conexion(ip_broker,puerto_broker);
	if(socket_catch != -1){
		conectarseAColaMensajes(socket_catch,identificadorProcesoGC,t_CATCH);
		conexionCatch = 1;
		sem_post(&conexionRecuperadaDeCatch);
	}

}

void conexionAColaGet(){
	socket_get = crear_conexion(ip_broker,puerto_broker);
	if(socket_get != -1){
		conectarseAColaMensajes(socket_get,identificadorProcesoGC,t_GET);
		conexionGet = 1;
		sem_post(&conexionRecuperadaDeGet);
	}

}

void reconexionColaNew(){
	log_info(loggerObligatorio,"Conexion caida, se reintentara en %d segundos",tiempo_de_reconexion);
	while (1){
		sleep(tiempo_de_reconexion);
		conexionAColaNew();
		if(conexionNew){
			log_info(loggerObligatorio, "La reconexion al Broker se realizo con exito");
			conexionNew = 1;
			break;
		}

		log_info(loggerObligatorio, "Fallo la reconexion, se volvera a intentar en %d segundos",tiempo_de_reconexion);
		}
}

int conectarseAColaMensajes(int socket,char *identificador,t_operacion operacion){
	void *paquete = pack_Handshake(identificador,operacion);
	uint32_t tamPaquete = strlen(identificador) + 1 + sizeof(uint32_t) + sizeof(t_operacion);
	int resultado = packAndSend(socket,paquete,tamPaquete,t_HANDSHAKE);

	free(paquete);
	return resultado;

}

void gestionMensajesGB(int* socket_servidor){
	struct sockaddr_in dir_cliente;

	int tam_direccion = sizeof(struct sockaddr_in);
	log_info(loggerObligatorio, "Esperando al GameBoy..");

	while(1){
	pthread_t hiloAtencionCliente;

	int socket = accept(*socket_servidor, (void*) &dir_cliente, &tam_direccion);

	log_info(loggerObligatorio, "Se conectó el GameBoy!");

	if(socket != -1){
		if(pthread_create(&hiloAtencionCliente,NULL,(void*)atender_cliente,&socket) == 0){
					 pthread_detach(hiloAtencionCliente);
	    }
	}

	}
}

void envioDeACK(uint32_t id, t_operacion operacion){
	int socket = crear_conexion(ip_broker, puerto_broker);
	void* paquete = pack_Ack(id, operacion, identificadorProcesoGC);
	uint32_t tamPaquete = sizeof(id) + sizeof(t_operacion) + strlen(identificadorProcesoGC) + 1 + sizeof(uint32_t);
	packAndSend(socket, paquete, tamPaquete, t_ACK);
	log_info(loggerObligatorio, "El envio del ACK del mensaje con ID [%d] se realizo con exito",id);
	close(socket);
	free(paquete);
}

void crearLoggerObligatorio(){
	char *nombrePrograma = "Log_Gamecard";
	loggerObligatorio = log_create(ruta_de_log,nombrePrograma,true, LOG_LEVEL_INFO);
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

int crear_conexion(char *ip, char* puerto){
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	// Cambio. Aunque puede que tengamos que devolverlo a lo normal.
	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1){
		log_error(loggerObligatorio,"No se pudo conectar al broker");
		freeaddrinfo(server_info);
		close(socket_cliente);
		return -1;
	}

	else log_info(loggerObligatorio,"Hubo conexion ");
	freeaddrinfo(server_info);

	return socket_cliente;
}

bool esSocketDeBroker(int socket){
	return socket == socket_new || socket == socket_catch || socket == socket_get;
}

void atender_cliente(int* socket){
	if(esSocketDeBroker(*socket)){
		log_info(loggerObligatorio, "Atendiendo al Broker...");
	}
	else{
		log_info(loggerObligatorio, "Atendiendo al Gameboy...");
	}

	Header headerRecibido;
	headerRecibido = receiveHeader(*socket);

	procesar_solicitud(headerRecibido, *socket);
}

void envioDeACKGB(int conexion, uint32_t id, t_operacion operacion){

	void* paquete = pack_Ack(id, operacion, "GameCard");
	uint32_t tamPaquete = sizeof(id) + sizeof(t_operacion) + strlen("Gamecard") + 1 + sizeof(uint32_t);
	packAndSend(conexion, paquete, tamPaquete, t_ACK);
	log_info(loggerObligatorio, "El envio del ACK del mensaje con ID [%d] se realizo con exito",id);

	free(paquete);
}

void procesar_solicitud(Header headerRecibido, int cliente_fd) {
	//int size;
	//void* msg;
	uint32_t id;
	uint32_t tamanio = headerRecibido.tamanioMensaje;
	uint32_t tamanioPokemon;
	uint32_t posX;
	uint32_t posY;

	char* pokemon;

		switch (headerRecibido.operacion) {

		case t_GET:
			log_info(loggerObligatorio,"Llegó un mensaje del tipo GET del GameBoy");

			void* paqueteGet = receiveAndUnpack(cliente_fd, tamanio);

			pokemon = unpackPokemonGet(paqueteGet);
			tamanioPokemon = strlen(pokemon) + 1;
			id = unpackID(paqueteGet);

			envioDeACKGB(cliente_fd,id, t_GET);
			//log_info(loggerObligatorio,"Pokemon: %s id: %d",pokemon,id);


			procedimientoGET(id,pokemon);

			free(pokemon);
			free(paqueteGet);
			break;

		case t_CATCH:
			log_info(loggerObligatorio,"Llegó un mensaje del tipo CATCH del GameBoy");
			void* paqueteCatch = receiveAndUnpack(cliente_fd, tamanio);
			pokemon = unpackPokemonCatch(paqueteCatch);
			tamanioPokemon = strlen(pokemon);


			id = unpackID(paqueteCatch);
			posX = unpackCoordenadaX_Catch(paqueteCatch, tamanioPokemon);
			posY = unpackCoordenadaY_Catch(paqueteCatch, tamanioPokemon);
			envioDeACKGB(cliente_fd,id, t_CATCH);

			int resultadoCatch = procedimientoCATCH(pokemon,posX,posY);
			//log_info(loggerObligatorio,"Pokemon: %s posX: %d posY: %d id: %d",pokemon,posX,posY,id);

//
			//
			// ACA DEBERIA IR LO QUE DEBE HACERSE CON CATCH Y PASAR LAS VARIABLES A ESA FUNCION
			//
			uint32_t resultado; // Prueba

			if(resultadoCatch == -1) resultado = 0;
			else resultado = 1;

			if(resultadoCatch != -2){
			envioDeMensajeCaught(resultado,id); // Prueba
			}
			free(pokemon);
			free(paqueteCatch);
			break;


		case t_NEW:
			log_info(loggerObligatorio,"Llegó un mensaje del tipo NEW del GameBoy");
			void* paqueteNew = receiveAndUnpack(cliente_fd, tamanio);
			pokemon = unpackPokemonNew(paqueteNew);
			tamanioPokemon = strlen(pokemon);


			id = unpackID(paqueteNew);
			posX = unpackCoordenadaX_New(paqueteNew, tamanioPokemon);
			posY = unpackCoordenadaY_New(paqueteNew, tamanioPokemon);
			uint32_t cantPokemon = unpackCantidadPokemons_New(paqueteNew, tamanioPokemon);
			envioDeACKGB(cliente_fd,id, t_NEW);

			int resultadoNew = procedimientoNEW(pokemon,posX,posY,cantPokemon);
			//log_info(loggerObligatorio,"Pokemon: %s posX: %d posY: %d cantidad: %d id: %d",pokemon,posX,posY,cantPokemon, id);
			//mostrarBitmap();
			//
			// Procedimiento NEW casi terminado
			//
			if(resultadoNew == 0){
			envioDeMensajeAppeared(pokemon,posX,posY,id); // Prueba
			}else if(resultadoNew == -1) log_error(loggerObligatorio,"No hay espacio en disco");

			// Preguntar que mensaje enviar en el caso que no haya espacio en disco
			free(pokemon);
			free(paqueteNew);
			break;

		default:
			pthread_exit(NULL);
			break;
		}
}

void leerArchivoDeConfiguracion(void){
	char* path = "GameCard.config";
	archivoConfig = config_create(path);
	if (archivoConfig == NULL){
		//log_error(loggerObligatorio,"Archivo de configuracion no encontrado");
	}
	setearValoresDeGame(archivoConfig);
	//log_info(loggerObligatorio,"La configuracion fue cargada exitosamente");
}

void setearValoresDeGame(t_config *config){
	ip_gc = config_get_string_value(config,"IP_GAMECARD");
	puerto_gc = config_get_string_value(config,"PUERTO_GAMECARD");
	ip_broker = config_get_string_value(config,"IP_BROKER");
	puerto_broker = config_get_string_value(config,"PUERTO_BROKER");
	path_de_tallgrass = config_get_string_value(config,"PUNTO_MONTAJE_TALLGRASS");
	tiempo_de_reconexion = config_get_int_value(config,"TIEMPO_DE_REINTENTO_CONEXION");
	tiempo_de_reintento_operacion = config_get_int_value(config,"TIEMPO_DE_REINTENTO_OPERACION");
	tiempo_de_retardo = config_get_int_value(config,"TIEMPO_RETARDO_OPERACION");
	identificadorProcesoGC = config_get_string_value(config,"ID");
	ruta_de_log = config_get_string_value(config,"LOG_FILE");
}

int envioDeMensajeCaught(uint32_t atrapado, uint32_t idmensaje){
	int socket = crear_conexion(ip_broker,puerto_broker);
	if(socket != -1){
		void* paqueteCaught = pack_Caught(idmensaje,atrapado);
		uint32_t tamPaquete =  2*sizeof(uint32_t);
		int resultado = packAndSend(socket,paqueteCaught,tamPaquete,t_CAUGHT);
		log_info(loggerObligatorio,"Le envié un mensaje CAUGHT con id correlativo %d al broker",idmensaje);
		close(socket);
		free(paqueteCaught);
		return resultado;
	} return -1;
}

int envioDeMensajeAppeared(char* pokemon, uint32_t posx, uint32_t posy, uint32_t idmensaje){

	int socket = crear_conexion(ip_broker,puerto_broker);
	if(socket != -1){
	void* paqueteAppeared = pack_Appeared(idmensaje,pokemon,posx,posy);
	uint32_t tamPaquete = strlen(pokemon) + 4*sizeof(uint32_t);
	int resultado = packAndSend(socket,paqueteAppeared,tamPaquete,t_APPEARED);
	log_info(loggerObligatorio,"Le envié un mensaje APPEARED con id correlativo %d al broker",idmensaje);
	close(socket);
	free(paqueteAppeared);
  	return resultado;
	} return -1;
}
int envioDeMensajeLocalized(char* pokemon,uint32_t idmensaje,uint32_t cantidadParesCoordenadas,uint32_t arrayCoordenadas[]){


	int socket = crear_conexion(ip_broker,puerto_broker);


	if(socket != -1){

	void* paqueteGet = pack_Localized(idmensaje,pokemon,cantidadParesCoordenadas,arrayCoordenadas);
	uint32_t tamPaquete = strlen(pokemon) + 3*sizeof(uint32_t) + cantidadParesCoordenadas*2*sizeof(uint32_t);
	int resultado = packAndSend(socket,paqueteGet,tamPaquete,t_LOCALIZED);
	log_info(loggerObligatorio,"Le envié un mensaje LOCALIZED con id correlativo %d al broker",idmensaje);
	close(socket);
	free(paqueteGet);

	return resultado;

	} return -1;

}

//////////////////////////////////////////////////
//////////////	UTILIDADES PARA DAR INICIO AL FS
////////////////////////////////////////////////////


// da inicio al punto de montaje
//

void iniciar_punto_de_montaje(char *puntoDeMontaje){
		carpetas_iniciar(puntoDeMontaje);

		//Creamos string del metadata del fs
		char* rutaMetadataFS = string_from_format("%s/Metadata.bin",RUTA_DE_METADATA_MONTAJE);

		config_fs = config_create(rutaMetadataFS);

		// Vemos si existe el .bin con la configuracion del FS y cargamos el config a las variables globales
		// En el caso que no exista la carpeta de configuracion crea una por default (Con el ejemplo que estaba en el pdf)
		if (config_fs == NULL || config_keys_amount(config_fs) == 0){
			inicio_default(puntoDeMontaje);
		}


		carga_config_fs();
		bloques_iniciar();
		iniciarBitmap();

		// Limpieza de variables
		free(rutaMetadataFS);
}

// Da un inicio default con la configuracion que esta en el pdf
void inicio_default(char* puntoDeMontaje){

	FILE* archivoAux;

	// Cargamos la ruta de metadata.bin
	// Y creamos el archivo metadata.bin VACIO
	char* auxMetadata = string_from_format("%s/Metadata/Metadata.bin",puntoDeMontaje);
	archivoAux = fopen(auxMetadata,"a");
	fclose(archivoAux);

	// Tratamos al archivo metadata.bin como un config
	// Y le agregamos la configuracion del pdf como default
	// Tamaño del bloque 64
	// cantidad de bloques 5192
	// Numero mágico TALL_GRASS
	config_fs = config_create(auxMetadata);
	config_set_value(config_fs, "BLOCK_SIZE", blocksizedefault);
	config_set_value(config_fs, "BLOCKS", blocksdefault);
	config_set_value(config_fs, "MAGIC_NUMBER", "TALL_GRASS");
	config_save(config_fs);


	// Se crea el archivo del bitmap
	char* auxBitMap = string_from_format("%s/Metadata/Bitmap.bin",puntoDeMontaje);
	archivoAux = fopen(auxBitMap,"a");
	fclose(archivoAux);
	free(auxMetadata);
	free(auxBitMap);
}

// Inicializa los bloques
// Por ahora solo los inicia en 0 y sobreescribe. Hay que ver como hacer para que no
// sobreescriba datos anteriores
void bloques_iniciar(){
	FILE *bloque;
	int i;
	// Se crean los bloques pero hay un problema porque elimina los bloques
	// que anteriormente fueron creados
	// en esta funcion tengo mucha perdida de memoria. Revisar
	for(i = 1; i <= CANTIDAD_DE_BLOQUES ; i++){
		char* bloque_a_crear = string_from_format("%s/%d.bin",RUTA_DE_BLOQUES,i);
		bloque = fopen(bloque_a_crear,"a");
		fclose(bloque);
		free(bloque_a_crear);
	}

	log_info(loggerObligatorio,"Bloques creados");
}


//	Crea las carpetas que debe tener el fs (las carpetas bases)
// 	que son metadata,files y blocks.
void carpetas_iniciar(char* puntoDeMontaje){

	// Se crea la carpeta de metadata
	RUTA_DE_METADATA_MONTAJE = string_from_format("%s/Metadata",puntoDeMontaje);
	mkdir(RUTA_DE_METADATA_MONTAJE,0777);

	// Se crea la carpeta de FILES
	RUTA_DE_FILES = string_from_format("%s/Files",puntoDeMontaje);
	crearDirectorio(RUTA_DE_FILES);

	// Se crea la carpeta de Blocks
	RUTA_DE_BLOQUES = string_from_format("%s/Blocks",puntoDeMontaje);
	mkdir(RUTA_DE_BLOQUES,0777);
}


// Carga la configuracion que está en el metadata.bin
// del fs. Toma el tamaño de bloque, bloques del bitmap y el magicnumber
void carga_config_fs(){
	TAMANIO_DE_BLOQUE = config_get_int_value(config_fs,"BLOCK_SIZE");
	CANTIDAD_DE_BLOQUES = config_get_int_value(config_fs,"BLOCKS");
	MAGIC_NUMBER = config_get_string_value(config_fs,"MAGIC_NUMBER");
}

// Actualiza el bitmap. Busca en los bloques a los bloque que estan ocupados o contienen datos y coloca
// en el bitmap 1 para indicar que esta ocupado ese bloque
void actualizarBitmap(){
	int i;

	for(i = 1; i <= CANTIDAD_DE_BLOQUES; i++){

			bitarray_clean_bit(bitmap,i-1);
			bloquesLibres++;
	}

}

void recuperarEstadoBitmap(){
	int i;

	for(i = 1; i <= CANTIDAD_DE_BLOQUES; i++){
			if(!bitarray_test_bit(bitmap,i-1)) bloquesLibres++;

	}
}

// Inicia el bitmap si existe el archivo bitmap.bin, si no existe crea un bitmap basado en el metadata.
void iniciarBitmap(){
	FILE *archi;
	int tamanio = 0;
	bool existenciaDeBitmap = 1;

	char* directorioBitmap = string_from_format("%s/Bitmap.bin",RUTA_DE_METADATA_MONTAJE);
	archi = fopen(directorioBitmap,"rb+");

	if(archi != NULL){
	fseek(archi,0,SEEK_END);
	tamanio = ftell(archi);
	fseek(archi,0,SEEK_SET);
	}

	if(tamanio == 0){
	//SE CREA EL BITMAP VACIO
	archi = fopen(directorioBitmap,"wb+");
	char* bitArray_vacio = calloc(1,((CANTIDAD_DE_BLOQUES+7)/8));
	fwrite((void* )bitArray_vacio,((CANTIDAD_DE_BLOQUES+7)/8),1,archi);
	//fclose(archi);
	free(bitArray_vacio);
	existenciaDeBitmap = 0;
	}

	int fd = fileno(archi);
	fseek(archi,0,SEEK_END);
	int tamanioArchi = ftell(archi);
	fseek(archi,0,SEEK_SET);

	//SE "LEE" el contenido del archivo
	char* bitarray = (char*) mmap(NULL, tamanioArchi, PROT_READ | PROT_WRITE , MAP_SHARED, fd,0);

	bitmap = bitarray_create_with_mode(bitarray, tamanioArchi, LSB_FIRST);

	if(!existenciaDeBitmap){

		actualizarBitmap();
	}else{

		recuperarEstadoBitmap();
	}

	log_info(loggerObligatorio,"Bitmap cargado y actualizado");

	free(directorioBitmap);
	fclose(archi);
}

////////////////////////////////////////////////
///////////////////////UTILIDADES VARIAS
////////////////////////////////////////////////


void crearDirectorio(char* pathDeDirectorio){
	mkdir(pathDeDirectorio,0777);
	char* rutaMetadata = agregar_metadata_a_path(pathDeDirectorio);

	FILE* metaDir = fopen(rutaMetadata,"a");
	fclose(metaDir);

	t_config* configMetaDir = obtener_metadata_de_ruta(pathDeDirectorio);
	config_set_value(configMetaDir,"DIRECTORY","Y");
	config_save(configMetaDir);
	config_destroy(configMetaDir);
	free(rutaMetadata);
}



// MUESTRA LAS RUTAS AUNQUE ESTARIA BUENO USAR EL LOG(UNO CAPAZ PARA NOSOTROS) PARA INFORMARs
// NO SIRVE PARA EL TP PERO PODEMOS VERIFICAR COSAS CON ESTO
void mostrarRutas(){
	printf("------------------------\n");
		printf("Mostrando rutas de trabajo \n");
		printf("Metadata: %s \n",RUTA_DE_METADATA_MONTAJE);
		printf("Files: %s \n",RUTA_DE_FILES);
		printf("Blocks: %s \n",RUTA_DE_BLOQUES);
		printf("\n");
		printf("Mostrando config del FS \n");
		printf("El tamaño del bloque del metadata es: %i \n",TAMANIO_DE_BLOQUE);
		printf("La cantidad de bloques es: %i \n",CANTIDAD_DE_BLOQUES);
		printf("El magic number es: %s \n",MAGIC_NUMBER);
}

//Agrega "/Metadata.bin" al string pasado
char* agregar_metadata_a_path(char *path){

	char* rutaMetadata;
	rutaMetadata = string_from_format("%s/Metadata.bin",path);

	return rutaMetadata;
}

//	retorna el metadata del archivo
// y hay que tratarlo como un config
// Si retorna NULL no existe la metadata o la ruta
t_config* obtener_metadata_de_ruta(char *path){
	char* rutaMetadata;
	t_config *metadata;
	rutaMetadata = agregar_metadata_a_path(path);

	metadata = config_create(rutaMetadata);
	free(rutaMetadata);
	return metadata;
}

// Mediante una ruta retorna si es un directorio o archivo
// 1 si es directorio
// 0 si es archivo
// -1 si es error

bool esDirectorio(t_config* unMeta){
	bool resultado;

	char* tipoDeArchivo;

	tipoDeArchivo = config_get_string_value(unMeta,"DIRECTORY");

	if(string_equals_ignore_case(tipoDeArchivo,"Y")){
		resultado = 1;
	}
	if(string_equals_ignore_case(tipoDeArchivo,"N")){
		resultado = 0;
	}
	return resultado;
}

bool esDirectorioPokemon(char* unPokemon){
	t_config* unConfig = obtenerFCB(unPokemon);
	bool resultado = esDirectorio(unConfig);
	config_destroy(unConfig);
	return resultado;
}

// Funcion que sirve solo para orientarse. No es necesaria ni nada.
void mostrarBitmap(){
	int i;
	printf("Mostrando bitmap: ");
	for(i = 0; i < CANTIDAD_DE_BLOQUES; i++){
		if(bitarray_test_bit(bitmap,i)) printf("%d",1);
		else printf("%d",0);
	}
	printf("\n");
}

// Retorna el length de un puntero a puntero
int length_punteroAPuntero(char** punteroAPuntero){
	int i = 0;

	while(punteroAPuntero[i] != NULL){
		i++;
	}
	return i;
}

// Limpia un puntero a puntero
void limpiarPunteroAPuntero(char** puntero){
	int i = 0;

	while(puntero[i] != NULL){
		free(puntero[i]);
		i++;
	}
	free(puntero);
}

void limpiarPunteroAPuntero2punto0(char** puntero,int cantidad){
	int i = 0;

	for(i = 0; i < cantidad ; i++){
		free(puntero[i]);

	}
}

// Recibe un arreglo de arrays y el arrayObjetivo.
// Retorna el numero de linea donde se encuentra dicho array
int obtenerLinea(char** lineas,char* array){
				int i = 0;
				while(lineas[i] != NULL){

				if(string_contains(lineas[i],array)){
					 return i;
					}else i++;
				}
				return -1;
}

// Recibe un array y un caracter
// Busca la posicion de un caracter de un array y retorna su posicion
int buscarPosicionDeCaracter(char* array,char caracter){
	int i;
	int tamanio = strlen(array);
	for(i = 0; i < tamanio; i++){

		if(array[i] == caracter) return i;
	}

	return -1;
}

// Tan solo crea un path del tipo bloque y lo retorna
char* crearPathDeBloque(char* nroDeBloque){
	char* rutaBloque = string_from_format("%s/%s.bin",RUTA_DE_BLOQUES,nroDeBloque);
	return rutaBloque;
}

// Recibe un arreglo de posiciones con sus cantidades(1-1=10), la nueva linea que va a reemplazar en el nroDeLinea pasado
// y el nroDeLinea que se va a realizar el reemplazo
// retorna un string que contiene todas las lineas (Ya habiendo reemplazado la linea deseada)
char* actualizarPosicion(char** posicionesSeparadas,char* lineaAActualizar,int nroDeLinea){
	int i = 0;
	char* posicionActualizada = string_new();


	while(posicionesSeparadas[i] != NULL){

		if(i != nroDeLinea) {

			string_append(&posicionActualizada,posicionesSeparadas[i]);
			string_append(&posicionActualizada,"\n");
		}else{
			string_append(&posicionActualizada,lineaAActualizar);
		}

		//if(posicionesSeparadas[i+1] != NULL){

		//}


		i++;
	}

	return posicionActualizada;
}

char* eliminarPosicion(char** posicionesSeparadas,int nroDeLinea){
	int i = 0;
	char* posicionActualizada = string_new();
	string_append(&posicionActualizada,"");

	while(posicionesSeparadas[i] != NULL){
		if(i != nroDeLinea){
			string_append(&posicionActualizada,posicionesSeparadas[i]);
			string_append(&posicionActualizada,"\n");
		}
		i++;
	}
	printf("Pos separada:\n%s\n",posicionActualizada);
	return posicionActualizada;


}

// Escribe un bloque, le debemos decir el nombre/numero de bloque, lo que deseamos escribir y
// el desplazamiento dentro de ese bloque(pisa los datos que se encuentren en esa posicion y las que siguen).
// Si es 0, empieza a escribir como si estuviera vacio. (Va a pisar lo que haya dentro)
// Retorna la cantidad de bytes
int escribirBloquePosicionandoPuntero(char* nombreDeBloque,char* stringAEscribir,int desplazamiento){

	if(strlen(stringAEscribir) == 0) return 0;

	char* rutaBloque = crearPathDeBloque(nombreDeBloque);

	FILE* bloque = fopen(rutaBloque,"rw+");
	//fseek(bloque,0,SEEK_END);
	//int tamanioBloque = ftell(bloque);


	int bytesAEscribir = TAMANIO_DE_BLOQUE - desplazamiento;

	if(strlen(stringAEscribir) < bytesAEscribir){
		bytesAEscribir = strlen(stringAEscribir);
	}

	fseek(bloque,desplazamiento,SEEK_CUR);

	fwrite(stringAEscribir,bytesAEscribir,1,bloque);

	fclose(bloque);

	free(rutaBloque);

	return bytesAEscribir;

}

//Recibe una posicion y su cantidad (1-1=10) y le suma cantidadASumar(1)
// retorna el nuevo array con la nueva cantidad (1-1=11)
char* sumarCantidadPokemon(char* lineaPokemon,int cantidadASumar){

	int cantidadPokemon = obtenerCantidad(lineaPokemon);

	cantidadPokemon += cantidadASumar;

	char** arraySeparado = string_split(lineaPokemon,"=");
	char* cantidadEnString = string_itoa(cantidadPokemon);
	char* nuevaLineaPokemon = string_from_format("%s=%s\n",arraySeparado[0],cantidadEnString);

	//string_append(&nuevaLineaPokemon,cantidadEnString);
	limpiarPunteroAPuntero(arraySeparado);
	free(cantidadEnString);
	return nuevaLineaPokemon;
}

int desplazamientoDelArrayHastaLineaPosicion(char** posicionesSeparadas,char* posObjetivo){
	int i = 0;
	int offset = 0;

	//Vemos si es el fin de posicionesSeparadas
	while(posicionesSeparadas[i] != NULL){
		//Vemos si esa posicion coincide con nuestra posicion objetivo
		if(string_contains(posicionesSeparadas[i],posObjetivo))break;
		//Si no sumamos al desplazamiento la cantidad de caracteres de esa posicion
		offset += strlen(posicionesSeparadas[i]) + 1;
		//Avanzamos de linea
		i++;
	}

	//offset = offset + i;

	return offset;
}

//	Recibe un arreglo de posiciones con sus cantidades y una posicion objetivo
//	retorna el desplazamiento que habría desde el iniico hasta el caracter despues del "=" de esa posicion objetivo. (es decir la primera
//	posicion de la cantidad pokemon)
//	como si fuera que lineasSeparadas estuviera concatenada con \n en cada una de las concatenaciones de cada linea
int desplazamientoDelArrayHastaCantPokemon(char** posicionesSeparadas,char* posObjetivo){
	int i = 0;
	int offset = 0;

	//Vemos si es el fin de posicionesSeparadas
	while(posicionesSeparadas[i] != NULL){
		//Vemos si esa posicion coincide con nuestra posicion objetivo
		if(string_contains(posicionesSeparadas[i],posObjetivo))break;
		//Si no sumamos al desplazamiento la cantidad de caracteres de esa posicion
		offset = strlen(posicionesSeparadas[i]) + 1;
		//Avanzamos de linea
		i++;
	}
	// suma el offset, numero de linea(ya que hay que sumar 1 caracter por cada linea saltada ya que en el posicionesSeparadas no esta
	// contemplado el \n) ,la posicion del caracter "=" en la linea y el +1 porque avanzamos un caracter más despues del "="

	offset = offset + 1 + buscarPosicionDeCaracter(posicionesSeparadas[i],'=');

	return offset;
}

// Retorna la posicion del bloque afectado
int bloqueAfectado(int desplazamiento){
	return desplazamiento/TAMANIO_DE_BLOQUE;
}

// Retorna el desplazamiento que hay que tener en el bloque.
//  El desplazamiento dado por parametro debe ser basado en el desplazamiento TOTAL del array que contiene todas las posiciones.
int desplazamientoEnBloque(int desplazamiento){
	return desplazamiento%TAMANIO_DE_BLOQUE;
}

// Retorna la cantidad de pokemones dado una lineaPokemon/posicionYCantidad (1-1=10)
// retorna 10
int obtenerCantidad(char* lineaPokemon){

	char** posicionYCantidad = string_split(lineaPokemon,"=");

			int numeroBuscado = atoi(posicionYCantidad[1]);

			limpiarPunteroAPuntero(posicionYCantidad);
			return numeroBuscado;
}

//	Recibe el nombre del bloque, el desplazamiento que debe haber en el bloque y los caracter que hay que colocar
// pisa al caracter que se encuentre en dicha posicion
// Es util solo cuando la  cantidadDePokemones antigua y actualizada tienen el mismo length
// Es decir: Que si antes de agregar tiene 10 y leugo de agregar tiene 18 es util usar esta funcion
// Ya que nos ahorramos el hecho de reescribir todos los bloques
void sobreescribirUnCaracter(char* nombreDeBloque,int desplazamiento,char* caracterAEscribir){

	char* rutaBloque = crearPathDeBloque(nombreDeBloque);

	//printf("Ruta de bloque %s\n",rutaBloque);
	FILE* bloque = fopen(rutaBloque,"r+w");

	fseek(bloque,desplazamiento,SEEK_CUR);

	fputs(caracterAEscribir,bloque);

	fclose(bloque);

	free(rutaBloque);
}

// Funcion en proceso. dado un string va a retornar la cantidad de bloques que se necesitan para guardar
// ese string
int bloquesNecesariosParaEscribir(char* datos){
	int  cantidadDeBytes = strlen(datos);
	if(cantidadDeBytes == 0) return 0;

	int bloquesNecesarios = (cantidadDeBytes/TAMANIO_DE_BLOQUE);

	if(cantidadDeBytes%TAMANIO_DE_BLOQUE != 0) bloquesNecesarios++;
	return bloquesNecesarios;
}


//Recibe el nombre del pokemon y un numero de bloque que va a ser
// añadido BLOCKS de su metadata
void agregarBloqueAPokemonBis(t_FCB* pokemone,int numeroDeBloque){


	char** bloques = string_get_string_as_array(pokemone->bloques);
	int i = 0;
	char* cargaDeBloques = string_new();

	while(bloques[i] != NULL){
		string_append(&cargaDeBloques,bloques[i]);
		string_append(&cargaDeBloques,",");
		i++;
	}

	char* numeroDeBloqueEnString = string_itoa(numeroDeBloque);
	string_append(&cargaDeBloques,numeroDeBloqueEnString);
	bitarray_set_bit(bitmap,numeroDeBloque -1);
	bloquesLibres--;
	char* bloquesActualizados = string_from_format("[%s]",cargaDeBloques);

	free(pokemone->bloques);
	pokemone->bloques = bloquesActualizados;

	free(cargaDeBloques);
	limpiarPunteroAPuntero(bloques);
	free(numeroDeBloqueEnString);
}

int solicitarBloquesParaPokemonBis(t_FCB* pokemone,int cantidadDeBloques){
		int i;
		int resultado;
	//	printf("Blques libres: %d\n",bloquesLibres);
		sem_wait(&semDeBloques);
		if(cantidadDeBloques <= bloquesLibres){

			for(i = 0; i < cantidadDeBloques ; i++){
		int numeroDeBloqueVacio = solicitarBloqueVacio();

		printf("\033[1;35m");
		log_info(loggerObligatorio,"Voy a agregar el bloque %d al pokemon %s",numeroDeBloqueVacio,pokemone->pokemon);
		printf("\033[0m");
		agregarBloqueAPokemonBis(pokemone,numeroDeBloqueVacio);

			}
			resultado = 0;
		}else resultado = -1;


		sem_post(&semDeBloques);
		return resultado;
}

// Funcion en proceso. Se le da la cantidad de bloques que debe reservar el pokemon
// Deberia retornar 0 si fueron solicitados todos
// o -1 si es que no hay suficiente espacio para asignar todos esos bloques



void quitarUltimoBloqueAPokemon(t_FCB *pokimane){

	char** bloques = string_get_string_as_array(pokimane->bloques);
	int ultimaPosicion = length_punteroAPuntero(bloques);
	char* bloquesActualizados = string_new();

	if(ultimaPosicion == 1){
		string_append(&bloquesActualizados,"");
	}else{

	int i;
		for(i = 0; i < ultimaPosicion - 1; i++){
			string_append(&bloquesActualizados,bloques[i]);
			if(bloques[i+2] != NULL){
				string_append(&bloquesActualizados,",");
			}else break;
		}
	}


	char* rutaDelBloque = crearPathDeBloque(bloques[ultimaPosicion-1]);
	FILE* archivoDeBloque = fopen(rutaDelBloque,"w");
	fclose(archivoDeBloque);
	free(rutaDelBloque);
	bitarray_clean_bit(bitmap,atoi(bloques[ultimaPosicion-1])-1);

	printf("\033[1;35m");
	log_info(loggerObligatorio,"El bloque %s que pertenecia al pokemon %s va a liberarse",bloques[ultimaPosicion-1],pokimane->pokemon);
	printf("\033[0m");


	char* bloqueNuevo = string_from_format("[%s]",bloquesActualizados);

	free(pokimane->bloques);
	pokimane->bloques = bloqueNuevo;

	free(bloquesActualizados);
	limpiarPunteroAPuntero(bloques);
}

void liberarBloquesParaPokemon(t_FCB* pokimane, int cantidadDeBloques){
	int i;

	for(i = 0; i < cantidadDeBloques; i++){
		quitarUltimoBloqueAPokemon(pokimane);
	}
}

////////////////////////////////////////////////
////////////////////////// COSAS DEL POKEMON
//////////////////////////////////////////

// Retorna el metada de un pokemon
// dado solo su nombre (Puede retornar el metadata del directorio, cuidado)
//
t_config* obtenerFCB(char *nombrePokemon){
	char *ruta_pokemon;
	t_config* configPokemon;

	ruta_pokemon = string_from_format("%s/%s/Metadata.bin",RUTA_DE_FILES,nombrePokemon);

	configPokemon = config_create(ruta_pokemon);
	free(ruta_pokemon);

	return configPokemon;
}

// Retorna si existe el pokemon: NO GENERA LEAKS
int existePokemon(char* nombrePokemon){
	int resultado = 0;


	t_config* config_pokemon = obtenerFCB(nombrePokemon);

	if(config_pokemon == NULL){
		return resultado;
	}

	resultado = 1;
	config_destroy(config_pokemon);
	return resultado;
}

//	Crea un archivo con su metadata del nombre del pokemon
//	Obviamente el archivo va a estar vacio hasta ser llenado
void crearPokemon(char* nombrePokemon){

	char* path_pokemon = string_from_format("%s/%s",RUTA_DE_FILES,nombrePokemon);

	mkdir(path_pokemon,0777);

	string_append(&path_pokemon,"/Metadata.bin");
	FILE *archivoVacioMetadata = fopen(path_pokemon,"w");
	fclose(archivoVacioMetadata);

	t_config* metaPokemon = config_create(path_pokemon);
	config_set_value(metaPokemon,"DIRECTORY","N");
	config_set_value(metaPokemon,"SIZE","0");
	config_set_value(metaPokemon,"BLOCKS","[]");
	config_set_value(metaPokemon,"OPEN","N");
	config_save(metaPokemon);
	config_destroy(metaPokemon);

	free(path_pokemon);
}

//Abre el archiv ode pokemon
void abrirArchivo(char* nombrePokemon){
	// CAPAZ UN MUTEX
	t_config* metaPokemon = obtenerFCB(nombrePokemon);
	config_set_value(metaPokemon,"OPEN","Y");
	config_save(metaPokemon);
	config_destroy(metaPokemon);
}

// Cierra el archivo de pokemon
void cerrarArchivoBis(t_FCB* pokemon){
	// CAPAZ UN MUTEX
	sem_wait(&aperturaDeArchivo);
	t_config* metaPokemon = obtenerFCB(pokemon->pokemon);
	char* tamanioEnString = string_itoa(pokemon->size);

	config_set_value(metaPokemon,"SIZE",tamanioEnString);
	config_set_value(metaPokemon,"BLOCKS",pokemon->bloques);
	config_set_value(metaPokemon,"OPEN","N");
	config_save(metaPokemon);
	config_destroy(metaPokemon);
	free(tamanioEnString);

	sem_post(&aperturaDeArchivo);
	printf("\033[0;32m");
	log_info(loggerObligatorio,"Archivo %s cerrado",pokemon->pokemon);
	printf("\033[0m");
}

// Dice si el archivo del pokemon esta abierto o no
bool archivoAbierto(char* nombrePokemon){
	int valor;
	t_config* metaPokemon = obtenerFCB(nombrePokemon);
	char *estaAbierto = config_get_string_value(metaPokemon,"OPEN");
	if(string_equals_ignore_case(estaAbierto,"Y")) valor = 1;
	else valor = 0;

	config_destroy(metaPokemon);
	return valor;
}

void *reintentoAbrir(void* argumentos){
	struct arg_estructura* valores = argumentos;

	while(1){
	if(verificarYAbrirArchivo(valores->nombrePokemon) == 1){
		pthread_exit(NULL);
		}
	printf("\033[0;31m");
	log_info(loggerObligatorio,"Esperando %d segundos para reintentar abrir %s",valores->tiempo,valores->nombrePokemon);
	printf("\033[0m");

	sleep(valores->tiempo);
	}

}

// Funcion auxiliar de  obtenerContenidoDeArchivo()
// lo que hace es recibir un char con el numero del archivo
// y llena un string con el contenido que tiene dicho archivo
// Se debe agregar al final \0 NO GENERA LEAKS
char* obtenerContenidoDeBloque(char* nroDeBloque){
	char* rutaDeFile = string_from_format("%s/%s.bin",RUTA_DE_BLOQUES,nroDeBloque);

	FILE* archi = fopen(rutaDeFile,"r");
		fseek(archi, 0, SEEK_END);
		int size = ftell(archi);
		fseek(archi,0,SEEK_SET);
		char* array = malloc(size+1);
		fread(array,size,1,archi);
		free(rutaDeFile);
		fclose(archi);
		array[size] = '\0';
		return array;
}

//----------------- GENERA LEAKS MIRAR PROXIMAMENTE
char* obtenerContenidoDeArchivo(char* bloques){
	// Los separa ["10","2","3"]
	if(strlen(bloques) <= 2) return string_new();

	char** arrayBloques = string_get_string_as_array(bloques);
	char* arrayDeArchivo = string_new();
	int i = 0;
	//Recorre cada numero y obtiene el contenido

	char* contenidoAux;

	while(arrayBloques[i] != NULL){
		contenidoAux = obtenerContenidoDeBloque(arrayBloques[i]);
		//	La linea obtenida es concatenada
		//	asi forma un string largo con all del contenido del archivo
		string_append(&arrayDeArchivo,contenidoAux);

		//printf("arrayDeArchivo: %s \n",arrayDeArchivo);
		//free(contenidoAux);// MIRAR ACA
		i++;

	}
	// MIRAR ESTO se necesita funcion que elimine puntero a punteros

	free(contenidoAux);
	limpiarPunteroAPuntero(arrayBloques);
	return arrayDeArchivo;

}
// Dice que si el arrayPosicion pertenece a las array de lineas(Que es la informacion de los bloques en un array)
// Hay que ver otra manera porque supongo que genera mucha carga a la memoria cargar un archivo completo
// El array posicion debe ser "2-2=" o "2-2" por ejemplo
bool contieneEstaPosicion(char* lineas,char* arrayPosicion){
			return string_contains(lineas,arrayPosicion);
}

// Retorna un entero que indica el numero de bloque vacio
int solicitarBloqueVacio(){
	int i;
	for(i = 0; i < CANTIDAD_DE_BLOQUES; i++){
		if(!bitarray_test_bit(bitmap,i))return i+1;


	}
	return -1;
}

int espacioLibreDelUltimoBloqueBis(t_FCB *pokemone){

	char** bloquesSeparados = string_get_string_as_array(pokemone->bloques);

	int cantidadDeBloques = length_punteroAPuntero(bloquesSeparados);

	int tamanioSobrando = cantidadDeBloques*TAMANIO_DE_BLOQUE - pokemone->size;

	limpiarPunteroAPuntero(bloquesSeparados);

	return tamanioSobrando;
}
// Retorna el espacio libre que tiene ese bloque
int espacioLibreDeBloqueBis(char* nombreDeBloque, t_FCB *pokemone){
	int ultimoBloqueAfectado = bloqueAfectado(pokemone->size);
	char* bloqueEnString = string_itoa(ultimoBloqueAfectado);

	char** bloquesSeparados = string_get_string_as_array(pokemone->bloques);

	int cantidadDeBloques = length_punteroAPuntero(bloquesSeparados);
	int i;
	int tamanioDeBloque = TAMANIO_DE_BLOQUE;

	for(i = 0; i < ultimoBloqueAfectado; i++){
		if(string_equals_ignore_case(bloquesSeparados[i],nombreDeBloque)){

			limpiarPunteroAPuntero(bloquesSeparados);
			free(bloqueEnString);
			return tamanioDeBloque;
		}
	}

	if(i == ultimoBloqueAfectado){
		tamanioDeBloque = TAMANIO_DE_BLOQUE -  desplazamientoEnBloque(pokemone->size);
	}

	limpiarPunteroAPuntero(bloquesSeparados);
	free(bloqueEnString);

	return tamanioDeBloque;
}

// Se debe asegurar previamente que existe la cantidad necesaria de bloques para escribir los datos a agregar
void escribirEnFinDeArchivoBis(t_FCB* pokemone,char* datos){
	int tamanioPokemon = pokemone->size;
	int posDeBloque = bloqueAfectado(tamanioPokemon);
	int cantidadEscrita = 0;
//printf("Datos %s contenidoDeFCB nombre %s tam %d bloques %s",datos,pokemone->pokemon,pokemone->size,pokemone->bloques);
	char** bloquesSeparados = string_get_string_as_array(pokemone->bloques);

	if(espacioLibreDelUltimoBloqueBis(pokemone) != 0){

		cantidadEscrita = escribirBloquePosicionandoPuntero(bloquesSeparados[posDeBloque],datos,desplazamientoEnBloque(tamanioPokemon));

		editarTamanioPokemonBis(pokemone,cantidadEscrita);
		posDeBloque++;
	}

	int i = 0;
	char* stringParaEscribir = string_substring_from(datos,cantidadEscrita);

	while(strlen(stringParaEscribir) != 0){

		cantidadEscrita = escribirBloquePosicionandoPuntero(bloquesSeparados[posDeBloque],stringParaEscribir,0);
		editarTamanioPokemonBis(pokemone,cantidadEscrita);
		stringParaEscribir = string_substring_from(stringParaEscribir,cantidadEscrita);
		i++;
	}


	limpiarPunteroAPuntero(bloquesSeparados);
	free(stringParaEscribir);
}

// Agrega la cantidad a size. Si es negativo el resta
void editarTamanioPokemonBis(t_FCB* pokemone,int cantidad){

	pokemone->size += cantidad;

	if(pokemone->size < 0) pokemone->size = 0;
}

// Se agrega una posicion al final del archivo, es decir en el ultimo bloque
int agregarNuevaPosicionBis(char* contenidoAagregar,t_FCB* metaPokemon){
	char** bloquesSeparados = string_get_string_as_array(metaPokemon->bloques);

	int ultimaPosicion = length_punteroAPuntero(bloquesSeparados);

	if(ultimaPosicion == 0){
//		printf("No tengo bloques asignados\n");
			int cantidadNecesaria = bloquesNecesariosParaEscribir(contenidoAagregar);

			if(solicitarBloquesParaPokemonBis(metaPokemon,cantidadNecesaria) == -1){
				log_error(loggerObligatorio,"No tengo espacio en disco");
				free(bloquesSeparados);
				return -1;
			}

			escribirEnFinDeArchivoBis(metaPokemon,contenidoAagregar);
			free(bloquesSeparados);
			return 0;
	}
	char* ultimaEscritura = string_from_format("%s",contenidoAagregar);
	char* ultimoBloque = bloquesSeparados[ultimaPosicion-1];

	int espacioLibre = espacioLibreDeBloqueBis(ultimoBloque,metaPokemon);
//	printf("Espacio libre: %d \n",espacioLibre);

	int bloquesNecesarios;

	if(strlen(ultimaEscritura) > espacioLibre){

	char* stringAcotado = string_substring_from(ultimaEscritura,espacioLibre);

	bloquesNecesarios = bloquesNecesariosParaEscribir(stringAcotado);
	free(stringAcotado);
	}else{
		bloquesNecesarios = 0;
	}


	if(solicitarBloquesParaPokemonBis(metaPokemon,bloquesNecesarios) == -1) {
		log_error(loggerObligatorio,"No tengo espacio en disco");
		free(ultimaEscritura);
		limpiarPunteroAPuntero(bloquesSeparados);
		return -1;
	}

	escribirEnFinDeArchivoBis(metaPokemon,ultimaEscritura);
	free(ultimaEscritura);
	//printf("Escrito\n");
	limpiarPunteroAPuntero(bloquesSeparados);
	//printf("sss\n");
	return 0;
}


// Recibe el desplazamiento en el que empezó a diferir la posicion antigua con la actual
// , el nuevo string de posiciones y el nombre de pokemon
//
//	Se debe asegurar que los bloques sean previamente asignados.
//
void actualizacionDeBloques(int desplazamientoDeCambio,char* posiciones,t_FCB* pokimane){
	int posicionBloque = bloqueAfectado(desplazamientoDeCambio);

	char** bloques = string_get_string_as_array(pokimane->bloques);

	char* bloqueAfectado = bloques[posicionBloque];
	int desplazamiento = desplazamientoEnBloque(desplazamientoDeCambio);
	char* posicionesAux = string_duplicate(posiciones);
	char* stringParaEscribir = string_substring_from(posicionesAux,desplazamientoDeCambio);

	/////	Deberia crear una funcion para verificar si existe la cantidad necesaria de bloques antes
	/// de empezar a escribir para que no haya inconsistencia de datos

	int bytesEscritos = escribirBloquePosicionandoPuntero(bloqueAfectado,stringParaEscribir,desplazamiento);
	// Y ANTES QUE ACÄ
	stringParaEscribir = string_substring_from(stringParaEscribir,bytesEscritos);

	posicionBloque++;

	while(bloques[posicionBloque] != NULL){
		bytesEscritos = escribirBloquePosicionandoPuntero(bloques[posicionBloque],stringParaEscribir,0);

		stringParaEscribir = string_substring_from(stringParaEscribir,bytesEscritos);

		posicionBloque++;
	}

	//free(bloquesArray);
	limpiarPunteroAPuntero(bloques);
	free(stringParaEscribir);
	free(posicionesAux);
}

// Funcion que se encarga de añadir la cantidad al pokemon
//	Recibe el string que contiene todas las posiciones(all del contenido del archivo), la posicion que se busca modificar
//	la cantidad que se desea sumar en esa posicion, el string que contiene los bloques y un nombre pokemon
// retorna 0 si sucedio t0do con exito. -1 si falló (No hay espacio en disco)
int anadirCantidad(char* posiciones,char* posicionBuscada,int cantidadASumar,t_FCB* pokemone){

	char** bloques = string_get_string_as_array(pokemone->bloques);
	// Separo cada linea por \n
	char** posicionesSeparadas = string_split(posiciones,"\n");

	// Obtengo el numero de linea que contiene esa posicon
	int posContenida = obtenerLinea(posicionesSeparadas,posicionBuscada);


	// sumo la cantidad con esta funcion. Me retorna la linea actualizada.
	char* nuevaLineaPokemon = sumarCantidadPokemon(posicionesSeparadas[posContenida],cantidadASumar);
	//printf("La nueva linea pokemon a  agregar es: %s\n",nuevaLineaPokemon);


	// Sumo 1 porque la posicion antigua no cuenta el \n
	int tamanioAntiguo = strlen(posicionesSeparadas[posContenida]) + 1;
	int tamanioNuevo = strlen(nuevaLineaPokemon);

// Verifico si me ocupa lo mismo. Si ocupa lo mismo entonces no hace falta reescribir todos. SIno solo cambiar en el byte
	// que hubo el cambio
	if(tamanioAntiguo == tamanioNuevo){

		int primeraPosicionCantPoke = desplazamientoDelArrayHastaCantPokemon(posicionesSeparadas,posicionBuscada);
		int posicionBloque = bloqueAfectado(primeraPosicionCantPoke);
		char* bloqueAfectado = bloques[posicionBloque];
		char* cantidadEnString = string_itoa(obtenerCantidad(nuevaLineaPokemon));
		sobreescribirUnCaracter(bloqueAfectado,desplazamientoEnBloque(primeraPosicionCantPoke),cantidadEnString);
		limpiarPunteroAPuntero(bloques);
		limpiarPunteroAPuntero(posicionesSeparadas);
		free(nuevaLineaPokemon);
		free(cantidadEnString);
		return 0;
	}

	// Sino debo reescribir a partir del bloque necesitado.
	int cantidadDeBytesQueCrece = tamanioNuevo - tamanioAntiguo;

	char* posicionActualizada = actualizarPosicion(posicionesSeparadas,nuevaLineaPokemon,posContenida);
	log_info(loggerObligatorio,"Hay que aumentar en bytes %d ",cantidadDeBytesQueCrece);
	int cantidadDeBloques = length_punteroAPuntero(bloques);
	int bloquesNecesarios = bloquesNecesariosParaEscribir(posicionActualizada);

	if(bloquesNecesarios > cantidadDeBloques) {
		int resultado = solicitarBloquesParaPokemonBis(pokemone,bloquesNecesarios - cantidadDeBloques);
		//printf("No tengo espacio en disco\n");
		//return -1;

		if(resultado == -1){
			limpiarPunteroAPuntero(bloques);
			limpiarPunteroAPuntero(posicionesSeparadas);
			free(nuevaLineaPokemon);
			return -1;
		}
	}

	//////////////////

	int primeraPosicionCantPoke = desplazamientoDelArrayHastaCantPokemon(posicionesSeparadas,posicionBuscada);

	log_info(loggerObligatorio,"Preparando archivo para su edicion");
	actualizacionDeBloques(primeraPosicionCantPoke,posicionActualizada,pokemone);
	editarTamanioPokemonBis(pokemone,cantidadDeBytesQueCrece);
	limpiarPunteroAPuntero(bloques);
	limpiarPunteroAPuntero(posicionesSeparadas);
	free(nuevaLineaPokemon);
	free(posicionActualizada);

	return 0;
}

void truncarUltimoBloque(t_FCB *pokimane){

	int offset = desplazamientoEnBloque(pokimane->size);

	char** bloques = string_get_string_as_array(pokimane->bloques);
	int ultimaPosicion = length_punteroAPuntero(bloques);

	char* rutaArchi = crearPathDeBloque(bloques[ultimaPosicion-1]);

	FILE* archi = fopen(rutaArchi,"rw+");
	int fd = fileno(archi);

	ftruncate(fd,offset);

	fclose(archi);
	free(rutaArchi);

	limpiarPunteroAPuntero(bloques);
}

// Funcion que se encarga de disminuir la cantidad de pokemon
// Recibe string uqe contiene todas las posiciones. Si contiene un 1 en su cantidad, se elimina la linea
void disminuirCantidad(char* posiciones,char* posicionBuscada,t_FCB *pokimane){
	char** bloques = string_get_string_as_array(pokimane->bloques);
		// Separo cada linea por \n
		char** posicionesSeparadas = string_split(posiciones,"\n");

		// Obtengo el numero de linea que contiene esa posicon
		int posContenida = obtenerLinea(posicionesSeparadas,posicionBuscada);


		// sumo la cantidad con esta funcion. Me retorna la linea actualizada.

		int cantidadDePokemones = obtenerCantidad(posicionesSeparadas[posContenida]);
		char* nuevaLineaPokemon;

		if(cantidadDePokemones <= 1){
			//log_info(loggerObligatorio,"Hay que eliminar la linea");
			nuevaLineaPokemon = string_new();
			string_append(&nuevaLineaPokemon,"");
		} else{//
		nuevaLineaPokemon = sumarCantidadPokemon(posicionesSeparadas[posContenida],-1);
		//printf("La nueva linea pokemon a  agregar es: %s\n",nuevaLineaPokemon);
		}

			// Hay que sumar 1 porque en posiciones Separadas se separa por el \n y no cuenta en el strlen
		int tamanioAntiguo = strlen(posicionesSeparadas[posContenida]) + 1;
		int tamanioNuevo = strlen(nuevaLineaPokemon);

	// Verifico si me ocupa lo mismo. Si ocupa lo mismo entonces no hace falta reescribir todos. SIno solo cambiar en el byte
		// que hubo el cambio
		if(tamanioAntiguo == tamanioNuevo){

			int primeraPosicionCantPoke = desplazamientoDelArrayHastaCantPokemon(posicionesSeparadas,posicionBuscada);
			int posicionBloque = bloqueAfectado(primeraPosicionCantPoke);
			char* bloqueAfectado = bloques[posicionBloque];
			char* cantidadEnString = string_itoa(obtenerCantidad(nuevaLineaPokemon));
			sobreescribirUnCaracter(bloqueAfectado,desplazamientoEnBloque(primeraPosicionCantPoke),cantidadEnString);
			limpiarPunteroAPuntero(bloques);
			limpiarPunteroAPuntero(posicionesSeparadas);
			free(cantidadEnString);
			free(nuevaLineaPokemon);
			return;
		}

		// Sino debo reescribir a partir del bloque necesitado.
		int cantidadDeBytesQueDisminuye = tamanioAntiguo -tamanioNuevo;
		log_info(loggerObligatorio,"El archivo debe disminuir en %d bytes",cantidadDeBytesQueDisminuye);

		char* posicionActualizada = actualizarPosicion(posicionesSeparadas,nuevaLineaPokemon,posContenida);

		//printf("posicion actualziada: %s \n",posicionActualizada);
		int cantidadDeBloques = length_punteroAPuntero(bloques);
		int bloquesNecesarios = bloquesNecesariosParaEscribir(posicionActualizada);

		// Disminuye
		editarTamanioPokemonBis(pokimane,(-1)*cantidadDeBytesQueDisminuye);
		if(bloquesNecesarios < cantidadDeBloques) {
			//printf("Entro a liberar\n");
			liberarBloquesParaPokemon(pokimane,cantidadDeBloques - bloquesNecesarios);
			if(bloquesNecesarios == 0){
				limpiarPunteroAPuntero(bloques);
				limpiarPunteroAPuntero(posicionesSeparadas);
				free(nuevaLineaPokemon);
				free(posicionActualizada);
				return;
			}
		}

		////////////////// REESCRIBE SIN LA LINEA

		int primeraPosicionLinea = desplazamientoDelArrayHastaLineaPosicion(posicionesSeparadas,posicionBuscada);

		log_info(loggerObligatorio,"Preparando archivo para su edicion.");
		actualizacionDeBloques(primeraPosicionLinea,posicionActualizada,pokimane);
		truncarUltimoBloque(pokimane);

		limpiarPunteroAPuntero(bloques);
		limpiarPunteroAPuntero(posicionesSeparadas);
		free(nuevaLineaPokemon);
		free(posicionActualizada);
		return;
}

// Retorna 1 si hubo éxito en la apertura del archivo
// 0 si no pudo abrirlo.
int verificarYAbrirArchivo(char* pokemon){
	// Se pregunta no está abierto

	sem_wait(&aperturaDeArchivo);
	int resultado;
	if(!archivoAbierto(pokemon)){
		// Si no está abierto lo abre
		abrirArchivo(pokemon);
		printf("\033[0;33m");
		log_info(loggerObligatorio,"Archivo %s abierto.",pokemon);
		printf("\033[0m");
		resultado = 1;
	}else resultado = 0;

	sem_post(&aperturaDeArchivo);
	return resultado;

}
//todo
t_FCB *cargarContenidoDelMetadata(char* nombrePokemon){
	sem_wait(&aperturaDeArchivo);
	t_config* config_pokemon = obtenerFCB(nombrePokemon);
	t_FCB* contenidoDelMetadata = malloc(sizeof(t_FCB));

	char* bloques = config_get_string_value(config_pokemon,"BLOCKS");


	contenidoDelMetadata->bloques = string_duplicate(bloques);
	contenidoDelMetadata->size = config_get_int_value(config_pokemon,"SIZE");
	contenidoDelMetadata->pokemon = string_duplicate(nombrePokemon);

	config_destroy(config_pokemon);

	sem_post(&aperturaDeArchivo);
	//printf("Bloqueees contenidoDePokemon %s\n",contenidoDelMetadata->bloques);
	return contenidoDelMetadata;
}

void liberarDatosFCB(t_FCB* unFCB){

	free(unFCB->bloques);
	free(unFCB->pokemon);
	free(unFCB);
}

// FINALIZADO
int procedimientoNEW(char* pokemon,uint32_t posx,uint32_t posy,uint32_t cantidad){
	int resultado;

	sem_wait(&aperturaDeArchivo);
	if(!existePokemon(pokemon)){

		printf("\033[0;31m");
		log_info(loggerObligatorio,"El pokemon %s no existe. Procediendo a crearlo",pokemon);
		printf("\033[0m");

		crearPokemon(pokemon);

	}else {

		if(esDirectorioPokemon(pokemon)){
			log_error(loggerObligatorio,"El nombre solicitado pertenece a un directorio con el nombre identico a %s",pokemon);
			sem_post(&aperturaDeArchivo);
			return -2;
		}

		log_info(loggerObligatorio,"Existe el pokemon %s",pokemon);

	}
	sem_post(&aperturaDeArchivo);

	if(verificarYAbrirArchivo(pokemon) == 0){
		log_error(loggerObligatorio,"El archivo %s no se puede abrir",pokemon);

		// REINTENTAR EN X TIEMPO

		struct arg_estructura argumentos;
		argumentos.nombrePokemon = pokemon;
		argumentos.tiempo = tiempo_de_reintento_operacion;

		pthread_t hiloDelReintento;
		pthread_create(&hiloDelReintento,NULL,&reintentoAbrir,(void*)&argumentos);

		pthread_join(hiloDelReintento,NULL);
	}


	t_FCB* contenidoDePokemon = cargarContenidoDelMetadata(pokemon);

	//printf("Bloqueees contenidoDePokemon %s\n",contenidoDePokemon->bloques);
	//char* bloques = obtenerBloquesDelPokemon(pokemon);

	char* posiciones = obtenerContenidoDeArchivo(contenidoDePokemon->bloques);
	char* posXEnString = string_itoa(posx);
	char* posYEnString = string_itoa(posy);
	char* posicionBuscada = string_from_format("%s-%s",posXEnString,posYEnString);
	free(posXEnString);
	free(posYEnString);

	if(!contieneEstaPosicion(posiciones,posicionBuscada)){
		// agregar al final del archivos
		log_error(loggerObligatorio,"No contengo la posicion %s..Procediendo a agregar",posicionBuscada);
		char* cantidadEnString = string_itoa(cantidad);
		char* posicionYCantidad = string_from_format("%s=%s\n",posicionBuscada,cantidadEnString);

		resultado = agregarNuevaPosicionBis(posicionYCantidad,contenidoDePokemon);

		free(posicionYCantidad);
		free(cantidadEnString);

	}else {
		log_info(loggerObligatorio,"Contengo la posicion");

		resultado = anadirCantidad(posiciones,posicionBuscada,cantidad,contenidoDePokemon);

	}

	printf("\033[1;34m");
	log_info(loggerObligatorio,"Aplicando %d segundos de retardo",tiempo_de_retardo);
	printf("\033[0m");
	sleep(tiempo_de_retardo);

	cerrarArchivoBis(contenidoDePokemon);

	liberarDatosFCB(contenidoDePokemon);
	free(posicionBuscada);
	free(posiciones);
	//free(bloques);
	return resultado;
}

void poseeBloquesONoExistePosicion(char* nombreDePokemon,char* posicionBuscada, char* bloques){
	if(strlen(bloques) <= 2){
		log_error(loggerObligatorio,"El archivo %s no posee bloques",nombreDePokemon);
	}else log_error(loggerObligatorio,"No existe la posicion %s del pokemon %s",posicionBuscada,nombreDePokemon);
}

// EN FASE DE PRUEBA
int procedimientoCATCH(char* pokemon,uint32_t posx,uint32_t posy){
	int resultado;

	sem_wait(&aperturaDeArchivo);
	if(!existePokemon(pokemon)){
		log_error(loggerObligatorio,"No existe el pokemon %s",pokemon);
		sem_post(&aperturaDeArchivo);
		return -1;
	}else {

		if(esDirectorioPokemon(pokemon)){
			log_error(loggerObligatorio,"El nombre solicitado pertenece a un directorio con el nombre identico a %s",pokemon);
			sem_post(&aperturaDeArchivo);
			return -2;
		}

		log_info(loggerObligatorio,"Existe el pokemon %s",pokemon);

	}
	sem_post(&aperturaDeArchivo);



	if(verificarYAbrirArchivo(pokemon) == 0){
		log_error(loggerObligatorio,"El archivo %s no se puede abrir",pokemon);

		// MATAR HILO
		// REINTENTAR EN X TIEMPO

		struct arg_estructura argumentos;
		argumentos.nombrePokemon = pokemon;
		argumentos.tiempo = tiempo_de_reintento_operacion;

		pthread_t hiloDelReintento;
		pthread_create(&hiloDelReintento,NULL,&reintentoAbrir,(void*)&argumentos);

		pthread_join(hiloDelReintento,NULL);
	}

	t_FCB* contenidoDePokemon = cargarContenidoDelMetadata(pokemon);

	char* posiciones = obtenerContenidoDeArchivo(contenidoDePokemon->bloques);
	char* posXEnString = string_itoa(posx);
	char* posYEnString = string_itoa(posy);
	char* posicionBuscada = string_from_format("%s-%s",posXEnString,posYEnString);
	free(posXEnString);
	free(posYEnString);

	if(!contieneEstaPosicion(posiciones,posicionBuscada)){

		poseeBloquesONoExistePosicion(pokemon,posicionBuscada,contenidoDePokemon->bloques);

		resultado = -1;
	}else{
		log_info(loggerObligatorio,"Contengo esa posicion");
		disminuirCantidad(posiciones,posicionBuscada,contenidoDePokemon);
		resultado = 0;
	}


		printf("\033[1;34m");
		log_info(loggerObligatorio,"Aplicando %d segundos de retardo",tiempo_de_retardo);
		printf("\033[0m");
		sleep(tiempo_de_retardo);

		cerrarArchivoBis(contenidoDePokemon);

		free(posicionBuscada);
		free(posiciones);
		liberarDatosFCB(contenidoDePokemon);
		//free(bloques);

		return resultado;
}

int procedimientoGET(uint32_t idMensaje,char* pokemon){

	void destruirCoordenadas(char *coordenadas){
		free(coordenadas);
	}


	int resultado;

	sem_wait(&aperturaDeArchivo);
	if(existePokemon(pokemon)){

		if(esDirectorioPokemon(pokemon)){
			log_error(loggerObligatorio,"El nombre solicitado pertenece a un directorio con el nombre identico a %s",pokemon);
			sem_post(&aperturaDeArchivo);
			return -2;
		}

		sem_post(&aperturaDeArchivo);
		log_info(loggerObligatorio,"Existe el pokemon %s",pokemon);


	if(verificarYAbrirArchivo(pokemon) == 0){
		log_error(loggerObligatorio,"Archivo no se puede abrir");

		// MATAR HILO
		// REINTENTAR EN X TIEMPO

		struct arg_estructura argumentos;
		argumentos.nombrePokemon = pokemon;
		argumentos.tiempo = tiempo_de_reintento_operacion;

		pthread_t hiloDelReintento;
		pthread_create(&hiloDelReintento,NULL,&reintentoAbrir,(void*)&argumentos);

		pthread_join(hiloDelReintento,NULL);
		}
		//log_info(loggerObligatorio,"Se puede abrir el archivo");
		//abrirArchivo(pokemon);

		t_FCB* contenidoDePokemon = cargarContenidoDelMetadata(pokemon);

		if(strlen(contenidoDePokemon->bloques) > 2){
		char* arrayDeArchivo = obtenerContenidoDeArchivo(contenidoDePokemon->bloques);
		char** posicionesConCantidad = string_split(arrayDeArchivo,"\n");

		int longitud=0;
		uint32_t cantidadParesCoordenadas = length_punteroAPuntero(posicionesConCantidad);

		for(int i=0;i<cantidadParesCoordenadas-1;i++){
			longitud = longitud + strlen(posicionesConCantidad[i])+1;


		}
		char** posicionesSeparadas = malloc(longitud);

		for(int j=0;j<cantidadParesCoordenadas;j++){
			posicionesSeparadas[j] = getToken(posicionesConCantidad[j],'=');
		}

		t_list* coordenadasSeparadas = guardarCoordenadas(posicionesSeparadas,cantidadParesCoordenadas);

		uint32_t coordenadasAEnviar[cantidadParesCoordenadas*2];
		insertarCoordenadas(coordenadasSeparadas,coordenadasAEnviar);

		printf("\033[1;34m");
		log_info(loggerObligatorio,"Aplicando %d segundos de retardo",tiempo_de_retardo);
		printf("\033[0m");
		sleep(tiempo_de_retardo);

		cerrarArchivoBis(contenidoDePokemon);
		resultado = envioDeMensajeLocalized(pokemon,idMensaje,cantidadParesCoordenadas,coordenadasAEnviar);

		free(arrayDeArchivo);
		//free(bloques);
		list_destroy_and_destroy_elements(coordenadasSeparadas,(void *) destruirCoordenadas);


		limpiarPunteroAPuntero(posicionesConCantidad);

		//////// ULTIMO TEST
		limpiarPunteroAPuntero2punto0(posicionesSeparadas,cantidadParesCoordenadas);
		free(posicionesSeparadas);
		liberarDatosFCB(contenidoDePokemon);
		//////// ULTIMO TEST

		return resultado;

	}else{


		log_error(loggerObligatorio,"El pokemon %s no posee bloques asignados",pokemon);

		printf("\033[1;34m");
		log_info(loggerObligatorio,"Aplicando %d segundos de retardo",tiempo_de_retardo);
		printf("\033[0m");
		sleep(tiempo_de_retardo);

		cerrarArchivoBis(contenidoDePokemon);
	}
	}else{
		sem_post(&aperturaDeArchivo);
		log_error(loggerObligatorio,"No existe el pokemon %s",pokemon);
	}


		uint32_t coordenadasVacias[] = {};

		resultado = envioDeMensajeLocalized(pokemon,idMensaje,0,coordenadasVacias);


	return resultado;
}

char *getToken(char *unString,char delimitador){
		    int length = strlen(unString) + 1;
		    char *stringARetornar = malloc(length);
		    int i =0;
		    while(unString[i] != delimitador){
		      stringARetornar[i] = unString[i];
		      i++;
		    }
		    stringARetornar[i] = '\0';
		    return stringARetornar;
		}

char *getCoordenadaX(char *coordenada){
    int i = 0;
    while(coordenada[i] != '-'){
            i++;
    }
    char *coordenadaX = malloc(i);

    for(int j = 0; j <= i; j++){
        coordenadaX[j] = coordenada[j];
    }
    coordenadaX[i] = '\0';
    return coordenadaX;
}

char *getCoordenadaY(char *coordenada){

    int i = 0;
    while(coordenada[i] != '-'){
            i++;
    }

    int j = 0;
    int k = i+1;

    for(int l = k; l < strlen(coordenada);l++){
            j++;
    }

    char *coordenadaY = malloc(j);

    j = 0;

    for(int g = k; g < strlen(coordenada);g++){
        coordenadaY[j] = coordenada[g];
        j++;
    }

    coordenadaY[j] = '\0';

    return coordenadaY;
}



t_list* guardarCoordenadas(char** posiciones,int length){
	t_list* coordenadasx = list_create();
	for(int i = 0; i < length;i++){
	    list_add(coordenadasx,getCoordenadaX(posiciones[i]));
	}


	t_list* coordenadasy = list_create();

	for(int j = 0; j < length;j++){
		    list_add(coordenadasy,getCoordenadaY(posiciones[j]));



		}

	t_list* coordenadas = list_create();
	for(int k=0;k<list_size(coordenadasx);k++){
		char *coordx = list_get(coordenadasx,k);
		list_add(coordenadas,coordx);
		char *coordy = list_get(coordenadasy,k);
		list_add(coordenadas,coordy);

	}

	list_destroy(coordenadasx);
	list_destroy(coordenadasy);
	return coordenadas;


}



void insertarCoordenadas(t_list *lista,uint32_t* vector){
    for(int i = 0; i < list_size(lista);i++){
        char *elem = list_get(lista,i);
        uint32_t elemento = atoi(elem);
        vector[i] = elemento;
    }
}


