/*
 * GameBoy.c
 *
 *  Created on: 9 may. 2020
 *      Author: utnso
 */

#include "GameBoy.h"



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

	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
		log_error(loggerObligatorio,"No se pudo conectar al proceso solicitado");
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
}

void terminar_programa()
{
	log_destroy(logger);
	config_destroy(config);
	log_destroy(loggerObligatorio);
	liberar_conexion(conexion);

}

int iniciarConexion(char* nombreDeProceso){
	char* claveIP = string_new();
	char* clavePUERTO = string_new();

	if(strcmp(nombreDeProceso,"SUSCRIPTOR") != 0){
	string_append(&claveIP,"IP_");
	string_append(&claveIP,nombreDeProceso);
	string_append(&clavePUERTO,"PUERTO_");
	string_append(&clavePUERTO,nombreDeProceso);
	}else{
		string_append(&claveIP,"IP_BROKER");
		string_append(&clavePUERTO,"PUERTO_BROKER");
	}
	ip = config_get_string_value(config,claveIP);
	puerto = config_get_string_value(config,clavePUERTO);

	free(claveIP);
	free(clavePUERTO);
	log_info(logger,ip);
	log_info(logger,puerto);

	conexion = crear_conexion(ip,puerto,nombreDeProceso);
	return conexion;
}

void enviar_mensaje_a_broker(char* tipo_de_mensaje,int cantidad_de_argumentos,char* argumentos[]){


	int hashDeMensaje = convertir_nombre(tipo_de_mensaje);


		switch(hashDeMensaje){
		// 	./gameboy GAMECARD NEW_POKEMON [POKEMON] [POSX] [POSY] [CANTIDAD] [ID_MENSAJE]
			case NEW:
				if(cantidad_de_argumentos == 8){
						char* pokemon = argumentos[3];
						uint32_t posx = atoi(argumentos[4]);
						uint32_t posy = atoi(argumentos[5]);
						uint32_t cantidad = atoi(argumentos[6]);
						uint32_t idmensaje = atoi(argumentos[7]);
					// packAndSend_New(conexion,idmensaje,pokemon,cantidad,posx,posy);
						// log_info(loggerObligatorio,"Mensaje de NEW enviado");
						printf("Aca envio el mensaje \n");
						printf("Pokemon: %s \n",pokemon);
						printf("PosX: %i \n",posx);
						printf("PosY: %i \n",posy);
						printf("Cantidad: %i \n",cantidad);
						printf("id mensaje: %i \n",idmensaje);
				}else printf("Faltan argumentos macho \n");
			break;

			//./gameboy BROKER APPEARED_POKEMON [POKEMON] [POSX] [POSY] [ID_MENSAJE]
			case APPEARED://if(string_contains("GBS",primerLetraDeProceso)) valor = 1;
				if(cantidad_de_argumentos == 7){
								char* pokemon = argumentos[3];
								uint32_t posx = atoi(argumentos[4]);
								uint32_t posy = atoi(argumentos[5]);
								uint32_t idmensaje = atoi(argumentos[6]);
								// packAndSend_Appeared(conexion,idmensaje,pokemon,posx,posy);
								// log_info(loggerObligatorio,"Mensaje de APPEARED enviado");
								printf("Aca envio el mensaje \n");
								printf("Pokemon: %s \n",pokemon);
								printf("PosX: %i \n",posx);
								printf("PosY: %i \n",posy);
								printf("id mensaje: %i \n",idmensaje);
								}else printf("Faltan argumentos macho \n");


			break;

			//./gameboy BROKER CATCH_POKEMON [POKEMON] [POSX] [POSY]
			case CATCH://if(string_contains("GBS",primerLetraDeProceso)) valor = 1;
				if(cantidad_de_argumentos == 6){
								char* pokemon = argumentos[3];
								uint32_t posx = atoi(argumentos[4]);
								uint32_t posy = atoi(argumentos[5]);
								// packAndSend_Appeared(conexion,-1,pokemon,posx,posy);
								// log_info(loggerObligatorio,"Mensaje de CATCH enviado");
								printf("Aca envio el mensaje \n");
								printf("Pokemon: %s \n",pokemon);
								printf("PosX: %i \n",posx);
								printf("PosY: %i \n",posy);
								}else printf("Faltan argumentos macho \n");
			break;

			//./gameboy BROKER CAUGHT_POKEMON [ID_MENSAJE] [OK/FAIL]
			case CAUGHT://if(string_contains("GBS",primerLetraDeProceso)) valor = 1;
				if(cantidad_de_argumentos == 5){
								uint32_t idmensaje = atoi(argumentos[3]);
								uint32_t atrapado = atoi(argumentos[4]);
								// packAndSend_Caught(conexion,idmensaje,atrapado);
								// log_info(loggerObligatorio,"Mensaje de CAUGHT enviado");
								printf("Atrapado: %i \n",atrapado);
								printf("id mensaje: %i \n",idmensaje);
								}else printf("Faltan argumentos macho \n");
			break;

			//./gameboy BROKER GET_POKEMON [POKEMON]
			case GET://if(string_contains("GBS",primerLetraDeProceso)) valor = 1;
				if(cantidad_de_argumentos == 4){
								char* pokemon = argumentos[3];
								// packAndSend_Get(conexion,-1,pokemon);
								// log_info(loggerObligatorio,"Mensaje de GET enviado");
								printf("pokemon: %s \n",pokemon);
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
    	// packAndSend_Appeared(conexion,-1,pokemon,posx,posy);
		// log_info(loggerObligatorio,"Mensaje de APPEARED enviado");
		printf("Aca envio el mensaje \n");
		printf("Pokemon: %s \n",pokemon);
		printf("PosX: %i \n",posx);
		printf("PosY: %i \n",posy);
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
						// packAndSend_New(conexion,idmensaje,pokemon,cantidad,posx,posy);
							// log_info(loggerObligatorio,"Mensaje de NEW enviado");
							printf("Aca envio el mensaje \n");
							printf("Pokemon: %s \n",pokemon);
							printf("PosX: %i \n",posx);
							printf("PosY: %i \n",posy);
							printf("Cantidad: %i \n",cantidad);
							printf("id mensaje: %i \n",idmensaje);
					}else printf("Faltan argumentos macho \n");
				break;
				// ./gameboy GAMECARD CATCH_POKEMON [POKEMON] [POSX] [POSY] [ID_MENSAJE]
				case CATCH://if(string_contains("GBS",primerLetraDeProceso)) valor = 1;
					if(cantidad_de_argumentos == 7){
									char* pokemon = argumentos[3];
									uint32_t posx = atoi(argumentos[4]);
									uint32_t posy = atoi(argumentos[5]);
									uint32_t idmensaje = atoi(argumentos[6]);
									// packAndSend_Appeared(conexion,-1,pokemon,posx,posy);
									// log_info(loggerObligatorio,"Mensaje de CATCH enviado");
									printf("Aca envio el mensaje \n");
									printf("Pokemon: %s \n",pokemon);
									printf("PosX: %i \n",posx);
									printf("PosY: %i \n",posy);
									printf("id mensaje: %i \n",idmensaje);
									}else printf("Faltan argumentos macho \n");
				break;

				// ./gameboy GAMECARD GET_POKEMON [POKEMON]
				case GET://if(string_contains("GBS",primerLetraDeProceso)) valor = 1;
					if(cantidad_de_argumentos == 4){
									char* pokemon = argumentos[3];
									printf("%s \n",pokemon);
									//uint32_t id = -1;
									//if(packAndSend_Get(conexion,id,pokemon))log_info(loggerObligatorio,"Mensaje de GET enviado");
									//else log_error(loggerObligatorio,"Mensaje de GET no enviado");
									printf("pokemon: %s \n",pokemon);
									}else printf("Faltan argumentos macho \n");
				break;
				default:
				break;
				}
}

