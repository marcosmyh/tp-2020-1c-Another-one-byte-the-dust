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
typedef struct t_pokemon t_pokemon;

typedef struct{
	int socket;
	void *paquete;
}t_infoPaquete;

typedef enum{
	t_atraparPokemon,
	t_intercambiarPokemon,
}t_operacionEntrenador;

typedef struct {
	int idEntrenador;
	uint32_t coordenadaX;
	uint32_t coordenadaY;
	t_list* pokemones;
	t_list* objetivo;
	t_pokemon* pokemonAAtrapar;
	sem_t semaforoEntrenador;
	pthread_t hiloEntrenador;
	int rafagasEstimadas;
	int rafagasEjecutadas;
	int distancia;
	int cantPokemonesPorAtrapar;
	int contadorRR;
	uint32_t IdCatch;
	bool ocupado;
	bool completoObjetivo;
	bool blockeado;
	t_operacionEntrenador operacionEntrenador;
}t_entrenador;

struct t_pokemon{
	char* nombrePokemon;
	uint32_t coordenadaX;
	uint32_t coordenadaY;
};

//COLAS
t_list* colaNew;
t_list* colaReady;
t_list* colaExec;
t_list* colaBlocked;
t_list* colaExit;

//VARIABLES
pthread_t hiloAtencionCaught;
pthread_t hiloAtencionAppeared;
pthread_t hiloAtencionLocalized;
pthread_t hiloAtencionGameBoy;
pthread_t hiloSuscripcionBroker;

sem_t semaforoReconexion;
sem_t semaforoRespuestaCatch;
sem_t conexionRecuperadaDeAppeared;
sem_t conexionRecuperadaDeCaught;
sem_t conexionRecuperadaDeLocalized;


bool conexionAppeared = 0;
bool conexionCaught = 0;
bool conexionLocalized = 0;
bool seEnvioMensajeGET = 0;
bool llegoRespuesta = 0;
char* identificadorProceso = NULL;
t_log* logger;
t_log* loggerObligatorio;
t_config* archivoConfig;
t_list* objetivoTeam;
t_list* pokemonesAtrapados;
t_list* pokemonesEnMapa;
t_list* mensajesGET;
t_list* mensajesCATCH;
t_list* entrenadores;
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
void gestionarHilosDelBroker();
void crearLoggerObligatorio();
void leerArchivoDeConfiguracion();
void setearValores(t_config* archConfiguracion);
int iniciar_servidor(char* ip, char* puerto, t_log* log);
void gestionMensajesGameBoy(int* socket_servidor);
int conectarse_a_un_servidor(char* ip, char* puerto, t_log* log);
void enviarHandshake (int socket, char* identificador, t_operacion operacion);
void recepcionMensajesAppeared();
void recepcionMensajesCaught();
void recepcionMensajesLocalized();
void suscripcionColaCaught();
void suscripcionColaAppeared();
void suscripcionColaLocalized();
void reconexionColaAppeared();
int obtenerCantidadEntrenadores();
void inicializarEntrenadores();
void inicializarColas();
void enviarPokemonesAlBroker();
void enviarGET(char* pokemon);
void enviarCATCH(char* pokemon, uint32_t coordenadaX, uint32_t coordenadaY);
void enviarACK(uint32_t ID, t_operacion operacion);
void inicializarSemaforosEntrenadores();
bool estaEnElObjetivo(char* pokemon);
bool yaFueAtrapado(char* pokemon);
char* obtenerPokemon(t_pokemon* unPokemon);
void planificarEntrenadores();
void ejecutarEntrenador();
void capturarPokemon(t_entrenador* entrenadorAEjecutar);
void atraparPokemon(t_entrenador* entrenadorAEjecutar, t_pokemon* pokemonAAtrapar);
void moverEntrenador(t_entrenador* entrenadorAEjecutar, t_pokemon* pokemonAAtrapar);
t_pokemon* pokemonMasCercanoA(t_entrenador* unEntrenador);
bool comparadorPosiciones(int unaPosicion, int otraPosicion);
int elMenorNumeroDe(t_list* aux);
void completarCatch(t_entrenador* unEntrenador, bool resultadoCaught);
bool cumplioObjetivo(t_entrenador* unEntrenador); //USAR ESTA PARA CHEQUEAR SI EL TEAM CUMPLIÓ EL OBJETIVO
void sacarPokemonDelMapa(t_pokemon* unPokemon);
void aplicarFIFO();
void guardarID(uint32_t *,uint32_t *);
void aplicarRR();
void aplicarSJFConDesalojo();
void aplicarSJF();
t_entrenador* calcularEstimacion(t_entrenador* unEntrenador);
bool comparadorDeEntrenadores(t_entrenador* unEntrenador, t_entrenador* otroEntrenador);
bool comparadorDeRafagas(t_entrenador* unEntrenador, t_entrenador* otroEntrenador);
bool comparadorIdCatch(t_entrenador* unEntrenador, uint32_t unIdCatch);
bool comparadorPokemones(t_pokemon* unPokemon, t_pokemon* otroPokemon);
int list_get_index(t_list* self, void* elemento, bool (*comparator) (void*,void*));
bool estaEnElMapa(char* unPokemon);
bool correspondeAUnIDDe(t_list* colaDeIDS, uint32_t IDCorrelativo); //LE PASAS UNA COLA DE IDS GUARDADOS Y UN ID A BUSCAR Y TE DICE SI ESTA O NO
void planificarEntradaAReady();
void administrarSuscripcionesBroker();
void calcularDistanciaA(t_list* listaEntrenadores, t_pokemon* unPokemon);
bool comparadorDeDistancia(t_entrenador* unEntrenador, t_entrenador* otroEntrenador);
bool estaOcupado(t_entrenador* unEntrenador);
int conectarseAColaMensaje(int socket,char *identificador,t_operacion operacion);
void conexionAColaAppeared();
void conexionAColaCaught();
void conexionAColaLocalized();
bool esSocketBroker(int socket);
void gestionMensajesCaught();
void gestionMensajesAppeared();
void gestionMensajesLocalized();
void iniciarGestionMensajes();
t_infoPaquete *crearInfoPaquete(int socket,void *paquete);
void destruirInfoMensaje(t_infoPaquete *info);
void procedimientoMensajeAppeared(t_infoPaquete *info);
void procedimientoMensajeCaught(t_infoPaquete *info);
void procedimientoMensajeLocalized(t_infoPaquete *info);
void procedimientoMensajeID(int socket);
void recibirHandshake(int socket,uint32_t tamanioPaquete);

#endif
