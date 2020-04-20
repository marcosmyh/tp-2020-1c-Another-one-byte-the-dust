#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

//ESTRUCTURAS

typedef enum t_operaciones{
	t_HANDSHAKE,
	t_NEW,
	t_LOCALIZED,
	t_GET,
	t_APPEARED,
	t_CATCH,
	t_CAUGHT
} t_operacion;

typedef struct {
	uint32_t tamanioMensaje;
	t_operacion operaciones;
}__attribute__((packed)) Header;


//FUNCIONES

Header receiveHeader(int socketCliente); //DESEMPAQUETA EL HEADER DE CUALQUIER PAQUETE PARA OBTENER EL CODIGO DE OPERACION

void* receiveAndUnpack(int socketCliente, uint32_t tamanioMensaje);//DEVUELVE EL PAQUETE ENTERO,DESPUES HAY QUE DESEMPAQUETAR CADA UNA DE LAS COSAS QUE LO COMPONEN

bool packAndSend(int socketCliente, const void* paquete, uint32_t tamPaquete, t_operacion operacion); //EMPAQUETA UN PAQUETE CUALQUIERA Y LO ENVIA

char* unpackPokemon(void* pack); //DESEMPAQUETA EL NOMBRE DEL POKEMON, ES IGUAL PARA TODOS

//FUNCIONES PARA HANDSHAKE
bool packAndSend_Handshake(int socketCliente, char* proceso, t_operacion operacion);

char* unpackProceso(void* pack);
t_operacion unpackOperacion(void* pack, uint32_t tamanioProceso);


//FUNCIONES PARA NEW

bool packAndSend_New(int socketCliente, char* pokemon, uint32_t cantidad, uint32_t coordenadaX, uint32_t coordenadaY);

uint32_t unpackCantidadPokemons_New(void* pack, uint32_t tamanioPokemon);
uint32_t unpackCoordenadaX_New(void* pack, uint32_t tamanioPokemon);
uint32_t unpackCoordenadaY_New(void* pack, uint32_t tamanioPokemon);


//FUNCIONES PARA LOCALIZED

bool packAndSend_Localized(int socketCliente, char* pokemon, uint32_t cantidadParesCoordenadas, uint32_t arrayCoordenadas[]);
void packCoordenada_Localized(void* buffer, uint32_t desplazamiento, uint32_t coordenada);

uint32_t unpackCantidadParesCoordenadas_Localized(void* pack, uint32_t tamanioPokemon);
uint32_t unpackCoordenadaX_Localized(void* pack, uint32_t desplazamiento);
uint32_t unpackCoordenadaY_Localized(void* pack, uint32_t desplazamiento);


//FUNCIONES PARA GET

bool packAndSend_Get(int socketCliente, char* pokemon);


//FUNCIONES PARA APPEARED

bool packAndSend_Appeared(int socketCliente, char* pokemon, uint32_t coordenadaX, uint32_t coordenadaY);

uint32_t unpackCoordenadaX_Appeared(void* pack, uint32_t tamanioPokemon);
uint32_t unpackCoordenadaY_Appeared(void* pack, uint32_t tamanioPokemon);


//FUNCIONES PARA CATCH

bool packAndSend_Catch(int socketCliente, char* pokemon, uint32_t coordenadaX, uint32_t coordenadaY);

uint32_t unpackCoordenadaX_Catch(void* pack, uint32_t tamanioPokemon);
uint32_t unpackCoordenadaY_Catch(void* pack, uint32_t tamanioPokemon);


//FUNCIONES PARA CAUGHT

bool packAndSend_Caught(int socketCliente, uint32_t atrapado);

bool unpackResultado_Caught(void* pack);
