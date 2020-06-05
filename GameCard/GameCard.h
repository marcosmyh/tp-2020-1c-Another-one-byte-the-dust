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
#include<commons/bitarray.h>
#include<commons/config.h>
#include<commons/string.h>
#include<string.h>
#include<pthread.h>
#include "../Serializacion/Serializacion.h"

#include <sys/mman.h>

//VARIABLES


pthread_t thread;
t_log *logger; // ALGO QUE NO ESTA TAN BIEN CREO CREER
t_log *loggerObligatorio;
int socket_servidor;
t_config *archivoConfig;
char* ip_gc;
char* puerto_gc;
char* ip_broker;
char* puerto_broker;
int tiempo_de_reconexion;
int tiempo_de_reintento_operacion;
int tiempo_de_retardo;
char* path_de_tallgrass;
//int bloquesLibres;   --- en proxima actualizacion

//sem_t semDeBloquesLibres; --- en proxima actualizacion

void crearLogger(void);
void crearLoggerObligatorio();
int iniciar_servidor(char* ip, char* puerto, t_log* log);
int esperar_cliente(int socket_servidor, t_log* log);
void atender_cliente(int *socket);
void procesar_solicitud(Header headerRecibido, int cliente_fd);
void leerArchivoDeConfiguracion(void);
void setearValoresDeGame(t_config *confg);

///ESTRUCTURA AUXILIAR
struct arg_estructura {
    char* nombrePokemon;
    int tiempo;
};


////////////
// Sobre el FS
////////////

uint32_t TAMANIO_DE_BLOQUE;
uint32_t CANTIDAD_DE_BLOQUES;
char *MAGIC_NUMBER;
char *RUTA_DE_BLOQUES;
char *RUTA_DE_FILES;
char *RUTA_DE_POKEMON;
char *RUTA_DE_METADATA_MONTAJE;
t_config *config_fs;
t_bitarray* bitmap;

// PARA INICIAR EL PUNTO DE MONTAJE
void iniciar_punto_de_montaje(char *puntoDeMontaje);
void inicio_default(char* puntoDeMontaje);
void carpetas_iniciar(char* puntoDeMontaje);
void carga_config_fs();
void bloques_iniciar();
void iniciarBitmap();
void actualizarBitmap();

// Utilidades varias

void mostrarRutas(void);
char* obtener_nombre_de_archivo(char *path);
char* obtener_direccion_metadata(char *path);
t_config* obtener_metadata_de_ruta(char *path);
void limpiarPunteroAPuntero(char** puntero);
void mostrarBitmap();
int solicitarBloqueVacio();
int length_punteroAPuntero(char** punteroAPuntero);
char* crearPathDeBloque(char* nroDeBloque);
int obtenerLinea(char** lineas,char* array);
int buscarPosicionDeCaracter(char* array,char *caracter);
char* actualizarPosicion(char** posicionesSeparadas,char* lineaAActualizar,int nroDeLinea);
char* sumarCantidadPokemon(char* lineaPokemon,int cantidadASumar);
int desplazamientoDelArrayHastaCantPokemon(char** posicionesSeparadas,char* posObjetivo);
int bloqueAfectado(int desplazamiento);
int desplazamientoEnBloque(int desplazamiento);
int obtenerCantidad(char* lineaPokemon);
void sobreescribirUnCaracter(char* nombreDeBloque,int desplazamiento,char* caracterAEscribir);

// Manipulacion de archivos
int esDirectorio(char* ruta);
char* obtenerArrayDebloques(char* pokemon);
char* obtener_contenido_de_archivo(char* nroDeArchivo);
bool contieneEstaPosicion(char* lineas,char* arrayPosicion);
int escribirBloquePosicionandoPuntero(char* nombreDeBloque,char* stringAEscribir,int desplazamiento);

// cositar para new
void escribirEnArchivo(FILE *bloque, char* contenidoAagregar,int caracteresAEscribir,char* nombrePokemon);
void agregarNuevaPosicion(char* contenidoAagregar,char* bloques,char* nombrePokemon);
void actualizacionDeBloques(char** bloques,int desplazamientoDeCambio,char* posiciones,char* pokemon);
int anadirCantidad(char* posiciones,char* posicionBuscada,int cantidadASumar,char* bloquesString,char* pokemon);

// Utilidades para pokemon
t_config* obtener_metadata_de_pokemon(char *nombrePokemon);
int existePokemon(char* nombrePokemon);
void crearPokemon(char* nombrePokemon);
bool archivoAbierto(char* nombrePokemon);
void abrirArchivo(char* nombrePokemon);
void cerrarArchivo(char* nombrePokemon);
void editarTamanioPokemon(char* nombrePokemon,int cantidad);
void agregarBloqueAPokemon(char* nombrePokemon,int numeroDeBloque);

// PROCEDIMIENTOS
void procedimientoNEW(uint32_t idMensaje,char* pokemon,uint32_t posx,uint32_t posy,uint32_t cantidad);


// ENVIO DE MENSAJES
int envioDeMensajeCaught(uint32_t id,uint32_t resultado);
int envioDeMensajeGet(char* pokemon,uint32_t idmensaje);
int envioDeMensajeAppeared(char* pokemon, uint32_t posx, uint32_t posy, uint32_t idmensaje);
int crear_conexion(char *ip, char* puerto);
#endif /* GAMECARD_GAMECARD_H_ */
