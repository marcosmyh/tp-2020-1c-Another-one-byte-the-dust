/*
 * Serializacion.c
 *
 *  Created on: 13 abr. 2020
 *      Author: utnso
 */

#include "Serializacion.h"

Header recieveHeader(int socketCliente){
	void* buffer = malloc(sizeof(t_operacion) + sizeof(uint32_t));
	int result = recv(socketCliente, buffer, (sizeof(t_operacion) + sizeof(uint32_t)), MSG_WAITALL);
	if(result == 0 || result == -1){
		Header headerQueRetorna;
		headerQueRetorna.operaciones = (-1);
		headerQueRetorna.tamanioMensaje = 0;
		free(buffer);
		return headerQueRetorna;
	}
	uint32_t tamanioMensaje = 0;
	t_operacion operacion;
	memcpy(&operacion,buffer,sizeof(t_operacion));
	memcpy(&tamanioMensaje,buffer+(sizeof(t_operacion)), (sizeof(uint32_t)));
	free(buffer);
	Header headerQueRetorna;
	headerQueRetorna.operaciones = operacion;
	headerQueRetorna.tamanioMensaje = tamanioMensaje;
	return headerQueRetorna;
}

void* recieveAndUnpack(int socketCliente, uint32_t tamanio){
	void* retorno = malloc(tamanio);
	recv(socketCliente, retorno, tamanio, MSG_WAITALL);
	return retorno;
}

bool packAndSend(int socketCliente, const void* paquete, uint32_t tamPaquete, t_operacion operacion){
	uint32_t tamMensaje= tamPaquete + sizeof(t_operacion) + sizeof(uint32_t);
	void* buffer = malloc(tamMensaje);
	uint32_t desplazamiento = 0;
	memcpy(buffer, &operacion, sizeof(t_operacion));
	desplazamiento += sizeof(t_operacion);
	memcpy(buffer+desplazamiento, &tamPaquete, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, paquete, tamPaquete);
	desplazamiento += tamPaquete;
	if (desplazamiento != tamMensaje){
		return (-1);
	}
	int resultado = send(socketCliente, buffer, tamMensaje, 0);
	free(buffer);
	return resultado;
}

bool packAndSend_New(int socketCliente, char* pokemon, uint32_t cantidad, uint32_t coordenadaX, uint32_t coordenadaY){
	uint32_t tamMensaje = strlen(pokemon) + 1 + sizeof(cantidad) + sizeof(coordenadaX) + sizeof(coordenadaY);
	uint32_t tamPokemon = strlen(pokemon);
	void* buffer = malloc(tamMensaje);
	uint32_t desplazamiento = 0;
	memcpy(buffer, &tamPokemon, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, pokemon, tamPokemon);
	desplazamiento += tamPokemon;
	memcpy(buffer+desplazamiento, cantidad, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, coordenadaX, sizeof(uint32_t));
	desplazamiento +=sizeof(uint32_t);
	memcpy(buffer+desplazamiento, coordenadaY, sizeof(uint32_t));
	int resultado = packAndSend(socketCliente, buffer, tamMensaje, t_NEW);
	free(buffer);
	return resultado;
}
bool packAndSend_Localized(){

}
bool packAndSend_GET(){

}
bool packAndSend_Appeared(){

}
bool packAndSend_Catch(){

}
bool packAndSend_Caught(){

}
char* unpackPokemon(void* pack){
	uint32_t tamanioPokemon = 0;
	uint32_t desplazamiento = 0;
	memcpy(&tamanioPokemon, pack, sizeof(uint32_t));
	char* pokemon = malloc(tamanioPokemon);
	desplazamiento += sizeof(uint32_t);
	memcpy(pokemon, pack+desplazamiento,tamanioPokemon);
	return pokemon;
}
uint32_t unpackCantidadPokemons_New(void* pack, uint32_t tamanioPokemon){
	uint32_t cantPokemones = 0;
	uint32_t desplazamiento = tamanioPokemon;
	memcpy(&cantPokemones, pack+desplazamiento, sizeof(uint32_t));
	return cantPokemones;
}
uint32_t unpackCoordenadaX_New(void* pack, uint32_t tamanioPokemon){
	uint32_t coordenadaX = 0;
	uint32_t desplazamiento = (tamanioPokemon + sizeof(uint32_t));
	memcpy(&coordenadaX, pack+desplazamiento, sizeof(uint32_t));
	return coordenadaX;
}
uint32_t unpackCoordenadaY_New(void* pack, uint32_t tamanioPokemon){
	uint32_t coordenadaY = 0;
	uint32_t desplazamiento = (tamanioPokemon + sizeof(uint32_t) + sizeof(uint32_t));
	memcpy(&coordenadaY, pack+desplazamiento, sizeof(uint32_t));
	return coordenadaY;
}






