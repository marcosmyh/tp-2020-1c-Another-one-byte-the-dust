#ifndef BROKER_BROKER_H_
#define BROKER_BROKER_H_

#include <stdio.h>
#include <stdlib.h>
#include "../Serializacion/Serializacion.h"
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <string.h>
#include <pthread.h>

//Estructuras
//Las presentes estructuras no son definitivas.


typedef struct
{
    int socket_suscriptor;
    char *identificadorProceso;
}t_suscriptor;


typedef struct
{
	uint32_t ID_mensaje;
	uint32_t ID_correlativo;
	void *paquete;
	t_list *suscriptoresQueRecibieronMensaje;
} t_mensaje;


//Variables

uint32_t contadorIDMensaje = 0;
uint32_t contadorIDTeam = 0;
uint32_t contadorIDGameCard = 0;
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
t_list *suscriptores_NEW_POKEMON;
t_list *APPEARED_POKEMON;
t_list *suscriptores_APPEARED_POKEMON;
t_list *CATCH_POKEMON;
t_list *suscriptores_CATCH_POKEMON;
t_list *CAUGHT_POKEMON;
t_list *suscriptores_CAUGHT_POKEMON;
t_list *GET_POKEMON;
t_list *suscriptores_GET_POKEMON;
t_list *LOCALIZED_POKEMON;
t_list *suscriptores_LOCALIZED_POKEMON;
t_list *IDs_mensajes;
t_list *IDs_procesos;
pthread_t hiloAtencionCliente;
pthread_mutex_t semaforoIDMensaje;
pthread_mutex_t semaforoIDTeam;
pthread_mutex_t semaforoIDGameCard;


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
void suscribirProceso(char *,int ,t_operacion);
t_mensaje *crearMensaje(void *,uint32_t);
uint32_t asignarIDMensaje();
void inicializarListasSuscriptores();
void destruirListasSuscriptores();
void enviarMensajeRecibidoASuscriptores(t_list *,void(*)(void*));
void validarRecepcionMensaje(uint32_t,t_operacion,char *);
t_suscriptor *crearSuscriptor(char *,int);
char *asignarIDProceso(char *);
bool yaExisteID(void *, t_list *,bool(*)(void *,void *));
bool intComparator(void *,void *);
bool stringComparator(void *, void *);
t_mensaje *obtenerMensaje(uint32_t,t_list *);

#endif
