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

Header recieveHeader(int socketCliente); //DESEMPAQUETA EL HEADER DE CUALQUIER PAQUETE PARA OBTENER EL CODIGO DE OPERACION
void* recieveAndUnpack(int socketCliente, uint32_t tamanio);//DEVUELVE EL PAQUETE ENTERO,DESPUES HAY QUE DESEMPAQUETAR CADA UNA DE LAS COSAS QUE LO COMPONEN
bool packAndSend(int socketCliente, const void* paquete, uint32_t tamPaquete, t_operacion operacion); //EMPAQUETA UN PAQUETE CUALQUIERA Y LO ENVIA
