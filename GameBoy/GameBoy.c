/*
 * GameBoy.c
 *
 *  Created on: 9 may. 2020
 *      Author: utnso
 */

#include "GameBoy.h"

int main(int argc,char* argv[]){

	// argv[0] es el nombre de nuestro programa
	// argv[1] va a ser el nombre de proceso a ejecutar
	// argv[2] es el tipo de mensaje
	// En adelante va a ser los argumentos enviados
	char* nombreDeProceso = argv[1];
	char* tipoDeMensaje = obtenerNombreSinElPokemon(argv[2]);
	// transforma el nombre del proceso en numeros
	int hashDeProceso = convertir_nombre(nombreDeProceso);

	iniciar_logger();
	iniciar_logger_obligatorio();
	leer_config();

	switch(hashDeProceso){


	case BROKER:
				printf("Broker Omegalul \n");		//---------------------- QUITAR
				if(esUnTipoDeMensajeValido(nombreDeProceso,tipoDeMensaje)){

								conexion = crear_conexion(IP_BROKER,PUERTO_BROKER,nombreDeProceso);
								enviar_mensaje_a_broker(tipoDeMensaje,argc,argv);
							} else log_error(logger,"No está definido el tipo de mensaje %s para %s",tipoDeMensaje,nombreDeProceso);		//---------------------- QUITAR
				break;

	case TEAM:
				printf("Team omegalul \n");
				if(esUnTipoDeMensajeValido(nombreDeProceso,tipoDeMensaje)){
								conexion = crear_conexion(IP_TEAM,PUERTO_TEAM,nombreDeProceso);
								enviar_mensaje_a_team(tipoDeMensaje,argc,argv);
							} else log_error(logger,"No está definido el tipo de mensaje %s para %s",tipoDeMensaje,nombreDeProceso);
				break;
	case GAMECARD:
				printf("Gamecard omegalul \n");
				if(esUnTipoDeMensajeValido(nombreDeProceso,tipoDeMensaje)){
								conexion = crear_conexion(IP_GAMECARD,PUERTO_GAMECARD,nombreDeProceso);
								enviar_mensaje_a_gamecard(tipoDeMensaje,argc,argv);

							} else log_error(logger,"No está definido el tipo de mensaje %s para %s",tipoDeMensaje,nombreDeProceso);
				break;

//./gameboy SUSCRIPTOR [COLA_DE_MENSAJES] [TIEMPO]
	case SUSCRIPTOR:
				printf("Suscriptor omegalul \n");
				if(esUnTipoDeMensajeValido(nombreDeProceso,tipoDeMensaje)){
								conexion = crear_conexion(IP_BROKER,PUERTO_BROKER,"BROKER");
								if(conexion != -1){
								int tiempo = atoi(argv[3]);
								// Hay que ver como enviar el nombre del mensaje.. se necesita una funcion que transforme nuestra
								// variable colaDeMensaje en un t_operacion
								// Tambien hay que ver como hacer la desconexion en tiempo x
								//packAndSend_Handshake(conexion, "GAMEBOY" , colaDeMensaje);
								conectarmeACola(conexion,tiempo,tipoDeMensaje);

								if(estadoDeConexion == -1){
									log_info(logger,"La conexión finalizó. Razon: Broker se cayó");
								}

								log_info(logger,"La cola solicitada para suscribirse fue : %s",tipoDeMensaje);
								log_info(logger,"El tiempo solicitado fue: %i",tiempo);

								}
							} else log_error(logger,"No está definido el tipo de mensaje %s para %s",tipoDeMensaje,nombreDeProceso);

				break;

			default: log_error(logger,"Ese proceso no existe macho");
			 break;

	}


	terminar_programa();
	free(tipoDeMensaje);
	free(identificador);
	/*
		//enviar mensajea
		//enviar_mensaje("Vamos Boca",conexion);
		//recibir mensaje
		//char *mensaje = recibir_mensaje(conexion);
		//loguear mensaje recibido
		//log_info(logger,mensaje);

*/
	return 0;
}


