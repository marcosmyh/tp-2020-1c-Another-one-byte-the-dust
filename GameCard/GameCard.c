/*
 ============================================================================
 Name        : GameCard.c
 Author      : Another One Byte The Dust
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "GameCard.h"


int main(void)
{
	int socket_servidor;
	crearLogger();

	//doy inicio a mi servidor y obtengo el socket
	socket_servidor = iniciar_servidor();
	//Servidor iniciado
	printf("Server ready for action! \n");

	while(1){
	int cliente = esperar_cliente(socket_servidor);

	if(pthread_create(&thread,NULL,(void*)atender_cliente,&cliente) == 0){
		pthread_detach(thread);
		}

	}
	return EXIT_SUCCESS;
}


void crearLogger(){
	char *path = "GameCardServer.log";
	char *nombreArchi = "GameCardServer";
	logger = log_create(path,nombreArchi,true, LOG_LEVEL_INFO);

}
