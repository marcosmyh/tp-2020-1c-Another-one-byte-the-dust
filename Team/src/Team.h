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

//ESTRUCTURAS


//VARIABLES
t_log* logger;
t_log* loggerTeams;
t_config* archivoConfig;
t_list* posiciones_entrenadores;
t_list* pokemon_entrenadores;
t_list* objetivos_entrenadores;
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

//FUNCIONES
void crearLogger();
void crearLoggerTeams();
void leerArchivoDeConfiguracion();
void setearValores(t_config* archConfiguracion);
int iniciar_servidor(char* ip, char* puerto, t_log* log);
int esperar_cliente(int socket_servidor, t_log* logger);
int conectarse_a_un_servidor(char* ip, char* puerto, t_log* log);
void atenderCliente(int socket_cliente); //ESTA ES LA FUNCION MAGICA

#endif