// Recibe un string. Transforma cada letra/digito del string en un int basado en su expresion ASCII
// retorna la suma de todos los digitos
int convertir_nombre(char* nombreDeProceso){
	int posicion = strlen(nombreDeProceso);
	int nombreHasheado = 0;
	int i;
	for(i = 0; i <= posicion;i++){
		nombreHasheado += toascii(nombreDeProceso[i]);
	}
	return nombreHasheado;
}

// recibe el nombre de proceso y su tipo de mensaje
//	dice si ese tipo de mensaje es valido para ese proceso
//
//
bool esUnTipoDeMensajeValido(char* nombreDeProceso, char* tipo_de_mensaje){
// Crea un numero basado en el string de tipo de mensaje
	int hashDeMensaje = convertir_nombre(tipo_de_mensaje);
	int valor = 0;
// Como lo extraido es de tipo char lo convertimos a string para poder utilizar la funcion string_contains
	char* primerLetraDeProceso = string_from_format("%c",nombreDeProceso[0]);

	// Aca viene lo turbio
	// Que significa "GBS"
	// Son las iniciales del nombre de proceso
	//	G: Gamecard
	//	B: Broker
	//  S: Suscriptor
	//	T: Team
	// Lo que hace es utilizar la primer letra del nombre del proceso para verificar
	// si coincide con su tipo de mensaje que se puede utilizar en ese proceso
	//
	//	por ejemplo: desde el gameboy el mensaje CATCH puede ser enviado a Gamecard,Broker y el modo suscriptor
	//  Entonces seria:  "GBS" sus iniciales para comprobar
	switch(hashDeMensaje){
		case NEW:
			if(string_contains("GBS",primerLetraDeProceso)) valor = 1;
		break;

		case LOCALIZED:if(string_contains("S",primerLetraDeProceso)) valor = 1;
		break;

		case GET:if(string_contains("GBS",primerLetraDeProceso)) valor = 1;
		break;

		case APPEARED:if(string_contains("BTS",primerLetraDeProceso)) valor = 1;
		break;

		case CATCH:if(string_contains("GBS",primerLetraDeProceso)) valor = 1;
		break;

		case CAUGHT:if(string_contains("BS",primerLetraDeProceso))valor = 1;
		break;

		default:
		break;
		}
	free(primerLetraDeProceso);
	return valor;
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}

int crear_conexion(char *ip, char* puerto, char* nombreDeProceso)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1){
		log_error(loggerObligatorio,"No se pudo conectar al proceso solicitado");
		freeaddrinfo(server_info);
		close(socket_cliente);
		return -1;
	}

	else log_info(loggerObligatorio,"Hubo conexion con el %s ",nombreDeProceso);
	freeaddrinfo(server_info);

	return socket_cliente;
}

void iniciar_logger(void)
{
	char* path = "/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/GameBoy/GameBoy.log";
	char* nombreArch = "GameBoy";
	logger = log_create(path,nombreArch,true,LOG_LEVEL_INFO);
}

void iniciar_logger_obligatorio(void)
{
	char* path = "/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/GameBoy/GameBoyObligatorio.log";
	char* nombreArch = "Log_Gameboy";
	loggerObligatorio = log_create(path,nombreArch,true,LOG_LEVEL_INFO);
}

void leer_config(void){
	config = config_create("/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/GameBoy/GameBoy.config");
	if (config == NULL){
		log_error(logger,"No existe el archivo de configuracion");
		}

	cargarConfig(config);
	log_info(logger,"Configuracion cargada con exito");
}

void cargarConfig(t_config* configuracion){
	IP_BROKER = config_get_string_value(configuracion,"IP_BROKER");
	PUERTO_BROKER = config_get_string_value(configuracion,"PUERTO_BROKER");
	IP_TEAM = config_get_string_value(configuracion,"IP_TEAM");
	PUERTO_TEAM = config_get_string_value(configuracion,"PUERTO_TEAM");
	IP_GAMECARD = config_get_string_value(configuracion,"IP_GAMECARD");
	PUERTO_GAMECARD = config_get_string_value(configuracion,"PUERTO_GAMECARD");

}

