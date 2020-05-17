#ifndef TEAM_H
#define TEAM_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <readline/readline.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <semaphore.h>

#include "../Serializacion/Serializacion.h"


//ESTRUCTURAS
typedef struct {
	int idEntrenador;
	uint32_t coordenadaX;
	uint32_t coordenadaY;
	t_list* pokemones;
	t_list* objetivo;
	int rafagasEstimadas;
	int rafagasEjecutadas;
	int distancia;
	bool ocupado;
	bool completoObjetivo;
	bool blockeado;
}t_entrenador;

typedef struct{
	char* nombrePokemon;
	uint32_t coordenadaX;
	uint32_t coordenadaY;
}t_pokemon;

//COLAS
t_list* colaNew;
t_list* colaReady;
t_list* colaExec;
t_list* colaBlocked;
t_list* colaExit;

//VARIABLES

//pthread_mutex_t semaforoAppeared;
//pthread_mutex_t semaforoCaught;
pthread_t hiloConexionCaught;
sem_t semaforoAtencionCaught;
sem_t semaforoCreacionCaught;
sem_t semaforoAtencionLocalized;
sem_t semaforoCreacionLocalized;


int flag = 0;
char* identificadorProceso = NULL;
t_log* logger;
t_log* loggerObligatorio;
t_config* archivoConfig;
t_list* objetivoTeam;
t_list* pokemonesAtrapados;
t_list* pokemonesEnMapa;
t_list* mensajesGET;
t_list* mensajesCATCH;
char** posiciones_entrenadores;
char** pokemon_entrenadores;
char** objetivos_entrenadores;
int tiempo_reconexion;
int retardo_ciclo_cpu;
char* algoritmo_planificacion;
int quantum;
double alpha;
int estimacion_inicial;
char* ip_broker;
char* puerto_broker;
char* ip_team;
char* puerto_team;
char* team_log_file;
int socket_servidor;
int socket_cliente;
int socket_appeared;
int socket_caught;
int socket_localized;

//FUNCIONES
void crearLogger();
void crearLoggerObligatorio();
void leerArchivoDeConfiguracion();
void setearValores(t_config* archConfiguracion);
int iniciar_servidor(char* ip, char* puerto, t_log* log);
int esperar_cliente(int socket_servidor, t_log* logger);
int conectarse_a_un_servidor(char* ip, char* puerto, t_log* log);
int conectarseAColasMensajes(char* ip, char* puerto, t_log* log);
void enviarHandshake (int socket, char* identificador, t_operacion operacion);
void reconectarseAColasMensajes();
int obtenerCantidadEntrenadores();
void inicializarEntrenadores();
void inicializarColas();
void enviarPokemonesAlBroker();
void enviarGET(char* ip, char* puerto, char* pokemon);
void enviarCATCH(char* ip, char* puerto, char* pokemon, uint32_t coordenadaX, uint32_t coordenadaY);
void enviarACK(char* ip, char* puerto, uint32_t ID, t_operacion operacion);
bool necesitaAtraparse(char* pokemon);
char* obtenerPokemon(t_pokemon* unPokemon);
void planificarEntrenadores();
void aplicarFIFO();
void aplicarRR();
void aplicarSJFConDesalojo();
void aplicarSJF();
t_entrenador* calcularEstimacion(t_entrenador* unEntrenador);
bool comparadorDeEntrenadores(t_entrenador* unEntrenador, t_entrenador* otroEntrenador);
bool comparadorDeRafagas(t_entrenador* unEntrenador, t_entrenador* otroEntrenador);
int list_get_index(t_list* self, void* elemento, bool (*comparator) (void*,void*));
bool estaEnElMapa(char* unPokemon);
bool correspondeAUnCatch(uint32_t id);
void planificarEntradaAReady();
void calcularDistanciaA(t_list* listaEntrenadores, t_pokemon* unPokemon);
bool comparadorDeDistancia(t_entrenador* unEntrenador, t_entrenador* otroEntrenador);
bool estaOcupado(t_entrenador* unEntrenador);
int conectarseAColaMensaje(int socket,char *identificador,t_operacion operacion);
char *recibirIdentificadorProceso(int socket);
void conexionAColaAppeared(int *socket);
void conexionAColaCaught(int *socket);
void conexionAColaLocalized(int *socket);
void conexionAColas();
void atenderCliente(int *socket_cliente); //ESTA ES LA FUNCION MAGICA

#endif

