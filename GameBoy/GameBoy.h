/*
 * GameBoy.h
 *
 *  Created on: 10 may. 2020
 *      Author: utnso
 */

#ifndef GAMEBOY_GAMEBOY_H_
#define GAMEBOY_GAMEBOY_H_

#include<stdio.h> // AGREGADO
#include<stdlib.h> // AGREGADO
#include<commons/log.h> // AGREGADO
#include<commons/string.h> // AGREGADO
#include<commons/config.h> // AGREGADO
#include<pthread.h>


#include<sys/socket.h>
#include<netdb.h>
#include<unistd.h>
#include<string.h>
#include <ctype.h>


#include "../Serializacion/Serializacion.h"
// VARIABLES
int conexion;
char* IP_BROKER;
char* IP_TEAM;
char* ip_broker;
char* IP_GAMECARD;
char* PUERTO_BROKER;
char* PUERTO_TEAM;
char* PUERTO_GAMECARD;
int tiempoRestante;
t_log* logger;
t_log* loggerObligatorio;
t_config* config;

// Estructuras
// Cada proceso escrito como la suma de sus caracteres ascii's
typedef enum{
	BROKER = 453,
	TEAM = 295,
	GAMECARD = 564,
	SUSCRIPTOR = 798
}t_proceso;

// Cada nombre de operacion escrita como la suma de sus caracteres ascii's
typedef enum{
	NEW = 234,
	LOCALIZED = 663,
	GET = 224,
	APPEARED = 578,
	CATCH = 355,
	CAUGHT = 444,
	SUSCRIPCION = 850
} t_nombre_de_op;

//Utilidades varias
int iniciarConexion(char* nombreDeProceso);
int crear_conexion(char *ip, char* puerto, char* nombreDeProceso);
void liberar_conexion(int socket_cliente);

void terminar_programa();
void leer_config(void);
void cargarConfig(t_config*);
void iniciar_logger_obligatorio(void);
void iniciar_logger(void);

// Preguntas
bool esUnTipoDeMensajeValido(char* nombreDeProceso, char* tipo_de_mensaje);

// Gran utilidad
int convertir_nombre(char* nombreDeProceso);
char* obtenerNombreSinElPokemon(char* proceso);

//Envio de mensajes a los distintos procesos
void enviar_mensaje_a_gamecard(char* tipo_de_mensaje,int cantidad_de_argumentos,char* argumentos[]);
void enviar_mensaje_a_team(char* tipo_de_mensaje,int cantidad_de_argumentos,char* argumentos[]);
void enviar_mensaje_a_broker(char* tipo_de_mensaje,int cantidad_de_argumentos,char* argumentos[]);

void envioDeMensajeNew(char* pokemon, uint32_t posx, uint32_t posy,uint32_t cantidad,uint32_t idmensaje);
void envioDeMensajeAppeared(char* pokemon, uint32_t posx, uint32_t posy, uint32_t idmensaje);
void *paqueteAppearedTeam(char *pokemon,uint32_t posx,uint32_t posy,uint32_t idMensaje);
void *insertarIDEnPaquete(uint32_t ID,void *paquete,uint32_t tamanioPaquete);
void envioDeMensajeCatch(char* pokemon, uint32_t posx, uint32_t posy, uint32_t idmensaje);
void envioDeMensajeCaught(uint32_t atrapado, uint32_t idmensaje);
void envioDeMensajeGet(char* pokemon,uint32_t idmensaje);

void conectarmeACola(int socket,int tiempo,char* colaDeMensaje);

#endif /* GAMEBOY_GAMEBOY_H_ */