void recibirACK(int conexion, uint32_t id){

	Header headerRecibido = receiveHeader(conexion);
	uint32_t tamanio = headerRecibido.tamanioMensaje;

	void *paquete = receiveAndUnpack(conexion,tamanio);
	uint32_t idDeAck = unpackID(paquete);

	if(id == idDeAck){
		log_info(logger,"El proceso recibió con éxito el mensaje con ID %d",id);
	}else{
		log_info(logger,"El proceso no recibió el mensaje con ID %d",id);
	}

	free(paquete);
}

void terminar_programa()
{
	log_destroy(logger);
	config_destroy(config);
	log_destroy(loggerObligatorio);
	liberar_conexion(conexion);

}

void envioDeMensajeNew(char* pokemon, uint32_t posx, uint32_t posy,uint32_t cantidad,uint32_t idmensaje){

	void* paqueteNew = pack_New(idmensaje,pokemon,cantidad,posx,posy);
	uint32_t tamPaquete =  strlen(pokemon) + 5*sizeof(uint32_t);
	int resultado = packAndSend(conexion,paqueteNew,tamPaquete,t_NEW);
	if(resultado == -1)log_error(loggerObligatorio,"El envio del mensaje de NEW falló");
	log_info(loggerObligatorio,"Mensaje de NEW enviado");
	free(paqueteNew);
}

void *insertarIDEnPaquete(uint32_t ID,void *paquete,uint32_t tamanioPaquete){
	void *paqueteAEnviar = malloc(tamanioPaquete+sizeof(uint32_t));
	memcpy(paqueteAEnviar,&ID,sizeof(uint32_t));
	memcpy(paqueteAEnviar+sizeof(uint32_t),paquete,tamanioPaquete);

	return paqueteAEnviar;
}

void *paqueteAppearedTeam(char *pokemon,uint32_t posx,uint32_t posy,uint32_t idMensaje){
	void *paqueteAppeared = pack_Appeared(idMensaje,pokemon,posx,posy);
	return paqueteAppeared;
}

void envioDeMensajeAppeared(char* pokemon, uint32_t posx, uint32_t posy, uint32_t idmensaje){
	void* paqueteAppeared = pack_Appeared(idmensaje,pokemon,posx,posy);
	uint32_t tamPaquete =  strlen(pokemon) + 4*sizeof(uint32_t);
	log_error(loggerObligatorio,"El tamaño del paquete es: %i ",tamPaquete);
	int resultado = packAndSend(conexion,paqueteAppeared,tamPaquete,t_APPEARED);
  	// packAndSend_Appeared(conexion,-1,pokemon,posx,posy);
	if(resultado == -1)log_error(loggerObligatorio,"El envio del mensaje de APPEARED falló");
	log_info(loggerObligatorio,"Mensaje de APPEARED enviado");
	free(paqueteAppeared);
}

void envioDeMensajeCatch(char* pokemon, uint32_t posx, uint32_t posy, uint32_t idmensaje){
	void* paqueteCatch = pack_Catch(idmensaje,pokemon,posx,posy);
		uint32_t tamPaquete =  strlen(pokemon) + 4*sizeof(uint32_t);
		log_error(loggerObligatorio,"El tamaño del paquete es: %i ",tamPaquete);
		int resultado = packAndSend(conexion,paqueteCatch,tamPaquete,t_CATCH);
	  	// packAndSend_Appeared(conexion,-1,pokemon,posx,posy);
		if(resultado == -1)log_error(loggerObligatorio,"El envio del mensaje de CATCH falló");
		log_info(loggerObligatorio,"Mensaje de CATCH enviado");
		free(paqueteCatch);
}

