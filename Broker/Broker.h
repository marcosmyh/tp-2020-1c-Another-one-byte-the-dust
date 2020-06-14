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
#include <semaphore.h>

typedef enum{
	CORRELATIVE_ID = 1,
	DOUBLE_ID,
	BROKER_ID,
	PARTITION_ID,
	SIZE_PARTITION,
} t_FLAG;

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
	uint32_t tamanioPaquete;
	t_list *suscriptoresALosQueMandeMensaje;
	t_list *suscriptoresQueRecibieronMensaje;
} t_mensaje;

typedef struct
{
	uint32_t ID_Particion;
	uint32_t tamanioParticion;
	uint32_t offset;
	uint32_t ID_mensaje;
	char *colaDeMensaje;
	bool estaLibre;
}t_particion;

//Variables
uint32_t offsetCache = 0;
uint32_t contadorIDMensaje;
uint32_t contadorIDTeam;
uint32_t contadorIDGameCard;
uint32_t contadorParticiones = 0;

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
t_list *particionesLibres;
t_list *particiones;
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
pthread_mutex_t semaforoMensajes;
pthread_mutex_t semaforoSuscripcionProceso;

//Funciones
t_log *crearLogger();
t_config *crearConfig();
int iniciar_servidor(char *, char *);
int esperar_cliente(int);
void atender_cliente(int *);
void procesar_solicitud(Header,int);
void setearValoresConfig();
void agregarMensajeACola(t_mensaje *,t_list *,char *);
void inicializarColas();
void destruirColas();
void suscribirProceso(char *,int ,t_operacion);
t_mensaje *crearMensaje(void *,uint32_t,uint32_t,uint32_t);
uint32_t asignarIDMensaje();
void inicializarListasSuscriptores();
void destruirListasSuscriptores();
void enviarMensajeRecibidoASuscriptores(t_list *,void(*)(t_suscriptor*));
void validarRecepcionMensaje(uint32_t,t_operacion,char *);
t_suscriptor *crearSuscriptor(char *,int);
char *asignarIDProceso(char *);
bool existeID(void *, t_list *,bool(*)(void *,void *));
bool intComparator(void *,void *);
bool stringComparator(void *, void *);
void *quitarIDPaquete(void *,uint32_t);
void *insertarIDEnPaquete(uint32_t,void *,uint32_t,t_FLAG);
bool existeRespuestaEnCola(uint32_t,t_list *);
uint32_t *obtenerIDCorrelativo(t_mensaje *);
uint32_t *obtenerIDMensaje(t_mensaje *);
t_mensaje *obtenerMensaje(uint32_t,t_list *,t_FLAG);
t_mensaje *obtenerMensajeDeCola(uint32_t,t_list *,uint32_t *(*)(t_mensaje *));
bool esPrimeraConexion(char *);
bool esUnTeam(char *);
bool esUnGameCard(char *);
bool esUnGameBoy(char *);
void iniciarMemoria();
void cachearMensaje(t_mensaje *,char *);
void destruirMensaje(t_mensaje *);
void destruirIdentificador(char *);
uint32_t obtenerPosicionMensaje(t_mensaje *,t_list *);
void eliminarMensaje(uint32_t, t_list *,char *,t_FLAG);
void eliminarPaqueteDeMemoria(uint32_t);
void compactarMemoria(uint32_t);
t_particion *crearParticion(uint32_t,uint32_t,char *);
uint32_t espacioDisponible();
void *descachearPaquete(t_mensaje *,char *);
t_particion *obtenerParticion(uint32_t,t_FLAG);
void ocuparEspacio(t_particion *);
bool tienenIDCorrelativoLosMensajes(char *);
void enviarMensajesCacheados(t_suscriptor *,t_list *);
bool existeAlgunACK(t_suscriptor *,t_list *);
bool existeACK(t_suscriptor *,t_list *);
bool existeParticionLibreConTamanioExacto(uint32_t);
uint32_t *obtenerIDParticion(t_particion *);
uint32_t *obtenerIDMensajeParticion(t_particion *);
bool esUnaParticionHija(t_particion *);
bool hayParticionesLibres();
t_particion *obtenerParticionLibreParaCachear(uint32_t);
bool existeParticionLibreConTamanio(uint32_t);
void destruirParticion(t_particion *);
void setearOffset(uint32_t,t_particion *);
void inicializarSemaforos();
void destruirSemaforos();

#endif
