/*
 * utils.h
 *
 *  Created on: 12 abr. 2020
 *      Author: utnso
 */

#ifndef GAMECARD_UTILS_H_
#define GAMECARD_UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<pthread.h>

#define IP "127.0.0.1"
#define PUERTO "4444"

typedef enum
{
	GET=1,
	CATCH=2,
	NEW=3,
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

pthread_t thread;
t_log *logger; // ALGO QUE NO ESTA TAN BIEN CREO CREER

void* recibir_buffer(int*, int);

int iniciar_servidor(void);
int esperar_cliente(int);
void* recibir_mensaje(int socket_cliente, int* size);
void procesar_solicitud(int cod_op, int cliente_fd);
void atender_cliente(int *socket);

#endif /* GAMECARD_UTILS_H_ */