void envioDeMensajeCaught(uint32_t atrapado, uint32_t idmensaje){
	void* paqueteCaught = pack_Caught(idmensaje,atrapado);
		uint32_t tamPaquete =  2*sizeof(uint32_t);
		log_error(loggerObligatorio,"El tamaño del paquete es: %i ",tamPaquete);
		int resultado = packAndSend(conexion,paqueteCaught,tamPaquete,t_CAUGHT);
	  	// packAndSend_Appeared(conexion,-1,pokemon,posx,posy);
		if(resultado == -1)log_error(loggerObligatorio,"El envio del mensaje de CAUGHT falló");
		log_info(loggerObligatorio,"Mensaje de CAUGHT enviado");
		free(paqueteCaught);
}

void envioDeMensajeGet(char* pokemon,uint32_t idmensaje){
	void* paqueteGet = pack_Get(idmensaje,pokemon);
			uint32_t tamPaquete = strlen(pokemon) + 2*sizeof(uint32_t);
			log_error(loggerObligatorio,"El tamaño del paquete es: %i ",tamPaquete);
			int resultado = packAndSend(conexion,paqueteGet,tamPaquete,t_GET);
		  	// packAndSend_Appeared(conexion,-1,pokemon,posx,posy);
			if(resultado == -1)log_error(loggerObligatorio,"El envio del mensaje de GET falló");
			log_info(loggerObligatorio,"Mensaje de GET enviado");
			free(paqueteGet);
}

void enviar_mensaje_a_broker(char* tipo_de_mensaje,int cantidad_de_argumentos,char* argumentos[]){


	int hashDeMensaje = convertir_nombre(tipo_de_mensaje);


		switch(hashDeMensaje){
		// 	./gameboy BROKER NEW_POKEMON [POKEMON] [POSX] [POSY] [CANTIDAD] [ID_MENSAJE]
			case NEW:
				if(cantidad_de_argumentos == 7){
						char* pokemon = argumentos[3];
						uint32_t posx = atoi(argumentos[4]);
						uint32_t posy = atoi(argumentos[5]);
						uint32_t cantidad = atoi(argumentos[6]);

						//Envio del mensaje
						envioDeMensajeNew(pokemon,posx,posy,cantidad,-1);
						//Envio del mensaje
						//recibirACK(conexion,-1);
						//printf("Aca envio el mensaje \n");
						//printf("Pokemon: %s \n",pokemon);
						//printf("PosX: %i \n",posx);
						//printf("PosY: %i \n",posy);
						//printf("Cantidad: %i \n",cantidad);
				}else printf("Faltan argumentos macho \n");
			break;

			//./gameboy BROKER APPEARED_POKEMON [POKEMON] [POSX] [POSY] [ID_MENSAJE]
			case APPEARED://if(string_contains("GBS",primerLetraDeProceso)) valor = 1;
				if(cantidad_de_argumentos == 7){
								char* pokemon = argumentos[3];
								uint32_t posx = atoi(argumentos[4]);
								uint32_t posy = atoi(argumentos[5]);
								uint32_t idmensaje = atoi(argumentos[6]);

								// Envio del mensaje
								envioDeMensajeAppeared(pokemon,posx,posy,idmensaje);
								// envio del mensaje
								//recibirACK(conexion,idmensaje);
								//printf("Aca envio el mensaje \n");
								//printf("Pokemon: %s \n",pokemon);
								//printf("PosX: %i \n",posx);
								//printf("PosY: %i \n",posy);
								//printf("id mensaje: %i \n",idmensaje);
								}else printf("Faltan argumentos macho \n");


			break;

			//./gameboy BROKER CATCH_POKEMON [POKEMON] [POSX] [POSY]
			case CATCH://if(string_contains("GBS",primerLetraDeProceso)) valor = 1;
				if(cantidad_de_argumentos == 6){
								char* pokemon = argumentos[3];
								uint32_t posx = atoi(argumentos[4]);
								uint32_t posy = atoi(argumentos[5]);
								uint32_t idmensaje = -1;

								// envio de mensaje
								envioDeMensajeCatch(pokemon,posx,posy,idmensaje);
								// envio de mensaje
								//recibirACK(conexion,idmensaje);
								//printf("Aca envio el mensaje \n");
								//printf("Pokemon: %s \n",pokemon);
								//printf("PosX: %i \n",posx);
								//printf("PosY: %i \n",posy);
								}else printf("Faltan argumentos macho \n");
			break;

			//./gameboy BROKER CAUGHT_POKEMON [ID_MENSAJE] [OK/FAIL]
			case CAUGHT://if(string_contains("GBS",primerLetraDeProceso)) valor = 1;
				if(cantidad_de_argumentos == 5){
								uint32_t idmensaje = atoi(argumentos[3]);
								char *resultado = argumentos[4];
								uint32_t atrapado;

								if(string_equals_ignore_case(resultado,"OK")){
									atrapado = 1;
								}
								else if(string_equals_ignore_case(resultado,"FAIL")){
									atrapado = 0;
								}

								// envio de mensaje
								envioDeMensajeCaught(atrapado,idmensaje);
								// envio de mensaje
								//recibirACK(conexion,idmensaje);
								//printf("Atrapado: %i \n",atrapado);
								//printf("id mensaje: %i \n",idmensaje);
								}else printf("Faltan argumentos macho \n");
			break;

			//./gameboy BROKER GET_POKEMON [POKEMON]
			case GET://if(string_contains("GBS",primerLetraDeProceso)) valor = 1;
				if(cantidad_de_argumentos == 4){
								char* pokemon = argumentos[3];
								uint32_t idmensaje = -1;

								// envio de mensaje
								envioDeMensajeGet(pokemon,idmensaje);
								// envio de mensaje
								//recibirACK(conexion,idmensaje);
								//printf("pokemon: %s \n",pokemon);
								}else printf("Faltan argumentos macho \n");
			break;

			default:
			break;
			}
}

