/*
 * GameCard.h
 *
 *  Created on: 12 abr. 2020
 *      Author: utnso
 */

#ifndef GAMECARD_GAMECARD_H_
#define GAMECARD_GAMECARD_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<commons/log.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/collections/list.h>
#include<commons/config.h>
#include<string.h>
#include<pthread.h>

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

//VARIABLES


pthread_t thread;
t_log *logger; // ALGO QUE NO ESTA TAN BIEN CREO CREER
int socket_servidor;
t_config *archivoConfig;
char* ip_gc;
char* puerto_gc;
char* ip_broker;
char* puerto_broker;
int tiempo_de_reconexion;
int tiempo_de_reintento_operacion;
char* path_de_tallgrass;

void crearLogger(void);
int iniciar_servidor(char* ip, char* puerto, t_log* log);
int esperar_cliente(int socket_servidor, t_log* log);
void atender_cliente(int *socket);
void procesar_solicitud(int cod_op, int cliente_fd);
void leerArchivoDeConfiguracion(void);
void setearValoresDeGame(t_config *confg);

#endif /* GAMECARD_GAMECARD_H_ */