int main(int argc,char* argv[]){

	// argv[0] es el nombre de nuestro programa
	// argv[1] va a ser el nombre de proceso a ejecutar
	// argv[2] es el tipo de mensaje
	// En adelante va a ser los argumentos enviados
	char* nombreDeProceso = argv[1];
	char* tipoDeMensaje = argv[2];
	printf("Enviaste el proceso: %s \n",nombreDeProceso); //---------------------- QUITAR
	printf("Enviaste el mensaje: %s \n",tipoDeMensaje); 	//---------------------- QUITAR
	printf("---------------- \n");		//---------------------- QUITAR
	int hashDeProceso = convertir_nombre(nombreDeProceso);

	//int hashDeMensaje = convertir_nombre(tipoDeMensaje);
	//printf("El hash es: %i \n",hashDeMensaje);

	iniciar_logger();
	iniciar_logger_obligatorio();
	leer_config();

	switch(hashDeProceso){


	case BROKER:
				printf("Broker Omegalul \n");		//---------------------- QUITAR
				if(esUnTipoDeMensajeValido(nombreDeProceso,tipoDeMensaje)){
								conexion = iniciarConexion(nombreDeProceso);
								enviar_mensaje_a_broker(tipoDeMensaje,argc,argv);
								break;
							} else log_error(logger,"No está definido el tipo de mensaje %s para %s",tipoDeMensaje,nombreDeProceso);		//---------------------- QUITAR
				break;

	case TEAM:
				printf("Team omegalul \n");
				if(esUnTipoDeMensajeValido(nombreDeProceso,tipoDeMensaje)){
								conexion = iniciarConexion(nombreDeProceso);
								enviar_mensaje_a_team(tipoDeMensaje,argc,argv);
							} else log_error(logger,"No está definido el tipo de mensaje %s para %s",tipoDeMensaje,nombreDeProceso);
				break;
	case GAMECARD:
				printf("Gamecard omegalul \n");
				if(esUnTipoDeMensajeValido(nombreDeProceso,tipoDeMensaje)){
								conexion = iniciarConexion(nombreDeProceso);
								enviar_mensaje_a_gamecard(tipoDeMensaje,argc,argv);
							} else log_error(logger,"No está definido el tipo de mensaje %s para %s",tipoDeMensaje,nombreDeProceso);
				break;

//./gameboy SUSCRIPTOR [COLA_DE_MENSAJES] [TIEMPO]
	case SUSCRIPTOR:
				printf("Suscriptor omegalul \n");
				if(esUnTipoDeMensajeValido(nombreDeProceso,tipoDeMensaje)){
								conexion = iniciarConexion(nombreDeProceso);
								char* colaDeMensaje = argv[2];
								int tiempo = atoi(argv[3]);
								// Hay que ver como enviar el nombre del mensaje.. se necesita una funcion que transforme nuestra
								// variable colaDeMensaje en un t_operacion
								// Tambien hay que ver como hacer la desconexion en tiempo x
								//packAndSend_Handshake(conexion, "GAMEBOY" , colaDeMensaje);

								printf("la cola a suscribirse es: %s \n",colaDeMensaje);
								printf("El tiempo es: %i \n",tiempo);
								printf("Ahora aca perri\n");		//---------------------- QUITAR
							} else log_info(logger,"No está definido el tipo de mensaje %s para %s",tipoDeMensaje,nombreDeProceso);

				break;

			default: printf("Ese proceso no existe macho \n");
			return 0;

	}


	terminar_programa();

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
