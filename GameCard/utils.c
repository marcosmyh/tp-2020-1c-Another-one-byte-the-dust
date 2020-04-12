/*
 * utils.c
 *
 *  Created on: 12 abr. 2020
 *      Author: utnso
 */

#include"utils.h"

int iniciar_servidor(void)
{
	int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(IP, PUERTO, &hints, &servinfo);

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

    return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	struct sockaddr_in dir_cliente;

	int tam_direccion = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);
	if(socket_cliente < 0) log_info(logger,"Se conecto un tipazo.");
	return socket_cliente;
}

void atender_cliente(int* socket)
{
	int cod_op;

	if(recv(*socket, &cod_op, sizeof(int), MSG_WAITALL) == -1)
		cod_op = -1;
	procesar_solicitud(cod_op, *socket);
}

void procesar_solicitud(int cod_op, int cliente_fd) {
	int size;
	//void* msg;

		switch (cod_op) {
		case GET:
			log_info(logger,"Me llegaron mensajes de Suscriber get \n");
			//Hacer algo con get
			//msg = recibir_mensaje(cliente_fd, &size);
			//free(msg);
			break;

		case CATCH:
			log_info(logger,"Me llegaron mensajes de Suscriber Catch \n");
			//Hacer lo que debia hacerse cuando se recibiera un catch
			//msg = recibir_mensaje(cliente_fd, &size);
			//free(msg);
			break;
		case NEW:
			log_info(logger,"Me llegaron mensajes de Suscriber New \n");
			//Hacer lo que debiera hacerse con new
			//msg = recibir_mensaje(cliente_fd, &size);
			//free(msg);
			break;
		case 0:
			printf("Se desconecto.\n");
			pthread_exit(NULL);

		case -1:
			pthread_exit(NULL);
		}
}

void* recibir_mensaje(int socket_cliente, int* size)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}
