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
	bool completoObjetivo;
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
t_log* logger;
t_log* loggerObligatorio;
t_config* archivoConfig;
t_list* objetivoTeam;
t_list* pokemonesAtrapados;
t_list* pokemonesEnMapa;
char** posiciones_entrenadores;
char** pokemon_entrenadores;
char** objetivos_entrenadores;
int tiempo_reconexion;
int retardo_ciclo_cpu;
char* algoritmo_planificacion;
int quantum;
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
void reconectarseAColasMensajes();
int obtenerCantidadEntrenadores();
void inicializarEntrenadores();
void inicializarColas();
void enviarPokemonesAlBroker();
void enviarGET(char* ip, char* puerto, t_log* logger, char* pokemon);
void enviarCATCH(char* ip, char* puerto, t_log* logger, char* pokemon, uint32_t coordenadaX, uint32_t coordenadaY);
bool necesitaAtraparse(char* pokemon);
char* obtenerPokemon(t_pokemon* unPokemon);
void planificarEntrenadores();
void aplicarFIFO();
void aplicarRR();
void aplicarSJFConDesalojo();
void aplicarSJF();
t_entrenador* calcularEstimacion(t_entrenador unEntrenador);
bool comparadorDeEntrenadores(t_entrenador* unEntrenador, t_entrenador* otroEntrenador);
bool comparadorDeRafagas(t_entrenador* unEntrenador, t_entrenador* otroEntrenador);
int list_get_index(t_list* self, void* elemento, bool (*comparator) (void*,void*));
void planificarEntradaAReady();
void atenderCliente(int socket_cliente); //ESTA ES LA FUNCION MAGICA

#endif
