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
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

//VARIABLES

pthread_t thread;
t_log *logger; // ALGO QUE NO ESTA TAN BIEN CREO CREER
t_log *loggerObligatorio;
t_config *archivoConfig;
char* ip_gc;
char* puerto_gc;
char* ip_broker;
char* puerto_broker;
int tiempo_de_reconexion;
int tiempo_de_reintento_operacion;
int tiempo_de_retardo;
char* path_de_tallgrass;
int bloquesLibres; 


int socket_new;
int socket_catch;
int socket_get;
int socket_servidor;


bool conexionNew = 0;
bool conexionCatch = 0;
bool conexionGet = 0;
char* identificadorProcesoGC = NULL;

pthread_t hiloAtencionCatch;
pthread_t hiloAtencionNew;
pthread_t hiloAtencionGet;
pthread_t hiloAtencionGB;
pthread_t hiloSuscripcionABroker;

sem_t semDeBloques;
sem_t reconexion;
sem_t aperturaDeArchivo;
sem_t conexionRecuperadaDeNew;
sem_t conexionRecuperadaDeCatch;
sem_t conexionRecuperadaDeGet;


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

typedef struct{
	int socket;
	void *paquete;
}t_infoPack;

////////////
// Carga default del FS
////////////
char* blocksizedefault = "64";
char* blocksdefault =  "4096";


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
void setearValoresDefaultfs(int cantidadDeArgumentos,char* argumentos[]);
// Utilidades varias

void mostrarRutas(void);
char* agregar_metadata_a_path(char *path);
t_config* obtener_metadata_de_ruta(char *path);
void limpiarPunteroAPuntero(char** puntero);
void mostrarBitmap();
int solicitarBloqueVacio();
int length_punteroAPuntero(char** punteroAPuntero);
char* crearPathDeBloque(char* nroDeBloque);
int obtenerLinea(char** lineas,char* array);
int buscarPosicionDeCaracter(char* array,char caracter);
char* actualizarPosicion(char** posicionesSeparadas,char* lineaAActualizar,int nroDeLinea);
char* sumarCantidadPokemon(char* lineaPokemon,int cantidadASumar);
int desplazamientoDelArrayHastaCantPokemon(char** posicionesSeparadas,char* posObjetivo);
int bloqueAfectado(int desplazamiento);
int desplazamientoEnBloque(int desplazamiento);
int obtenerCantidad(char* lineaPokemon);
void sobreescribirUnCaracter(char* nombreDeBloque,int desplazamiento,char* caracterAEscribir);
int bloquesNecesariosParaEscribir(char* datos);

// Manipulacion de archivos
int esDirectorio(char* ruta);
char* obtener_contenido_de_bloque(char* nroDeBloque);
bool contieneEstaPosicion(char* lineas,char* arrayPosicion);
int escribirBloquePosicionandoPuntero(char* nombreDeBloque,char* stringAEscribir,int desplazamiento);
void crearDirectorio(char* pathDeDirectorio);
int verificarYAbrirArchivo(char* pokemon);
int espacioLibreDeBloque(char* nombreDeBloque);
int tamanioBloque(char* nombreDeBloque);
char* obtenerContenidoDeArchivo(char* bloques);

// cositar para new
int agregarNuevaPosicion(char* contenidoAagregar,char* bloques,char* nombrePokemon);
void actualizacionDeBloques(int desplazamientoDeCambio,char* posiciones,char* pokemon);
int anadirCantidad(char* posiciones,char* posicionBuscada,int cantidadASumar,char* bloquesString,char* pokemon);

//Cosas para Catch
void disminuirCantidad(char* posiciones,char* posicionBuscada,char* bloquesString,char* pokemon);


// Utilidades para pokemon
t_config* obtener_metadata_de_pokemon(char *nombrePokemon);
int existePokemon(char* nombrePokemon);
void crearPokemon(char* nombrePokemon);
bool archivoAbierto(char* nombrePokemon);
void abrirArchivo(char* nombrePokemon);
void cerrarArchivo(char* nombrePokemon);
void editarTamanioPokemon(char* nombrePokemon,int cantidad);
void agregarBloqueAPokemon(char* nombrePokemon,int numeroDeBloque);
char* obtenerArrayDebloques(char* pokemon);
int obtenerTamanioDePokemon(char* pokemon);

// PROCEDIMIENTOS
int procedimientoNEW(char* pokemon,uint32_t posx,uint32_t posy,uint32_t cantidad);
int procedimientoCATCH(char* pokemon,uint32_t posx,uint32_t posy);
int procedimientoGET(uint32_t idMensaje,char* pokemon);

// ENVIO DE MENSAJES
int envioDeMensajeCaught(uint32_t id,uint32_t resultado);
int envioDeMensajeGet(char* pokemon,uint32_t idmensaje);
int envioDeMensajeAppeared(char* pokemon, uint32_t posx, uint32_t posy, uint32_t idmensaje);
int crear_conexion(char *ip, char* puerto);
int envioDeMensajeLocalized(char* pokemon,uint32_t idmensaje,uint32_t cantidadParesCoordenadas,uint32_t arrayCoordenadas[]);

void iniciarGestionMensajesGC();
void suscripcionColaNew();
void suscripcionColaCatch();
void suscripcionColaGet();
void gestionMensajesGet();
void gestionMensajesCatch();
void gestionMensajesNew();
void administrarSuscripcionesBroker();
void conexionAColaNew();
void conexionAColaCatch();
void conexionAColaGet();
void reconexionColaNew();
int conectarseAColaMensajes(int socket,char *identificador,t_operacion operacion);
void gestionMensajesGB(int* socket_servidor);
bool esSocketDeBroker(int socket);
void procedimientoMensajeCatch(t_infoPack *infoNew);
void procedimientoMensajeGet(t_infoPack *infoNew);
void procedimientoMensajeNew(t_infoPack *infoNew);
t_infoPack *crearInfoDePaquete(int socket,void *paquete);
void envioDeACK(uint32_t id, t_operacion operacion);
void recibirHandshake(int socket,uint32_t tamanioPaquete);
#endif /* GAMECARD_GAMECARD_H_ */


char *getToken(char *,char);
char *getCoordenadaX(char *);
char *getCoordenadaY(char *);
t_list *guardarCoordenadas(char**,int);
uint32_t generarArrayDeCoordenadas(char** coordenadasSeparadas);
void mostrarCoordenadas(t_list* lista);
void insertarCoordenadas(t_list *lista,uint32_t* vector);
void imprimirContenido(uint32_t *vector,uint32_t size);