void enviar_mensaje_a_team(char* tipo_de_mensaje,int cantidad_de_argumentos,char* argumentos[]){

	//int valor = 0;
	//./gameboy TEAM APPEARED_POKEMON [POKEMON] [POSX] [POSY]

	if(cantidad_de_argumentos == 6){
		char* pokemon = argumentos[3];
		uint32_t posx = atoi(argumentos[4]);
		uint32_t posy = atoi(argumentos[5]);
		uint32_t id = -1;

		void *paquete = paqueteAppearedTeam(pokemon,posx,posy,id);
		uint32_t tamPaquete = strlen(pokemon) + 4*sizeof(uint32_t);
		void *paqueteAEnviar = insertarIDEnPaquete(-1,paquete,tamPaquete);

		log_error(loggerObligatorio,"El tamaño del paquete es: %i ",tamPaquete+sizeof(uint32_t));
		int resultado = packAndSend(conexion,paqueteAEnviar,tamPaquete+sizeof(uint32_t),t_APPEARED);
		if(resultado == -1)log_error(loggerObligatorio,"El envio del mensaje de APPEARED falló");
		log_info(loggerObligatorio,"Mensaje de APPEARED enviado");
		//recibirACK(conexion,id);
		//printf("Aca envio el mensaje \n");
		//printf("Pokemon: %s \n",pokemon);
		//printf("PosX: %i \n",posx);
		//printf("PosY: %i \n",posy);
	}else printf("Faltan argumentos macho \n");
}

