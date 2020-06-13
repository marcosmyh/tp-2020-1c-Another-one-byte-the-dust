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

typedef enum{
	t_HANDSHAKE,
	t_NEW,
	t_LOCALIZED,
	t_GET,
	t_APPEARED,
	t_CATCH,
	t_CAUGHT,
	t_ACK,
	t_ID
} t_operacion;

typedef struct {
	uint32_t tamanioMensaje;
	t_operacion operacion;
}__attribute__((packed)) Header;


//FUNCIONES

Header receiveHeader(int socketCliente); //DESEMPAQUETA EL HEADER DE CUALQUIER PAQUETE PARA OBTENER EL CODIGO DE OPERACION

void* receiveAndUnpack(int socketCliente, uint32_t tamanioMensaje);//DEVUELVE EL PAQUETE ENTERO,DESPUES HAY QUE DESEMPAQUETAR CADA UNA DE LAS COSAS QUE LO COMPONEN

bool packAndSend(int socketCliente, const void* paquete, uint32_t tamPaquete, t_operacion operacion); //EMPAQUETA UN PAQUETE CUALQUIERA Y LO ENVIA

char* unpackPokemonGet(void* pack);
char *unpackPokemonNew(void *pack);
char *unpackPokemonCatch(void *pack);
char *unpackPokemonAppeared(void *pack);
char *unpackPokemonLocalized(void *pack);

uint32_t unpackID(void* pack); //DESEMPAQUETA EL ID DEL MENSAJE, ES IGUAL PARA TODOS, SI NO MANDAS UN ID EMPAQUETALE UN -1

uint32_t unpackIDCorrelativo(void *pack);


//FUNCIONES PARA HANDSHAKE
void* pack_Handshake(char* proceso, t_operacion operacion);

char* unpackProceso(void* pack); //ESTE SIRVE PARA DESEMPAQUETAR EL IDENTIFICADOR DEL PROCESO, EJ: "TEAM-1" o "GAMECARD-4"

//unpackOperacion es utilizado únicamente por el broker para saber a que cola deberá suscribir al
//proceso que se conecta por primera vez.
t_operacion unpackOperacion(void* pack, uint32_t tamanioProceso);


//FUNCIONES PARA NEW

void* pack_New(uint32_t id, char* pokemon, uint32_t cantidad, uint32_t coordenadaX, uint32_t coordenadaY);

uint32_t unpackCantidadPokemons_New(void* pack, uint32_t tamanioPokemon);
uint32_t unpackCoordenadaX_New(void* pack, uint32_t tamanioPokemon);
uint32_t unpackCoordenadaY_New(void* pack, uint32_t tamanioPokemon);


//FUNCIONES PARA LOCALIZED

void* pack_Localized(uint32_t id, char* pokemon, uint32_t cantidadParesCoordenadas, uint32_t arrayCoordenadas[]);
void packCoordenada_Localized(void* buffer, uint32_t desplazamiento, uint32_t coordenada);

uint32_t unpackCantidadParesCoordenadas_Localized(void* pack, uint32_t tamanioPokemon);
uint32_t unpackCoordenadaX_Localized(void* pack, uint32_t desplazamiento);
uint32_t unpackCoordenadaY_Localized(void* pack, uint32_t desplazamiento);


//FUNCIONES PARA GET

void* pack_Get(uint32_t id, char* pokemon);

//FUNCIONES PARA APPEARED

void* pack_Appeared(uint32_t idCorrelativo, char* pokemon, uint32_t coordenadaX, uint32_t coordenadaY);

uint32_t unpackCoordenadaX_Appeared(void* pack, uint32_t tamanioPokemon);
uint32_t unpackCoordenadaY_Appeared(void* pack, uint32_t tamanioPokemon);


//FUNCIONES PARA CATCH

void* pack_Catch(uint32_t id, char* pokemon, uint32_t coordenadaX, uint32_t coordenadaY);

uint32_t unpackCoordenadaX_Catch(void* pack, uint32_t tamanioPokemon);
uint32_t unpackCoordenadaY_Catch(void* pack, uint32_t tamanioPokemon);


//FUNCIONES PARA CAUGHT

void* pack_Caught(uint32_t id, uint32_t atrapado);

bool unpackResultado_Caught(void* pack);


//FUNCIONES PARA ID

void* pack_ID(uint32_t ID, t_operacion operacion);

//FUNCIONES PARA ACK

void* pack_Ack(uint32_t ID, t_operacion operacion, char* identificadorProceso);


//unpackOperacionID obtiene la operación de un paquete con código de operación t_ID
t_operacion unpackOperacionID(void* pack);

//unpackOperacionACK obtiene la operación de un paquete con código de operación t_ACK
t_operacion unpackOperacionACK(void *pack);

char* unpackIdentificadorProcesoACK(void* pack);
