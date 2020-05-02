/*
 * GameCard.h
 *
 *  Created on: 12 abr. 2020
 *      Author: utnso
 */

#ifndef GAMECARD_GAMECARD_H_
#define GAMECARD_GAMECARD_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<commons/config.h>
#include<commons/string.h>
#include<string.h>
#include<pthread.h>
#include "../Serializacion/Serializacion.h"

//VARIABLES


pthread_t thread;
t_log *logger; // ALGO QUE NO ESTA TAN BIEN CREO CREER
int socket_servidor;
t_config *archivoConfig;
char* ip_gc;
char* puerto_gc;
char* ip_broker;
char* puerto_broker;
int tiempo_de_reconexion;
int tiempo_de_reintento_operacion;
char* path_de_tallgrass;

void crearLogger(void);
int iniciar_servidor(char* ip, char* puerto, t_log* log);
int esperar_cliente(int socket_servidor, t_log* log);
void atender_cliente(int *socket);
void procesar_solicitud(int cod_op, int cliente_fd);
void leerArchivoDeConfiguracion(void);
void setearValoresDeGame(t_config *confg);

////////////
// Sobre el FS
////////////

uint32_t TAMANIO_DE_BLOQUE;
uint32_t BLOQUES_DE_BITMAP;
char *MAGIC_NUMBER;
char *RUTA_DE_BLOQUES;
char *RUTA_DE_FILES;
char *RUTA_DE_POKEMON;
char *RUTA_DE_METADATA_MONTAJE;
t_config *config_fs;

// PARA INICIAR EL PUNTO DE MONTAJE
void iniciar_punto_de_montaje(char *puntoDeMontaje);
void inicio_default(char* puntoDeMontaje);
void carpetas_iniciar(char* puntoDeMontaje);
void carga_config_fs();
void bloques_iniciar();

// Utilidades varias

void mostrarRutas(void);
char* obtener_nombre_de_archivo(char *path);
char* obtener_direccion_metadata(char *path);
t_config* obtener_metadata_de_ruta(char *path);
int esDirectorio(char* ruta);

// Utilidades para pokemon
t_config* obtener_metadata_de_pokemon(char *nombrePokemon);
bool existePokemon(char* nombrePokemon);
void crearPokemon(char* nombrePokemon);
bool archivoAbierto(char* nombrePokemon);
void abrirArchivo(char* nombrePokemon);
void cerrarArchivo(char* nombrePokemon);


#endif /* GAMECARD_GAMECARD_H_ */
