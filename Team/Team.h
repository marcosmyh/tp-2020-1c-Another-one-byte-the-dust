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

typedef struct t_entrenador t_entrenador;

typedef struct{
	int socket;
	void *paquete;
}t_infoPaquete;

typedef enum{
	t_NEW_STATE,
	t_BLOCKED_STATE
} t_FLAG;


typedef enum{
	t_atraparPokemon,
	t_intercambiarPokemon,
}t_operacionEntrenador;

struct t_entrenador{
	int idEntrenador;
	uint32_t coordenadaX;
	uint32_t coordenadaY;
	t_list* pokemones;
	t_list* objetivo;
	t_list* pokemonesQueFaltan;
	t_pokemon* pokemonAAtrapar;
	t_pokemon* pokemonQueNecesito;
	t_entrenador* entrenadorAIntercambiar;
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
	int ciclosEjecutados;
};

struct t_pokemon{
	char* nombrePokemon;
	uint32_t coordenadaX;
	uint32_t coordenadaY;
	int distanciaAEntrenador;
};

//COLAS
t_list* colaNew;
t_list* colaReady;
t_list* colaExec;
t_list* colaBlocked;
t_list* colaExit;
t_list *pokemonesDuranteEjecucion;

//VARIABLES
pthread_t hiloAtencionCaught;
pthread_t hiloAtencionAppeared;
pthread_t hiloAtencionLocalized;
pthread_t hiloAtencionGameBoy;
pthread_t hiloSuscripcionBroker;

pthread_t hiloMensajes;
pthread_t hiloPlanificacionReady;
pthread_t hiloPlanificacionEntrenadores;
pthread_t hiloRevisionDeadlocks;
pthread_t impresionDeMetricas;

sem_t semaforoReconexion;
sem_t semaforoRespuestaCatch;
sem_t conexionRecuperadaDeAppeared;
sem_t conexionRecuperadaDeCaught;
sem_t conexionRecuperadaDeLocalized;
sem_t semaforoMetricas;
sem_t semaforoReady;
sem_t semaforoMensajesAppeared;
sem_t semaforoControlAppeared;
sem_t semaforoAgregadoPokemones;
sem_t semaforoExec;
sem_t semaforoInicioDeadlock;

sem_t semaforoPlanificacionReady;
sem_t semaforoPlanificacionExec;

pthread_mutex_t mutexPokemonesEnMapa;
pthread_mutex_t mutexPlanificacionReady;
pthread_mutex_t mutexPlanificacionEntrenadores;
pthread_mutex_t mutexEntrenadoresReady;
pthread_mutex_t mutexDeadlock;
pthread_mutex_t mutexRevisionDeadlock;

//METRICAS
int ciclosTotales = 0;
int cambiosDeContexto = 0;
int deadlocksProducidos = 0;
int deadlocksResueltos = 0;

bool planificacionInicialReady = 0;
bool conexionAppeared = 0;
bool conexionCaught = 0;
bool conexionLocalized = 0;
bool seEnvioMensajeGET = 0;
bool llegoRespuesta = 0;
bool planificacionCompleta = 0;
char* identificadorProceso;
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
void destruirSemaforosTeam();
void inicializarHilosTeam();
void inicializarSemaforosTeam();
bool hayPokemones = 0;
bool hayEntrenadores = 0;
int tamArray (char** puntero);
bool charContains(char *cadena,char caracter);
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
void completarCatch(t_entrenador* unEntrenador, bool resultadoCaught);
void completarCatchNormal(t_entrenador* unEntrenador);
void completarCatchSinBroker(t_entrenador* unEntrenador);
void completarIntercambioNormal(t_entrenador* unEntrenador);
void completarIntercambioSinBroker(t_entrenador* unEntrenador);
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

void revisionDeadlocks();
bool cumplioObjetivoTeam();
bool noCumplioObjetivo(t_entrenador* unEntrenador);
void detectarDeadlocks();
t_entrenador* obtenerEntrenadorQueTieneMiPokemon(t_entrenador* unEntrenador,char *unPokemon);
void generarPokemonesFaltantes();
void agregarPokemonesQueFaltan(t_entrenador* unEntrenador);

void intercambiarPokemon(t_entrenador* entrenadorAEjecutar);
bool comparadorNombrePokemones(char* unPokemon, char* otroPokemon);
char *pokemonQueNoNecesito(t_entrenador* unEntrenador);
bool estaEnObjetivo(char* unPokemon, t_entrenador* unEntrenador);
void moverEntrenadorParaIntercambio(t_entrenador* entrenadorAEjecutar, t_entrenador* otroEntrenador);
void realizarIntercambio(t_entrenador* entrenadorAEjecutar, t_entrenador*entrenadorAIntercambiar, char* pokemonQueElOtroNoNecesita,  char* pokemonQueYoNoNecesito);

void imprimirMetricas();
void metricas();
bool tieneLosMismosElementos(t_list*,t_list*,bool(*)(void *,void *));
int cantOcurrencias(char *nombrePokemon,t_list *lista);
bool stringComparator(void *unString, void *otroString);
void transicionAReady(t_entrenador *entrenador,t_FLAG estado);
void transicionDeBlockedAReady(t_entrenador *entrenador);
void transicionDeNewAReady(t_entrenador *entrenador);
t_entrenador *entrenadorQueVaAReady(t_list *,int);
t_list *obtenerEntrenadoresDisponibles();
void mostrarContenidoLista(t_list* lista,void(*printer)(void *));
void imprimirString(void *contenidoAMostrar);
void imprimirNumero(void *);
t_pokemon *crearPokemon(char *,uint32_t,uint32_t);
char *obtenerPokemonParaIntercambiar(t_entrenador *,t_entrenador *);
t_list *obtenerParejasEnDeadlock(t_list *);
t_entrenador *obtenerEntrenadorQueTieneElPokemon(char *pokemon,t_list *entrenadoresEnDeadlock,t_entrenador *entrenador);
bool existeID(void *ID_a_buscar, t_list *lista,bool(*comparator)(void *,void *));
int *obtenerIDEntrenador(t_entrenador *entrenador);
bool intComparator(void *unNumero,void *otroNumero);
bool estaRepetidoPokemon(char *pokemon,t_entrenador *entrenador);
void quitarParejasRepetidas(t_list *parejasEnDeadlock);
void cambiarOperacionEntrenadores(t_list *,int);
void modificarContadorDeadlocks(int);
bool estaEnBlocked(t_entrenador *entrenador);
bool hayPokemonesParaAtrapar();

#endif
