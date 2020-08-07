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
#include <math.h>
#include <signal.h>
#include <sys/time.h>

typedef enum{
	CORRELATIVE_ID = 1,
	DOUBLE_ID,
	BROKER_ID,
	PARTITION_ID,
	SIZE_PARTITION,
	OFFSET,
	ACK,
	REPLACEMENT_ALGORITHM
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
	uint32_t tamanioPaquete;
	char *colaALaQuePertenece;
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
	uint64_t ultimoAcceso;
}t_particion;


typedef struct
{
	void *paqueteACachear;
	void(*cachearMensaje)(t_mensaje *,uint32_t);
	t_particion *(*obtenerParticionLibre)(uint32_t);
	void(*ejecutarAlgoritmoDeReemplazo)();
	void(*liberarParticion)(t_particion *);
}t_singletonMemoria;

typedef struct nodo{
	uint32_t id;
	uint32_t offset;
	uint32_t tamanioQueOcupa;
	bool ocupado;
	struct nodo *hijoIzquierdo;
	struct nodo *hijoDerecho;
	struct nodo *padre;
}t_nodo;


uint32_t offsetCache = 0;
uint32_t contadorIDMensaje;
uint32_t contadorIDTeam;
uint32_t contadorIDGameCard;
uint32_t contadorParticiones = 0;
t_nodo* nodoRaiz;

t_singletonMemoria *singletonMemoria;
t_log *logObligatorio;
t_log *logExtra;
t_config *config;
void *memoria;
char *log_path_broker;
char *ip;
char *puerto;
char *algoritmo_memoria;
char *algoritmo_reemplazo;
char *algoritmo_particion_libre;
int socket_servidor;
int socket_cliente;
uint32_t tamanio_memoria;
uint32_t tamanio_minimo_particion;
uint32_t frecuencia_compactacion;
t_list *particiones;
t_list *particionesLibres;
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
pthread_t hiloAtencionCliente;
pthread_mutex_t semaforoIDMensaje;
pthread_mutex_t semaforoMensajes;

t_log *crearLogger(char *,bool);
t_config *crearConfig();
int iniciar_servidor(char *, char *);
int esperar_cliente(int);
void atender_cliente(int *);
void procesar_solicitud(Header,int);
void setearValoresConfig();
void agregarMensajeACola(t_mensaje *,t_list *);
void inicializarColas();
void destruirColas();
void suscribirProceso(char *,int ,t_operacion);
t_mensaje *crearMensaje(uint32_t,uint32_t,uint32_t,char *);
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
void iniciarMemoria();
void destruirMensaje(t_mensaje *);
void destruirIdentificador(char *);
uint32_t obtenerPosicionMensaje(t_mensaje *,t_list *);
void eliminarMensaje(uint32_t, t_list *,char *,t_FLAG);
void compactarMemoria();
t_particion *crearParticion(uint32_t);
void *descachearPaquete(t_mensaje *,char *);
t_particion *obtenerParticion(uint32_t,t_FLAG);
void ocuparEspacio(t_particion *);
bool tienenIDCorrelativoLosMensajes(char *);
void enviarMensajesCacheados(t_suscriptor *,t_list *,char *,t_operacion,t_FLAG);
bool existeAlgunACK(t_suscriptor *,t_list *);
bool existeACK(t_suscriptor *,t_list *);
uint32_t *obtenerIDParticion(t_particion *);
uint32_t *obtenerIDMensajeParticion(t_particion *);
void destruirParticion(t_particion *);
void setearOffset(uint32_t,t_particion *);
void inicializarSemaforos();
void destruirSemaforos();
void particionesDinamicas(t_mensaje *,uint32_t);
void cachearMensaje(t_mensaje *);
void liberarRecursosAdministracionMemoria();
bool tieneMenorOffset(t_particion *,t_particion *);
void consolidarParticion(t_particion *);
uint32_t obtenerPosicionParticion(uint32_t);
void modificarParticion(t_particion *,uint32_t,char *);
bool estaLibre(t_particion *);
void mostrarContenidoLista(t_list*,void(*)(void *));
void imprimirNumero(void *);
void imprimirString(void *contenidoAMostrar);
uint32_t obtenerPosicionIDParticion(uint32_t);
void buddySystem(t_mensaje *,uint32_t);
void construirSingletonMemoria();
void liberarParticionParticionesDinamicas(t_particion *);
void liberarParticionBuddySystem(t_particion *);
void FIFO();
void LRU();
t_particion *firstFit(uint32_t);
t_particion *bestFit(uint32_t);
uint32_t *obtenerOffsetParticion(t_particion *);
bool tieneMenorIDMensaje(t_particion *,t_particion *);
char *obtenerIDSuscriptor(t_suscriptor *);
bool existeSuscriptor(char *,t_list *);
void agregarSuscriptor(t_suscriptor *,t_list *,char *);
bool todosRecibieronElMensaje(t_mensaje *,t_list *);
void destruirSuscriptor(t_suscriptor *);
void setearHorarioAcceso(t_particion *);
bool seUsoMenosRecientemente(t_particion *,t_particion *);
void ordenarParticionesLibres(bool(*)(void *,void*));
void ordenarParticionesLibresSegun(t_FLAG);
bool tieneMenorTamanio(t_particion *,t_particion *);
void recorrerParticionesYLiberar(t_list *,char *);
t_list *obtenerColaMensaje(char *);
void dumpDeCache(int);
void mostrarEstadoDeMemoria();
void dumpDeCache(int);
uint64_t getHorario();
t_nodo* crearNodo(t_particion *);
void limpiarNodo(t_nodo *);
void crearNodoPadre();
void asignarPadre(t_nodo *, t_nodo *);
void asignarHijos(t_nodo *, t_nodo *,t_nodo *);
void escribirEnMemoria(void*,uint32_t,uint32_t);
int obtenerExponente(int);
int obtenerTamanioMinimoBuddy(int);
int crearHijos(t_nodo *);
t_nodo* buscarNodo(t_nodo *,t_particion *);
void ocuparNodo(t_nodo *,t_particion*);
void liberarNodo(t_nodo *);
bool hermanoLibre(t_nodo *);
void matarNodo(t_nodo *);
void reunirHermanos(t_nodo *);
t_nodo *obtenerHermano(t_nodo *);
void consolidarBuddy(t_nodo *);
uint32_t dividirYObtenerIDParticionExacta(t_nodo *, uint32_t);
bool existeParticionLibreParaCachearMensaje(uint32_t);
void enviarIDAlProductor(int,uint32_t,int);
void enviarACKBroker(int,uint32_t,t_operacion);


#endif