void enviar_mensaje_a_gamecard(char* tipo_de_mensaje,int cantidad_de_argumentos,char* argumentos[]){
	int hashDeMensaje = convertir_nombre(tipo_de_mensaje);

			switch(hashDeMensaje){
				// ./gameboy GAMECARD NEW_POKEMON [POKEMON] [POSX] [POSY] [CANTIDAD] [ID_MENSAJE]
				case NEW:
					if(cantidad_de_argumentos == 8){
							char* pokemon = argumentos[3];
							uint32_t posx = atoi(argumentos[4]);
							uint32_t posy = atoi(argumentos[5]);
							uint32_t cantidad = atoi(argumentos[6]);
							uint32_t idmensaje = atoi(argumentos[7]);

					    	// Envio del mensaje
							envioDeMensajeNew(pokemon,posx,posy,cantidad,idmensaje);
							// Envio del mensaje
							//recibirACK(conexion,idmensaje);
							//printf("Aca envio el mensaje \n");
							//printf("Pokemon: %s \n",pokemon);
							//printf("PosX: %i \n",posx);
							//printf("PosY: %i \n",posy);
							//printf("Cantidad: %i \n",cantidad);
							//printf("id mensaje: %i \n",idmensaje);
					}else printf("Faltan argumentos macho \n");
				break;
				// ./gameboy GAMECARD CATCH_POKEMON [POKEMON] [POSX] [POSY] [ID_MENSAJE]
				case CATCH:
					if(cantidad_de_argumentos == 7){
									char* pokemon = argumentos[3];
									uint32_t posx = atoi(argumentos[4]);
									uint32_t posy = atoi(argumentos[5]);
									uint32_t idmensaje = atoi(argumentos[6]);

							    	// Envio del mensaje
									envioDeMensajeCatch(pokemon,posx,posy,idmensaje);
									// Envio del mensaje
									//recibirACK(conexion,idmensaje);
									//printf("Aca envio el mensaje \n");
									//printf("Pokemon: %s \n",pokemon);
									//printf("PosX: %i \n",posx);
									//printf("PosY: %i \n",posy);
									//printf("id mensaje: %i \n",idmensaje);
									}else printf("Faltan argumentos macho \n");
				break;

				// ./gameboy GAMECARD GET_POKEMON [POKEMON]
				case GET:
					if(cantidad_de_argumentos == 4){
									char* pokemon = argumentos[3];
									printf("%s \n",pokemon);
									uint32_t id = -1;

							    	// Envio del mensaje
									envioDeMensajeGet(pokemon,id);
									// Envio del mensaje
									//recibirACK(conexion,id);
									//printf("pokemon: %s \n",pokemon);
									}else printf("Faltan argumentos macho \n");
				break;
				default:
				break;
				}


}


void limpiarPunteroToPuntero(char** puntero){
	int i = 0;

	while(puntero[i] != NULL){
		free(puntero[i]);
		i++;
	}
	free(puntero);
}

char* obtenerNombreSinElPokemon(char* proceso){
	char** nombres = string_split(proceso,"_");
	char* nombreSinPoke = string_duplicate(nombres[0]);
	limpiarPunteroToPuntero(nombres);
	return nombreSinPoke;
}



t_infoPackGB *crearInfoPaqueteDeGB(int socket,void *paquete,Header tipoDeOp){
	t_infoPackGB *mensaje = malloc(sizeof(t_infoPackGB));
	mensaje->socket = socket;
	mensaje->paquete = paquete;
	mensaje->headerRecibido = tipoDeOp;
	return mensaje;
}

void destruirInfoPaqueteGB(t_infoPackGB *infoPaquete){
	void *paquete = infoPaquete->paquete;
	free(paquete);
	free(infoPaquete);
}

void envioDelACK(uint32_t id, t_operacion operacion){
	int socket = crear_conexion(IP_BROKER, PUERTO_BROKER,"Broker");
	void* paquete = pack_Ack(id, operacion,identificador);
	uint32_t tamPaquete = sizeof(id) + sizeof(t_operacion) + strlen(identificador) + 1 + sizeof(uint32_t);
	packAndSend(socket, paquete, tamPaquete, t_ACK);
	log_info(logger, "El envio del ACK del mensaje con ID [%d] se realizo con exito",id);
	close(socket);
	free(paquete);
}


