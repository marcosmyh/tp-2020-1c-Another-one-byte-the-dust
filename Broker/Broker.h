#ifndef BROKER_BROKER_H_
#define BROKER_BROKER_H_

#include <stdio.h>
#include <stdlib.h>
#include "../Serializacion/Serializacion.h"
#include <commons/log.h>
#include <commons/config.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/collections/list.h>
#include <string.h>
#include <pthread.h>

//Estructuras
//Las presentes estructuras no son definitivas.

typedef struct
{
    int socket_suscriptor;
    t_list *colasALasQueEstaSuscrito;
}t_suscriptor;

typedef struct
{
	uint32_t IDMensaje;
	void* paquete;
} t_mensaje;


//Variables

t_log *logger;
t_config *config;
char *ip;
char *puerto;
char *algoritmo_memoria;
char *algoritmo_reemplazo;
char *algoritmo_particion_libre;
int socket_servidor;
int socket_cliente;
int tamanio_memoria;
int tamanio_minimo_particion;
int frecuencia_compactacion;
t_list *NEW_POKEMON;
t_list *APPEARED_POKEMON;
t_list *CATCH_POKEMON;
t_list *CAUGHT_POKEMON;
t_list *GET_POKEMON;
t_list *LOCALIZED_POKEMON;
pthread_t hiloAtencionCliente;


//Funciones
t_log *crearLogger();
t_config *crearConfig();
int iniciar_servidor(char *, char *);
int esperar_cliente(int);
void atender_cliente(int *);
void procesar_solicitud(Header,int);
void setearValores(t_config *);
void agregarMensajeACola(t_mensaje *,t_list *,char *);
void inicializarColas();
void destruirColas();
void suscribirProceso(t_operacion);

#endif
