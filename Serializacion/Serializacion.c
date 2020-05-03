/*
 * Serializacion.c
 *
 *  Created on: 13 abr. 2020
 *      Author: utnso
 */

#include "Serializacion.h"

Header receiveHeader(int socketCliente){
	void* buffer = malloc(sizeof(t_operacion) + sizeof(uint32_t));
	int result = recv(socketCliente, buffer, (sizeof(t_operacion) + sizeof(uint32_t)), MSG_WAITALL);
	if(result == 0 || result == -1){
		Header headerQueRetorna;
		headerQueRetorna.operacion = (-1);
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
	headerQueRetorna.operacion = operacion;
	headerQueRetorna.tamanioMensaje = tamanioMensaje;
	return headerQueRetorna;
}

void* receiveAndUnpack(int socketCliente, uint32_t tamanioMensaje){
	void* retorno = malloc(tamanioMensaje);
	recv(socketCliente, retorno, tamanioMensaje, MSG_WAITALL);
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

bool packAndSend_Handshake(int socketCliente, char* proceso, t_operacion operacion){
	uint32_t tamMensaje = strlen(proceso) + 1 + sizeof(t_operacion);
	uint32_t tamProceso = strlen(proceso);
	void* buffer = malloc(tamMensaje);
	uint32_t desplazamiento = 0;
	memcpy(buffer, &tamProceso, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, proceso, tamProceso);
	desplazamiento += tamProceso;
	memcpy(buffer+desplazamiento, &operacion, sizeof(t_operacion));
	int resultado = packAndSend(socketCliente, buffer, tamMensaje, t_HANDSHAKE);
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
	memcpy(buffer+desplazamiento, &cantidad, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, &coordenadaX, sizeof(uint32_t));
	desplazamiento +=sizeof(uint32_t);
	memcpy(buffer+desplazamiento, &coordenadaY, sizeof(uint32_t));
	int resultado = packAndSend(socketCliente, buffer, tamMensaje, t_NEW);
	free(buffer);
	return resultado;
}

bool packAndSend_Localized(int socketCliente, char* pokemon, uint32_t cantidadParesCoordenadas, uint32_t arrayCoordenadas[]){
	//Revisar como obtener el tamaño del array
	uint32_t tamMensaje = strlen(pokemon) + 1 + sizeof(cantidadParesCoordenadas) + sizeof(arrayCoordenadas) + (sizeof(uint32_t)*2*cantidadParesCoordenadas);
	uint32_t tamPokemon = strlen(pokemon);
	void* buffer = malloc(tamMensaje);
	uint32_t desplazamiento = 0;
	memcpy(buffer, &tamPokemon, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, pokemon, tamPokemon);
	desplazamiento += tamPokemon;
	memcpy(buffer+desplazamiento, &cantidadParesCoordenadas, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	for(int i=0; i<(cantidadParesCoordenadas*2); i++){
		packCoordenada_Localized(buffer, desplazamiento, arrayCoordenadas[i]);
	}
	int resultado = packAndSend(socketCliente, buffer, tamMensaje, t_LOCALIZED);
	free(buffer);
	return resultado;
}

void packCoordenada_Localized(void* buffer, uint32_t desplazamiento, uint32_t coordenada){
	memcpy(buffer+desplazamiento, &coordenada, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
}

bool packAndSend_Get(int socketCliente, char* pokemon){
	uint32_t tamMensaje = strlen(pokemon) + 1;
	uint32_t tamPokemon = strlen(pokemon);
	void* buffer = malloc(tamMensaje);
	uint32_t desplazamiento = 0;
	memcpy(buffer, &tamPokemon, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, pokemon, tamPokemon);
	int resultado = packAndSend(socketCliente, buffer, tamMensaje, t_GET);
	free(buffer);
	return resultado;
}

bool packAndSend_Appeared(int socketCliente, char* pokemon, uint32_t coordenadaX, uint32_t coordenadaY){
	uint32_t tamMensaje = strlen(pokemon) + 1 + sizeof(uint32_t) + sizeof(uint32_t);
	uint32_t tamPokemon = strlen(pokemon);
	void* buffer = malloc(tamMensaje);
	uint32_t desplazamiento = 0;
	memcpy(buffer, &tamPokemon, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, pokemon, tamPokemon);
	desplazamiento += tamPokemon;
	memcpy(buffer+desplazamiento, &coordenadaX, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, &coordenadaY, sizeof(uint32_t));
	int resultado = packAndSend(socketCliente, buffer, tamMensaje, t_APPEARED);
	free(buffer);
	return resultado;
}

bool packAndSend_Catch(int socketCliente, char* pokemon, uint32_t coordenadaX, uint32_t coordenadaY){
	uint32_t tamMensaje = strlen(pokemon) + 1 + sizeof(uint32_t) + sizeof(uint32_t);
	uint32_t tamPokemon = strlen(pokemon);
	void* buffer = malloc(tamMensaje);
	uint32_t desplazamiento = 0;
	memcpy(buffer, &tamPokemon, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, pokemon, tamPokemon);
	desplazamiento += tamPokemon;
	memcpy(buffer+desplazamiento, &coordenadaX, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer+desplazamiento, &coordenadaY, sizeof(uint32_t));
	int resultado = packAndSend(socketCliente, buffer, tamMensaje, t_APPEARED);
	free(buffer);
	return resultado;
}

bool packAndSend_Caught(int socketCliente, uint32_t atrapado){
	uint32_t tamMensaje = sizeof(uint32_t);
	void* buffer= malloc(tamMensaje);
	memcpy(buffer, &atrapado, sizeof(uint32_t));
	int resultado = packAndSend(socketCliente, buffer, tamMensaje, t_CAUGHT);
	free(buffer);
	return resultado;
}

bool packAndSend_Ack(int socketCliente, bool acknowledgement){
	uint32_t tamMensaje = sizeof(acknowledgement);
	uint32_t tamAck = sizeof(acknowledgement);
	void* buffer = malloc(tamMensaje);
	memcpy(buffer, &acknowledgement, tamAck);
	int resultado = packAndSend(socketCliente, buffer, tamMensaje, t_ACK);
	free(buffer);
	return resultado;
}

bool packAndSend_ID(int socketCliente, uint32_t ID){
	uint32_t tamMensaje = sizeof(uint32_t);
	uint32_t tamID = sizeof(uint32_t);
	void* buffer = malloc(tamMensaje);
	memcpy(buffer, &ID, tamID);
	int resultado = packAndSend(socketCliente, buffer, tamMensaje, t_ID);
	free(buffer);
	return resultado;
}

//UNPACK NOMBRE POKEMON (IGUAL PARA TODOS)

char* unpackPokemon(void* pack){
	uint32_t tamanioPokemon = 0;
	uint32_t desplazamiento = 0;
	memcpy(&tamanioPokemon, pack, sizeof(uint32_t));
	char* pokemon = malloc(tamanioPokemon);
	desplazamiento += sizeof(uint32_t);
	memcpy(pokemon, pack+desplazamiento,tamanioPokemon);
	return pokemon;
}

 //UNPACK HANDSHAKE

char* unpackProceso(void* pack){
	uint32_t tamanioProceso = 0;
	uint32_t desplazamiento = 0;
	memcpy(&tamanioProceso, pack, sizeof(uint32_t));
	char* proceso = malloc(tamanioProceso);
	desplazamiento += sizeof(uint32_t);
	memcpy(proceso, pack+desplazamiento, tamanioProceso);
	return proceso;
}

t_operacion unpackOperacion(void* pack, uint32_t tamanioProceso){
	t_operacion operacion;
	uint32_t desplazamiento = tamanioProceso + sizeof(uint32_t);
	memcpy(&operacion, pack+desplazamiento, sizeof(t_operacion));
	return operacion;
}

 //UNPACK NEW

uint32_t unpackCantidadPokemons_New(void* pack, uint32_t tamanioPokemon){
	uint32_t cantPokemones = 0;
	uint32_t desplazamiento = tamanioPokemon + sizeof(uint32_t);
	memcpy(&cantPokemones, pack+desplazamiento, sizeof(uint32_t));
	return cantPokemones;
}
uint32_t unpackCoordenadaX_New(void* pack, uint32_t tamanioPokemon){
	uint32_t coordenadaX = 0;
	uint32_t desplazamiento = (tamanioPokemon + sizeof(uint32_t) + sizeof(uint32_t));
	memcpy(&coordenadaX, pack+desplazamiento, sizeof(uint32_t));
	return coordenadaX;
}
uint32_t unpackCoordenadaY_New(void* pack, uint32_t tamanioPokemon){
	uint32_t coordenadaY = 0;
	uint32_t desplazamiento = (tamanioPokemon + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t));
	memcpy(&coordenadaY, pack+desplazamiento, sizeof(uint32_t));
	return coordenadaY;
}

// UNPACK LOCALIZED

uint32_t unpackCantidadParesCoordenadas_Localized(void* pack, uint32_t tamanioPokemon){
	uint32_t cantParesCoordenadas = 0;
	uint32_t desplazamiento = tamanioPokemon + sizeof(uint32_t);
	memcpy(&cantParesCoordenadas, pack+desplazamiento, sizeof(uint32_t));
	return cantParesCoordenadas;
}
uint32_t unpackCoordenadaX_Localized(void* pack, uint32_t desplazamiento){
	uint32_t coordenadaX = 0;
	memcpy(&coordenadaX, pack+desplazamiento, sizeof(uint32_t));
	return coordenadaX;
}
uint32_t unpackCoordenadaY_Localized(void* pack, uint32_t desplazamiento){
	uint32_t coordenadaY = 0;
	memcpy(&coordenadaY, pack+desplazamiento, sizeof(uint32_t));
	return coordenadaY;
}

// UNPACK APPEARED

uint32_t unpackCoordenadaX_Appeared(void* pack, uint32_t tamanioPokemon){
	uint32_t coordenadaX = 0;
	uint32_t desplazamiento = (tamanioPokemon + sizeof(uint32_t));
	memcpy(&coordenadaX, pack+desplazamiento, sizeof(uint32_t));
	return coordenadaX;
}
uint32_t unpackCoordenadaY_Appeared(void* pack, uint32_t tamanioPokemon){
	uint32_t coordenadaY = 0;
	uint32_t desplazamiento = (tamanioPokemon + sizeof(uint32_t) + sizeof(uint32_t));
	memcpy(&coordenadaY, pack+desplazamiento, sizeof(uint32_t));
	return coordenadaY;
}

// UNPACK CATCH

uint32_t unpackCoordenadaX_Catch(void* pack, uint32_t tamanioPokemon){
	uint32_t coordenadaX = 0;
	uint32_t desplazamiento = (tamanioPokemon + sizeof(uint32_t));
	memcpy(&coordenadaX, pack+desplazamiento, sizeof(uint32_t));
	return coordenadaX;
}
uint32_t unpackCoordenadaY_Catch(void* pack, uint32_t tamanioPokemon){
	uint32_t coordenadaY = 0;
	uint32_t desplazamiento = (tamanioPokemon + sizeof(uint32_t) + sizeof(uint32_t));
	memcpy(&coordenadaY, pack+desplazamiento, sizeof(uint32_t));
	return coordenadaY;
}

// UNPACK CAUGHT

bool unpackResultado_Caught(void* pack){
	bool atrapado;
	memcpy(&atrapado, pack, sizeof(uint32_t));
	return atrapado;
}

// UNPACK ACK

bool unpackACK(void* pack){
	bool acknowledgement;
	memcpy(&acknowledgement, pack, sizeof(bool));
	return acknowledgement;
}

// UNPACK ID

uint32_t unpackID(void* pack){
	uint32_t ID;
	memcpy(&ID, pack, sizeof(uint32_t));
	return ID;
}