void recepcionDeMensaje(){
	pthread_t hiloRecepcion;
	log_info(logger,"Esperando mensajes.");
	while(tiempoRestante != 0){
	Header headerRecibido;
	headerRecibido = receiveHeader(conexion);
	 if(headerRecibido.operacion != -1 && headerRecibido.tamanioMensaje != 0){
		 uint32_t tamanio = headerRecibido.tamanioMensaje;
		 void* paquete = receiveAndUnpack(conexion, tamanio);
		 t_infoPackGB *infoPaquete = crearInfoPaqueteDeGB(conexion,paquete,headerRecibido);
		 pthread_create(&hiloRecepcion,NULL,(void *)discriminarMensaje,infoPaquete);
		 pthread_detach(hiloRecepcion);

	 }else{
		printf("Se cayó el broker \n");
		estadoDeConexion = -1;
		pthread_exit(NULL);
	 }
	}
}


void discriminarMensaje(t_infoPackGB *infoPack){

				uint32_t tipoDeOperacion = infoPack->headerRecibido.operacion;
				void* paquete = infoPack->paquete;
				uint32_t id = unpackID(paquete);

				//envioDelACK(id,tipoDeOperacion);
				switch (tipoDeOperacion) {
						case t_GET:;
							//log_info(logger,"Me llegaron mensajes de la cola Get");

							char* pokemonGet = unpackPokemonGet(paquete);
							//uint32_t tamanioPokemonGet = strlen(pokemonGet);

							log_info(logger,"Pokemon: %s id: %d",pokemonGet,id);
							free(pokemonGet);
							break;

						case t_CATCH:;
							//log_info(logger,"Me llegaron mensajes de la cola Catch");
							char* pokemonCatch = unpackPokemonCatch(paquete);
							uint32_t tamanioPokemoCatch = strlen(pokemonCatch);

							uint32_t posXCatch = unpackCoordenadaX_Catch(paquete, tamanioPokemoCatch);
							uint32_t posYCatch = unpackCoordenadaY_Catch(paquete, tamanioPokemoCatch);

							log_info(logger,"Pokemon: %s posX: %d posY: %d id: %d",pokemonCatch,posXCatch,posYCatch,id);
							free(pokemonCatch);
							break;
						case t_NEW:;
							//log_info(logger,"Me llegaron mensajes de la cola New");
							char* pokemonNew = unpackPokemonNew(paquete);
							uint32_t tamanioPokemonNew = strlen(pokemonNew);

							uint32_t posXNew = unpackCoordenadaX_New(paquete, tamanioPokemonNew);
							uint32_t posYNew = unpackCoordenadaY_New(paquete, tamanioPokemonNew);
							uint32_t cantPokemonNew = unpackCantidadPokemons_New(paquete, tamanioPokemonNew);

							log_info(logger,"Pokemon: %s posX: %d posY: %d cantidad: %d id: %d",pokemonNew,posXNew,posYNew,cantPokemonNew,id);
							free(pokemonNew);
							break;
						case t_APPEARED:;
							//log_info(logger,"Me llegaron mensajes de la cola APPEARED");
							char* pokemonAppeared = unpackPokemonAppeared(paquete);
							uint32_t tamanioPokemonAppeared = strlen(pokemonAppeared);

							uint32_t posXAppeared = unpackCoordenadaX_Appeared(paquete, tamanioPokemonAppeared);
							uint32_t posYAppeared = unpackCoordenadaY_Appeared(paquete, tamanioPokemonAppeared);

							log_info(logger,"Pokemon: %s posX: %d posY: %d id: %d",pokemonAppeared,posXAppeared,posYAppeared, id);
							free(pokemonAppeared);
							break;

						case t_CAUGHT:;
							//log_info(logger,"Me llegaron mensajes de la cola CAUGHT");

							int resultado = unpackResultado_Caught(paquete);

							log_info(logger,"Resultado Caught: %d id: %d",resultado, id);

							break;

						case t_LOCALIZED:;
							//log_info(logger,"Me llegaron mensajes de la cola LOCALIZED");
							//uint32_t tamanioPokemonLocalized = strlen(pokemonLocalized);
							char* pokemonLocalized = unpackPokemonLocalized(paquete);
							// EN FASE DE PRUEBA

							log_info(logger,"Pokemon: %s resultado Caught: %d id: %d",pokemonLocalized,id);
							free(pokemonLocalized);
							break;

						default:
							break;
						}

				destruirInfoPaqueteGB(infoPack);

}


// Temporizador que modifica una variable global llamada tiempoRestante para cortar el while de la recepcion de mensajes

void crearTimer(int *tiempo){
	int i;
	for(i = 0; i <= *tiempo; i++){
		if(estadoDeConexion == -1) pthread_exit(NULL);
		tiempoRestante = *tiempo - i;
		sleep(1);
	}
	pthread_exit(NULL);
}

// Se encarga de recibir los mensajes de la cola suscripta
void recepcionDeMensajes(int tiempo){

	pthread_t thread;
	pthread_t hiloTemporizador;
	tiempoRestante = tiempo;
	pthread_create(&hiloTemporizador,NULL,(void*)crearTimer,&tiempo);

	log_info(logger,"Tiempo restante: %d",tiempoRestante);

	if(pthread_create(&thread,NULL,(void*)recepcionDeMensaje,NULL) == 0){
	pthread_detach(thread);
	}

	pthread_join(hiloTemporizador,NULL);
	pthread_cancel(thread);
}

int enviarSuscripcion(int socket,char *identificador,t_operacion operacion){
	void *paquete = pack_Handshake(identificador,operacion);
	uint32_t tamPaquete = strlen(identificador) + 1 + sizeof(uint32_t) + sizeof(t_operacion);
	int resultado = packAndSend(socket,paquete,tamPaquete,t_HANDSHAKE);
	free(paquete);
	return resultado;
}

// Se conecta y suscribe a la cola de mensajes enviada por parametro y finaliza cuando su tiempo llega a 0

void conectarmeACola(int socket,int tiempo,char* colaDeMensaje){
	int hashDeCola = convertir_nombre(colaDeMensaje);
	t_operacion tipoDeMensaje;

	switch(hashDeCola){

		case NEW: tipoDeMensaje = t_NEW;
			break;
		case APPEARED: tipoDeMensaje = t_APPEARED;
			break;
		case CATCH: tipoDeMensaje = t_CATCH;
			break;
		case CAUGHT: tipoDeMensaje = t_CAUGHT;
			break;
		case GET: tipoDeMensaje = t_GET;
			break;
		case LOCALIZED: tipoDeMensaje = t_LOCALIZED;
			break;
	}

	// SUSCRIPCION A FUTURO.
	int resultado = enviarSuscripcion(conexion,"GameBoy",tipoDeMensaje);

	if(resultado == -1)log_error(loggerObligatorio,"La suscripcion a %s falló",colaDeMensaje);
	log_info(loggerObligatorio,"Suscripcion a %s enviada",colaDeMensaje);


	// RECIBIENDO ACK
	Header headerRecibido = receiveHeader(conexion);
	uint32_t tamanio = headerRecibido.tamanioMensaje;

// RESPUESTA DEL MENSAJE PARA ASEGURARME QUE ME SUSCRIBIÓ
	void *paqueteDeRespuesta = receiveAndUnpack(conexion,tamanio);
	identificador = unpackProceso(paqueteDeRespuesta);
	uint32_t sizeProceso = strlen(identificador) + 1;
	t_operacion operacionSuscripta = unpackOperacion(paqueteDeRespuesta,sizeProceso);

	if(operacionSuscripta == tipoDeMensaje){
	log_info(loggerObligatorio,"Suscripción con éxito, Mi identificador: %s",identificador);

	free(paqueteDeRespuesta);
	recepcionDeMensajes(tiempo);
	}else log_error(loggerObligatorio,"No se pudo concretar la suscripción a %s",colaDeMensaje);
}

