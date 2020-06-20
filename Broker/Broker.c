#include "Broker.h"

int main(void) {
    config = crearConfig();
    logger = crearLogger();
    setearValoresConfig();
    inicializarColas();
    inicializarListasSuscriptores();
    inicializarSemaforos();
    iniciarMemoria();

    socket_servidor = iniciar_servidor(ip,puerto);

    //TODO
    //Crear un hiloAtencion..

    while(1){
       	int socket = esperar_cliente(socket_servidor);
    	int *socket_cliente = malloc(sizeof(int));
    	*socket_cliente = socket;

    	pthread_create(&hiloAtencionCliente,NULL,(void*)atender_cliente,socket_cliente);
        pthread_detach(hiloAtencionCliente);
    }

    destruirSemaforos();
    liberarRecursosAdministracionMemoria();
    destruirColas();
    destruirListasSuscriptores();
    config_destroy(config);
    log_destroy(logger);
	return 0;
}

t_log *crearLogger(){
    char *path = "/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/Broker/Broker.log";

    return log_create(path,"Broker",1,LOG_LEVEL_INFO);
}

t_config *crearConfig(){
	char *path = "/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/Broker/Broker.config";
	return config_create(path);
}

int iniciar_servidor(char *ip, char *puerto){
	int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip, puerto, &hints, &servinfo);

    for (p=servinfo; p != NULL; p = p->ai_next)
    {
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            continue;
        }
        break;
    }

	listen(socket_servidor, SOMAXCONN);

    freeaddrinfo(servinfo);

    log_info(logger, "Listo para recibir procesos!");

    return socket_servidor;
}

int esperar_cliente(int socket_servidor){
	struct sockaddr_in dir_cliente;

	int tam_direccion = sizeof(struct sockaddr_in);
	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);
	log_info(logger,"Se conecto un proceso al Broker.");
	return socket_cliente;
}

void atender_cliente(int *socket){
    Header header;
    header = receiveHeader(*socket);
    procesar_solicitud(header,*socket);
}

void procesar_solicitud(Header header,int cliente_fd){
     uint32_t sizePaquete = header.tamanioMensaje;
     int codigo_operacion = header.operacion;
     t_mensaje *mensaje;
     void *paquete;

     //Determinar cuando doy de de baja a un suscriptor

     void enviarMensajeA(t_suscriptor *suscriptor){
    	 int socket = suscriptor->socket_suscriptor;
    	 char *identificadorProceso = suscriptor->identificadorProceso;
    	 packAndSend(socket,paquete,sizePaquete,codigo_operacion);

    	 list_add(mensaje->suscriptoresALosQueMandeMensaje,suscriptor);

    	 log_info(logger,"Le mande un mensaje a %s",identificadorProceso);
     }

     switch (codigo_operacion) {
     		case t_HANDSHAKE:;
     		    t_operacion operacionDeSuscripcion;
     		    char *proceso;
     		    char *identificadorProceso;
     		    void *paqueteHANDSHAKE;

     			paquete = receiveAndUnpack(cliente_fd,sizePaquete);
     			proceso = unpackProceso(paquete);

     			uint32_t sizeProceso = strlen(proceso) + 1;
     			operacionDeSuscripcion = unpackOperacion(paquete,sizeProceso);

     			if(esPrimeraConexion(proceso)){
     				identificadorProceso = asignarIDProceso(proceso);
     				paqueteHANDSHAKE = pack_Handshake(identificadorProceso,operacionDeSuscripcion);
     				uint32_t sizePaquete = strlen(identificadorProceso) + 1 + sizeof(uint32_t) + sizeof(t_operacion);
     				packAndSend(cliente_fd,paqueteHANDSHAKE,sizePaquete,t_HANDSHAKE);
    			}
     			else{
    				identificadorProceso = proceso;
    				packAndSend(cliente_fd,paquete,sizePaquete,t_HANDSHAKE);
     			}

     			suscribirProceso(identificadorProceso,cliente_fd,operacionDeSuscripcion);

     			free(paquete);

     			break;

     		case t_NEW:;

     		    paquete = receiveAndUnpack(cliente_fd,sizePaquete);

     		    //Este paquete es el que se va a guardar en la cola.
     		    void *paqueteNewSinID = quitarIDPaquete(paquete,sizePaquete);

                //Asigno un ID al mensaje recibido
                uint32_t ID_NEW = asignarIDMensaje();

                void *paqueteID_New = pack_ID(ID_NEW,t_NEW);

                uint32_t sizePaqueteID_New = sizeof(ID_NEW) + sizeof(t_NEW);

                //Le envio el ID al productor.
                packAndSend(cliente_fd,paqueteID_New,sizePaqueteID_New,t_ID);

                //Ahora armo el paquete que van a recibir los suscriptores (consumidores)
                void *paqueteNEW = insertarIDEnPaquete(ID_NEW,paqueteNewSinID,sizePaquete,0);

                paquete = paqueteNEW;

                mensaje = crearMensaje(paqueteNewSinID,ID_NEW,-1,sizePaquete - sizeof(uint32_t));

     			agregarMensajeACola(mensaje,NEW_POKEMON,"NEW_POKEMON");

     			enviarMensajeRecibidoASuscriptores(suscriptores_NEW_POKEMON,enviarMensajeA);

     			free(paqueteNEW);
     			free(paqueteID_New);

     			break;

     		case t_LOCALIZED:;
     		    paquete = receiveAndUnpack(cliente_fd,sizePaquete);

     		    uint32_t ID_LOCALIZED_Correlativo = unpackID(paquete);

     		    if(existeRespuestaEnCola(ID_LOCALIZED_Correlativo,LOCALIZED_POKEMON)){
     		    	log_info(logger,"El mensaje con ID Correlativo [%d] ya existe en la cola LOCALIZED_POKEMON",ID_LOCALIZED_Correlativo);
     		    	free(paquete);
     		    }
     		    else{
     		    	uint32_t ID_LOCALIZED_Generado = asignarIDMensaje();

     		    	void *paqueteLocalizedSinID = quitarIDPaquete(paquete,sizePaquete);

     		    	void *paqueteID_Localized = pack_ID(ID_LOCALIZED_Generado,t_LOCALIZED);

     		    	uint32_t sizePaqueteID_Localized = sizeof(uint32_t) + sizeof(t_operacion);

     		    	packAndSend(cliente_fd,paqueteID_Localized,sizePaqueteID_Localized,t_ID);

     		    	void *paqueteLocalized = insertarIDEnPaquete(ID_LOCALIZED_Generado,paquete,sizePaquete,DOUBLE_ID);

     		    	paquete = paqueteLocalized;

     		    	mensaje = crearMensaje(paqueteLocalizedSinID,ID_LOCALIZED_Generado,ID_LOCALIZED_Correlativo,sizePaquete - sizeof(uint32_t));

     		    	agregarMensajeACola(mensaje,LOCALIZED_POKEMON,"LOCALIZED_POKEMON");

     		    	sizePaquete = sizePaquete + sizeof(uint32_t);

     		    	enviarMensajeRecibidoASuscriptores(suscriptores_LOCALIZED_POKEMON,enviarMensajeA);

     		    	free(paquete);
     		    	free(paqueteID_Localized);
     		    }

     			break;

     		case t_GET:;

     		    paquete = receiveAndUnpack(cliente_fd,sizePaquete);

     		    void *paqueteGetSinID = quitarIDPaquete(paquete,sizePaquete);

     		    uint32_t ID_GET = asignarIDMensaje();

     		    void *paqueteID_Get = pack_ID(ID_GET,t_GET);

     		    uint32_t sizePaqueteID_Get = sizeof(uint32_t) + sizeof(t_operacion);

     		    packAndSend(cliente_fd,paqueteID_Get,sizePaqueteID_Get,t_ID);

     		    void *paqueteGET = insertarIDEnPaquete(ID_GET,paqueteGetSinID,sizePaquete,0);

     		    paquete = paqueteGET;

     		    mensaje = crearMensaje(paqueteGetSinID,ID_GET,-1,sizePaquete - sizeof(uint32_t));

     		    agregarMensajeACola(mensaje,GET_POKEMON,"GET_POKEMON");

     		    enviarMensajeRecibidoASuscriptores(suscriptores_GET_POKEMON,enviarMensajeA);

     		    free(paqueteGET);
     		    free(paqueteID_Get);

     			break;

     		case t_APPEARED:;

     		    paquete = receiveAndUnpack(cliente_fd,sizePaquete);

     		    uint32_t ID_APPEARED_Correlativo = unpackID(paquete);

     		    if(existeRespuestaEnCola(ID_APPEARED_Correlativo,APPEARED_POKEMON)){
     		    	log_info(logger,"El mensaje con ID Correlativo [%d] ya existe en la cola APPEARED POKEMON",ID_APPEARED_Correlativo);
     		    	free(paquete);
     		    }
     		    else{

     		    uint32_t ID_APPEARED_Generado = asignarIDMensaje();

     		    void *paqueteAppearedSinID = quitarIDPaquete(paquete,sizePaquete);

                void *paqueteID_Appeared = pack_ID(ID_APPEARED_Generado,t_APPEARED);

                uint32_t sizePaqueteID_Appeared = sizeof(uint32_t) + sizeof(t_operacion);

                //Le envio el ID al productor (que es el mismo ID que vino seteado en el paquete).
                packAndSend(cliente_fd,paqueteID_Appeared,sizePaqueteID_Appeared,t_ID);

                void *paqueteAppeared = insertarIDEnPaquete(ID_APPEARED_Generado,paquete,sizePaquete,DOUBLE_ID);

                paquete = paqueteAppeared;

                mensaje = crearMensaje(paqueteAppearedSinID,ID_APPEARED_Generado,ID_APPEARED_Correlativo,sizePaquete - sizeof(uint32_t));

                agregarMensajeACola(mensaje,APPEARED_POKEMON,"APPEARED_POKEMON");

                //Ahora el paquete tiene otro ID!
                sizePaquete = sizePaquete + sizeof(uint32_t);

            	enviarMensajeRecibidoASuscriptores(suscriptores_APPEARED_POKEMON,enviarMensajeA);

            	free(paquete);
            	free(paqueteID_Appeared);

     		    }

     			break;

     		case t_CATCH:;

     			// Recibo el Paquete
     		     paquete = receiveAndUnpack(cliente_fd,sizePaquete);
     		     //Sacarle el ID_Basura con el que llega al BROKER / <<< Esto se usara para guardarlo en la cola >>>
     		     //Generar una ID nueva / lo empaco / se lo envio al productor
     		     uint32_t ID_CATCH = asignarIDMensaje();
     		     void *paqueteID_CATCH = pack_ID(ID_CATCH, t_CATCH);
     		     uint32_t sizePaqueteID_Catch = sizeof(uint32_t)+sizeof(t_CATCH);
     		     packAndSend(cliente_fd,paqueteID_CATCH,sizePaqueteID_Catch,t_ID);

     		     //Saco el ID para armar el MENSAJE / Armar el Mensaje / Guardarlo en COLA
     		     void *paqueteCatchSinID = quitarIDPaquete(paquete,sizePaquete);
     		     mensaje = crearMensaje(paqueteCatchSinID,ID_CATCH,-1,sizePaquete - sizeof(uint32_t));
     		     agregarMensajeACola(mensaje,CATCH_POKEMON,"CATCH_POKEMON");

     		     //Empaqueto Mensaje con ID asignado para Enviar / Enviar a Suscriptores
     		     void *paqueteCATCH = insertarIDEnPaquete(ID_CATCH,paqueteCatchSinID,sizePaquete,BROKER_ID);
     		     paquete = paqueteCATCH;
     		     enviarMensajeRecibidoASuscriptores(suscriptores_CATCH_POKEMON,enviarMensajeA);

     		     free(paqueteCATCH);
     		     free(paqueteID_CATCH);

     			break;

     		case t_CAUGHT:
     			// Recibo el Paquete /
     			paquete = receiveAndUnpack(cliente_fd,sizePaquete);
     			//Obtengo el ID del paquete que llega el BROKER
     			uint32_t ID_CAUGHT_Correlativo = unpackID(paquete);

     			if(existeRespuestaEnCola(ID_CAUGHT_Correlativo,CAUGHT_POKEMON)){
     				log_error(logger,"El mensaje con ID_Correlativo [%d] ya existe en la cola CAUGHT POKEMON",ID_CAUGHT_Correlativo);
     				free(paquete);
     			}
     			else{
					//Generar una ID nueva / lo empaco / se lo envio al productor
					uint32_t ID_CAUGHT_Generado = asignarIDMensaje();
					void *paqueteID_CAUGHT = pack_ID(ID_CAUGHT_Generado,t_CAUGHT);
					uint32_t sizePaqueteID_CAUGHT = sizeof(uint32_t) + sizeof(t_CAUGHT);
					packAndSend(cliente_fd,paqueteID_CAUGHT,sizePaqueteID_CAUGHT,t_ID);

					//Saco el ID para armar el MENSAJE / Armar el Mensaje / Guardarlo en COLA
					void *paqueteCaughtSinID = quitarIDPaquete(paquete,sizePaquete);

					mensaje = crearMensaje(paqueteCaughtSinID,ID_CAUGHT_Generado,ID_CAUGHT_Correlativo,sizePaquete - sizeof(uint32_t));

					void *paqueteCaught = insertarIDEnPaquete(ID_CAUGHT_Generado,paquete,sizePaquete,DOUBLE_ID);

					paquete = paqueteCaught;

					agregarMensajeACola(mensaje,CAUGHT_POKEMON,"CAUGHT_POKEMON");

			        //Ahora el paquete tiene otro ID!
			        sizePaquete = sizePaquete + sizeof(uint32_t);
					//Empaqueto Mensaje con ID asignado para Enviar / Enviar a Suscriptores
					enviarMensajeRecibidoASuscriptores(suscriptores_CAUGHT_POKEMON,enviarMensajeA);

					free(paquete);
					free(paqueteID_CAUGHT);
     			}

     			break;

     		case t_ACK:
     			paquete = receiveAndUnpack(cliente_fd,sizePaquete);
     			uint32_t ID_mensaje = unpackID(paquete);
     			t_operacion nombreCola = unpackOperacionACK(paquete);
     			char *ID_proceso = unpackIdentificadorProcesoACK(paquete);

     			validarRecepcionMensaje(ID_mensaje,nombreCola,ID_proceso);

     			free(paquete);

     			break;

     			//Revisar bien esto.
     		default:
     			pthread_exit(NULL);
     		}

}


t_particion *firstFit(uint32_t tamPaquete){
	t_particion *particion;

	bool esMenor(void *unNumero,void *otroNumero){
		uint32_t *valor = unNumero;
		uint32_t *otroValor = otroNumero;
		return *valor < *otroValor;
	}

	list_sort(particionesLibres,esMenor);

	mostrarLista(particionesLibres);

	for(int i = 0; i < list_size(particionesLibres);i++){
		uint32_t *IDParticion = list_get(particionesLibres,i);
		particion = obtenerParticion(*IDParticion,PARTITION_ID);
		uint32_t tamanioParticion = particion->tamanioParticion;
		if(tamanioParticion >= tamPaquete){
			log_info(logger,"Encontré una particion :D! SU ID es %d",*IDParticion);
			return particion;
		}
	}

	return NULL;

}

void FIFO(){

}

void LRU(){

}

t_particion *bestFit(uint32_t tam){
	//TODO
	return NULL;
}

void mostrarNumero(int *var){
    printf("ID PARTICION: %d \n",*var);

}

void mostrarLista(t_list *lista){
	for(int i =0; i < list_size(lista);i++){
		uint32_t *numero = list_get(lista,i);
		printf("ID PARTICION: %d \n",*numero);
	}
}

void mostrarContenidoLista(t_list* lista){
   list_iterate(lista,(void *) mostrarNumero);
}

void liberarParticion(t_particion *particion){
	uint32_t offset = particion->offset;

	uint32_t tamanioParticion = particion->tamanioParticion;

	list_add(particionesLibres,&(particion->ID_Particion));

	log_error(logger,"EL ID DEL MENSAJE QUE GUARDA LA PARTICION: %d",particion->ID_mensaje);

	FILE *arch = fopen("/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/Broker/Cache","rw+");

	fseek(arch,offset,SEEK_CUR);

	void *paqueteVacio = calloc(1,tamanioParticion);

	fwrite(paqueteVacio,tamanioParticion,1,arch);

    fclose(arch);

    consolidarParticion(particion);

}


uint32_t obtenerPosicionParticion(uint32_t IDParticion){
	t_particion *particion;
	for(int i = 0; i < list_size(particiones);i++){
		particion = list_get(particiones,i);
		uint32_t IDParticionAComparar = particion->ID_Particion;
		if(intComparator(&IDParticion,&IDParticionAComparar)){
			return i;
		}
	}
	return -1;
}

bool estaLibre(t_particion *particion){
	uint32_t IDParticion = particion->ID_Particion;
	return existeID(&IDParticion,particionesLibres,intComparator);
}

void modificarParticion(t_particion *particion,uint32_t IDMensaje,char *colaDeMensaje){
	particion->ID_mensaje = IDMensaje;
	particion->colaDeMensaje = colaDeMensaje;
}

void consolidarParticion(t_particion *particionActual){
	uint32_t IDParticion = particionActual->ID_Particion;
	uint32_t offsetActual = particionActual->offset;
	uint32_t posActual = obtenerPosicionParticion(IDParticion);
	uint32_t tamanioParticion = particionActual->tamanioParticion;
	uint32_t posAnterior = posActual-1;
	uint32_t posSiguiente = posActual+1;

	log_error(logger,"POS ANTERIOR %d",posAnterior);
	log_error(logger,"POS SIGUIENTE: %d",posSiguiente);

	t_particion *particionAnterior = list_get(particiones,posAnterior);
	t_particion *particionSiguiente = list_get(particiones,posSiguiente);

	if(particionAnterior != NULL){
		if(estaLibre(particionAnterior)){
			uint32_t tamanioParticionAnterior = particionAnterior->tamanioParticion;
			uint32_t offsetAnterior = particionAnterior->offset;
			uint32_t tamanioTotal = tamanioParticion + tamanioParticionAnterior;
			t_particion *particionConsolidadaIzquierda = crearParticion(tamanioTotal,-1,"");
			log_error(logger,"ID PARTICION CONSOLIDADIZQUIERDA: %d",particionConsolidadaIzquierda->ID_Particion);
			setearOffset(offsetAnterior,particionConsolidadaIzquierda);
			log_error(logger,"VOY A BORRAR LA PARTICION: %d, QUE SE ENCUENTRA EN %d",particionActual->ID_Particion,posActual);
			list_remove(particiones,posActual);
			list_remove(particionesLibres,obtenerPosicionIDParticion(particionActual->ID_Particion));
		    list_replace(particiones,posAnterior,particionConsolidadaIzquierda);
		    list_remove(particionesLibres,obtenerPosicionIDParticion(particionAnterior->ID_Particion));
		    list_add(particionesLibres,&(particionConsolidadaIzquierda->ID_Particion));
		    mostrarLista(particionesLibres);


		    if(particionSiguiente != NULL && estaLibre(particionSiguiente)){
		    	consolidarParticion(particionSiguiente);
		    }

		    log_error(logger,"TAMANIO PARTICION CONSOLIDADA IZQUIERDA: %d",tamanioTotal);

		}
	}

	if(particionSiguiente != NULL){
		if(estaLibre(particionSiguiente)){

            uint32_t tamanioParticionSiguiente = particionSiguiente->tamanioParticion;
            uint32_t tamanioTotal = tamanioParticion + tamanioParticionSiguiente;
            t_particion *particionConsolidadaDerecha = crearParticion(tamanioTotal,-1,"");
			setearOffset(offsetActual,particionConsolidadaDerecha);
			list_remove(particiones,posSiguiente);
			list_remove(particionesLibres,obtenerPosicionIDParticion(particionSiguiente->ID_Particion));
		    list_replace(particiones,posActual,particionConsolidadaDerecha);
		    list_remove(particionesLibres,obtenerPosicionIDParticion(particionActual->ID_Particion));
		    list_add(particionesLibres,&(particionConsolidadaDerecha->ID_Particion));

		    if(particionAnterior != NULL && estaLibre(particionAnterior)){
		    	consolidarParticion(particionAnterior);
		    }
		}
	}

	return;
}


bool existeParticionLibreParaCachearMensaje(uint32_t tamPaquete){
	t_particion *particion;
	bool resultado = false;
	for(int i = 0; i < list_size(particionesLibres);i++){
		uint32_t *IDParticion = list_get(particionesLibres,i);
		particion = obtenerParticion(*IDParticion,PARTITION_ID);
		uint32_t tamanioParticion = particion->tamanioParticion;
		if(tamanioParticion >= tamPaquete){
			resultado = true;
			break;
		}
	}
	return resultado;
}


//Creo que esta funcion no va...
void introducirBloqueEnCache(void *paquete,t_particion *particion){

	uint32_t tamParticion = particion ->tamanioParticion;

	FILE *arch = fopen("/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/Broker/Cache","rw+");

	fseek(arch,offsetCache,SEEK_CUR);

	log_error(logger,"Posición de inicio de la partición creada [%d]",offsetCache);

	setearOffset(offsetCache,particion);

	fwrite(paquete,tamParticion,1,arch);

	offsetCache += tamParticion;

	fclose(arch);
}

void buddySystem(t_mensaje *mensaje,char *nombreCola,void (*algoritmoReemplazo)(),t_particion *(*funcionCorrespondencia)(uint32_t)){
	//TODO
}

//Se puede evitar repetir lógica...
void particionesDinamicas(t_mensaje *mensaje,char *nombreCola,void (*algoritmoReemplazo)(),t_particion *(*funcionCorrespondencia)(uint32_t)){

	void *paqueteACachear = mensaje->paquete;

	uint32_t tamPaquete = mensaje->tamanioPaquete;

	uint32_t IDMensaje = mensaje->ID_mensaje;

	uint32_t tamPaqueteAuxiliar = tamPaquete;

	if(tamPaquete < tamanio_minimo_particion){
		tamPaqueteAuxiliar = tamanio_minimo_particion;
	}

	if(!existeParticionLibreParaCachearMensaje(tamPaqueteAuxiliar)){
		//compactarMemoria();
		//particionesDinamicas(mensaje,nombreCola,etc)
		//o llamar a FIFO/LRU
	}
	else{
		t_particion *particionLibre = funcionCorrespondencia(tamPaqueteAuxiliar);

		uint32_t posicionParticionLibre = obtenerPosicionIDParticion(particionLibre->ID_Particion);

		modificarParticion(particionLibre,IDMensaje,nombreCola);

		uint32_t offsetParticionLibre = particionLibre->offset;

		uint32_t tamanioRestante = particionLibre->tamanioParticion - tamPaqueteAuxiliar;

		if(tamanioRestante == 0){

			FILE *arch = fopen("/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/Broker/Cache","rw+");

			fseek(arch,offsetParticionLibre,SEEK_CUR);

		    log_error(logger,"Posición de inicio de la partición obtenida por FIRST FIT: %d",offsetParticionLibre);

		    fwrite(paqueteACachear,tamPaqueteAuxiliar,1,arch);

			fclose(arch);

			list_remove(particionesLibres,posicionParticionLibre);
		}
		else{

			FILE *arch = fopen("/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/Broker/Cache","rw+");

			fseek(arch,offsetParticionLibre,SEEK_CUR);

			log_error(logger,"Posición de inicio de la partición obtenida por FIRST FIT: %d",offsetParticionLibre);

			fwrite(paqueteACachear,tamPaqueteAuxiliar,1,arch);

			log_error(logger,"OCUPE LA PARTICION CON ID %d el mensaje con ID %d",particionLibre->ID_Particion,IDMensaje);

			fclose(arch);

			particionLibre->tamanioParticion = tamPaqueteAuxiliar;

			t_particion *nuevaParticion = crearParticion(tamanioRestante,-1,"");
			log_error(logger,"ID PARTICION OBTENIDA POR FUNCION CORRESPONDENCIA: %d",particionLibre->ID_Particion);
			log_error(logger,"ID NUEVA PARTICION: %d",nuevaParticion->ID_Particion);
			log_error(logger,"TAMANIO PARTICION LIBRE: %d",tamanioRestante);
			list_add(particiones,nuevaParticion);
			list_replace(particionesLibres,posicionParticionLibre,&(nuevaParticion->ID_Particion));

			setearOffset(offsetParticionLibre+tamPaqueteAuxiliar,nuevaParticion);
		}
	}
}


void cachearMensaje(t_mensaje *mensaje,char *nombreCola){
	//Llamo al "singleton"
	funcionCacheo->algoritmoMemoria(mensaje,nombreCola,funcionCacheo->algoritmoReemplazo,funcionCacheo->funcionCorrespondencia);
}

void construirFuncionCacheo(){
	//Se hace una sola vez..
	funcionCacheo = malloc(sizeof(singletonMemoria));

	if(stringComparator("PARTICIONES",algoritmo_memoria)){
		funcionCacheo->algoritmoMemoria = particionesDinamicas;
	}else{
		funcionCacheo->algoritmoMemoria = buddySystem;
	}

	if(stringComparator("FF",algoritmo_particion_libre)){
		funcionCacheo->funcionCorrespondencia = firstFit;
	}
	else{
		funcionCacheo->funcionCorrespondencia = bestFit;
	}

	if(stringComparator("FIFO",algoritmo_reemplazo)){
		funcionCacheo->algoritmoReemplazo = FIFO;
	}
	else{
		funcionCacheo->algoritmoReemplazo = LRU;
	}
}

void agregarMensajeACola(t_mensaje *mensaje,t_list *colaDeMensajes,char *nombreCola){
	pthread_mutex_lock(&semaforoMensajes);
	list_add(colaDeMensajes,mensaje);
	log_info(logger,"Un nuevo mensaje fue agregado a la cola %s",nombreCola);
	log_info(logger,"La cola %s tiene %d mensajes",nombreCola,list_size(colaDeMensajes));

	cachearMensaje(mensaje,nombreCola);
	pthread_mutex_unlock(&semaforoMensajes);
}


bool existeAlgunACK(t_suscriptor *suscriptor,t_list *colaDeMensajes){
	bool resultado = false;

	for(int i = 0; i < list_size(colaDeMensajes);i++){
		t_mensaje *mensaje = list_get(colaDeMensajes,i);
		t_list *acks = mensaje->suscriptoresQueRecibieronMensaje;
		if(existeACK(suscriptor,acks)){
			resultado = true;
			break;
		}
	}

	return resultado;
}

bool existeACK(t_suscriptor *suscriptor,t_list *acks){
	char *identificadorProceso = suscriptor->identificadorProceso;
	return existeID(identificadorProceso,acks,stringComparator);
}


void compactarMemoria(){
	FILE *arch = fopen("/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/Broker/Cache","rw+");

	t_particion *particionActual;
	t_particion *particionSiguiente;
	uint32_t espacioLibre = 0;
	uint32_t offsetLocalCache = 0;

	list_sort(particiones,tieneMenorOffset);

	for(int i = 0; i < list_size(particiones);i++){

		particionActual = list_get(particiones,i);

		particionSiguiente = list_get(particiones,i+1);

		//Habría que validar si las particiones obtenidas no son NULL..

		uint32_t tamParticionActual = particionActual->tamanioParticion;

		if(estaLibre(particionActual)){

			uint32_t tamParticionSiguiente = particionSiguiente->tamanioParticion;

			uint32_t offsetSiguiente = particionSiguiente->offset;

			fseek(arch,offsetSiguiente,SEEK_CUR);

			void *particion = malloc(tamParticionSiguiente);

			fread(particion,tamParticionSiguiente,1,arch);

			fseek(arch,offsetLocalCache,SEEK_CUR);

			fwrite(particion,tamParticionSiguiente,1,arch);

			setearOffset(offsetLocalCache,particionSiguiente);

			offsetLocalCache += tamParticionSiguiente;

			espacioLibre += tamParticionActual;
		}else{
			offsetLocalCache += tamParticionActual;
		}
	}

	list_clean(particionesLibres);

	t_particion *nuevaParticion = crearParticion(espacioLibre,-1,"");

	setearOffset(offsetLocalCache,nuevaParticion);

	list_add(particionesLibres,nuevaParticion);

	fclose(arch);
}


uint32_t obtenerPosicionMensaje(t_mensaje *mensaje,t_list *colaDeMensajes){
	uint32_t posicionMensaje;

	uint32_t IDMensaje = mensaje->ID_mensaje;
	for(int i = 0; i < list_size(colaDeMensajes);i++){
		t_mensaje *mensajeAComparar = list_get(colaDeMensajes,i);
		uint32_t IDAComparar = mensajeAComparar->ID_mensaje;
		if(intComparator(&IDMensaje,&IDAComparar)){
			memcpy(&posicionMensaje,&i,sizeof(uint32_t));
			break;
		}
	}

	return posicionMensaje;
}

//Faltaría invocar a liberarParticion
void eliminarMensaje(uint32_t ID, t_list *colaDeMensajes,char *nombreCola,t_FLAG flag){
	t_mensaje *mensajeAEliminar = obtenerMensaje(ID,colaDeMensajes,flag);
	uint32_t posicionMensaje = obtenerPosicionMensaje(mensajeAEliminar,colaDeMensajes);
	list_remove_and_destroy_element(colaDeMensajes,posicionMensaje,(void *) destruirMensaje);
	log_info(logger,"El mensaje con ID [%d] se eliminó de la cola %s. Motivo: Todos los suscriptores recibieron el mensaje.",ID,nombreCola);

}

void destruirMensaje(t_mensaje *mensaje){
	void *paquete = mensaje->paquete;
	t_list *suscriptoresQueRecibieronMensaje = mensaje->suscriptoresQueRecibieronMensaje;
	t_list *suscriptoresALosQueMandeMensaje = mensaje->suscriptoresALosQueMandeMensaje;
	free(paquete);
	list_destroy_and_destroy_elements(suscriptoresALosQueMandeMensaje,(void *) destruirIdentificador);
	list_destroy_and_destroy_elements(suscriptoresQueRecibieronMensaje,(void *) destruirIdentificador);
	free(mensaje);
}

void destruirIdentificador(char *identificadorProceso){
	free(identificadorProceso);
}

void iniciarMemoria(){
	char *paquete = calloc(1,tamanio_memoria);
	FILE *arch = fopen("/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/Broker/Cache","w");
	fwrite(paquete,tamanio_memoria,1,arch);
	particionesLibres = list_create();
	particiones = list_create();

	//Tal vez sería conveniente que crearParticion sólo reciba un tamanio...
	t_particion *particionInicial = crearParticion(tamanio_memoria,-1,"");
	log_error(logger,"Se inició la memoria cache!");
	setearOffset(0,particionInicial);

	list_add(particiones,particionInicial);
	list_add(particionesLibres,&(particionInicial->ID_Particion));

	construirFuncionCacheo();

	fclose(arch);
}


uint32_t obtenerPosicionIDParticion(uint32_t IDParticion){

	for(int i = 0; i < list_size(particionesLibres);i++){
		uint32_t *ID_A_Comparar = list_get(particionesLibres,i);
		if(intComparator(ID_A_Comparar,&IDParticion)){
			return i;
		}
	}

	return -1;
}

uint32_t asignarIDParticion(){
	uint32_t ID;
	contadorParticiones++;
	memcpy(&ID,&contadorParticiones,sizeof(uint32_t));
	return ID;
}

t_particion *crearParticion(uint32_t tamanioParticion,uint32_t ID_mensaje,char *colaDeMensaje){
	t_particion *particion = malloc(sizeof(t_particion));
	particion->ID_Particion = asignarIDParticion();
	particion->ID_mensaje = ID_mensaje;
	particion->colaDeMensaje = colaDeMensaje;
	particion->tamanioParticion = tamanioParticion;
	//El offset se setea más adelante..

	return particion;
}

void destruirParticion(t_particion *particion){
	char *nombreCola = particion->colaDeMensaje;
	free(nombreCola);
	free(particion);
}

void setearOffset(uint32_t offset,t_particion *particion){
	particion->offset = offset;
}

uint32_t espacioDisponible(){
	uint32_t tamanioDisponible = tamanio_memoria - offsetCache;
	return tamanioDisponible;
}

uint32_t *obtenerIDParticion(t_particion *particion){
	return &(particion->ID_Particion);
}

uint32_t *obtenerIDMensajeParticion(t_particion *particion){
	return &(particion->ID_mensaje);
}

uint32_t *obtenerTamanioParticion(t_particion *particion){
	return &(particion->tamanioParticion);
}

t_particion *obtenerParticionDeParticiones(uint32_t elem,uint32_t *(*funcionObtencionElemento)(t_particion*)){
	t_particion *particion;

	for(int i = 0;i < list_size(particiones);i++){
		particion = list_get(particiones,i);
		uint32_t *elemAComparar = funcionObtencionElemento(particion);
		if(intComparator(elemAComparar,&elem)){
			return particion;
		}
	}

	return NULL;
}


t_particion *obtenerParticion(uint32_t elem,t_FLAG flag){
	t_particion *particion;

	if(flag == BROKER_ID){
		particion = obtenerParticionDeParticiones(elem,obtenerIDMensajeParticion);
		if(particion == NULL){
			log_error(logger,"No se pudo obtener la particion según BROKER ID [%d]",elem);
		}
	}

	if(flag == SIZE_PARTITION){
		particion = obtenerParticionDeParticiones(elem,obtenerTamanioParticion);
		if(particion == NULL){
			log_error(logger,"No se pudo obtener la particion según el tamanio [%d]",elem);
		}
	}

	if(flag == PARTITION_ID){
		particion = obtenerParticionDeParticiones(elem,obtenerIDParticion);
		if(particion == NULL){
			log_error(logger,"No se pudo obtener la particion con ID [%d]",elem);
		}
	}

	return particion;
}


void *descachearPaquete(t_mensaje *mensaje,char *colaDePokemon){
	uint32_t IDMensaje = mensaje->ID_mensaje;

    uint32_t tamPaquete = mensaje->tamanioPaquete;

	t_particion *particionMensaje = obtenerParticion(IDMensaje,BROKER_ID);

	uint32_t offset = particionMensaje->offset;

	log_error(logger,"TAM PAQUETE: %d y su offset :D %d",tamPaquete,offset);

	FILE *arch = fopen("/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/Broker/Cache","r");

	fseek(arch,offset,SEEK_CUR);

	void *paquete = malloc(tamPaquete);

	fread(paquete,tamPaquete,1,arch);

	fclose(arch);

	if(tienenIDCorrelativoLosMensajes(colaDePokemon)){
		uint32_t IDCorrelativo = mensaje->ID_correlativo;
		void *paqueteConUnID = insertarIDEnPaquete(IDCorrelativo,paquete,tamPaquete+sizeof(uint32_t),0);
		void *paqueteConDosID = insertarIDEnPaquete(IDMensaje,paqueteConUnID,tamPaquete + sizeof(uint32_t),DOUBLE_ID);
		return paqueteConDosID;
	}
	else{
		void *paqueteConUnID = insertarIDEnPaquete(IDMensaje,paquete,tamPaquete+sizeof(uint32_t),0);
		return paqueteConUnID;
	}
}

bool tienenIDCorrelativoLosMensajes(char *nombreCola){
	return stringComparator(nombreCola,"APPEARED_POKEMON") || stringComparator(nombreCola,"CAUGHT_POKEMON") || stringComparator(nombreCola,"LOCALIZED_POKEMON");
}

void enviarMensajesCacheados(t_suscriptor *suscriptor,t_list *mensajes){

}

bool tieneMenorOffset(void *particion1,void *particion2){
	t_particion *unaParticion = particion1;
	t_particion *otraParticion = particion2;

	uint32_t offsetUnaParticion = unaParticion->offset;
	uint32_t offsetOtraParticion = otraParticion->offset;

	return offsetUnaParticion < offsetOtraParticion;
}

bool existeParticionLibreConTamanio(uint32_t cantACachear){
	bool resultado = false;
	for(int i = 0;i < list_size(particionesLibres);i++){
		t_particion *particion = list_get(particiones,i);
		uint32_t tamanioParticion = particion->tamanioParticion;
		if(tamanioParticion == cantACachear){
			resultado = true;
			break;
		}
	}
	return resultado;
}

void enviarMensajeRecibidoASuscriptores(t_list *listaSuscriptores,void(*funcionDeEnvio)(t_suscriptor *)){
	pthread_mutex_lock(&semaforoMensajes);
	list_iterate(listaSuscriptores,(void *) funcionDeEnvio);
	pthread_mutex_unlock(&semaforoMensajes);
}

void *quitarIDPaquete(void *paquete,uint32_t tamanioPaquete){
	uint32_t tamPaquete = tamanioPaquete - sizeof(uint32_t);
	void *paqueteNuevo = malloc(tamPaquete);
	memcpy(paqueteNuevo,paquete+sizeof(uint32_t),tamPaquete);

	return paqueteNuevo;
}

void *insertarIDEnPaquete(uint32_t ID,void *paquete,uint32_t tamanioPaquete,t_FLAG flag){
	if(flag == DOUBLE_ID){
		void *paqueteAEnviar = malloc(tamanioPaquete+sizeof(uint32_t));
		memcpy(paqueteAEnviar,&ID,sizeof(uint32_t));
		memcpy(paqueteAEnviar+sizeof(uint32_t),paquete,tamanioPaquete);

		return paqueteAEnviar;
	}
	else{
		void *paqueteAEnviar = malloc(tamanioPaquete);
		memcpy(paqueteAEnviar,&ID,sizeof(uint32_t));
		memcpy(paqueteAEnviar+sizeof(uint32_t),paquete,tamanioPaquete - sizeof(uint32_t));

		return paqueteAEnviar;
	}
}

bool esUnGameCard(char *nombreProceso){
	return stringComparator(nombreProceso,"GameCard");
}

bool esUnGameBoy(char* nombreProceso){
	return stringComparator(nombreProceso,"GameBoy");
}

bool esUnTeam(char *nombreProceso){
	return stringComparator(nombreProceso,"Team");
}

bool esPrimeraConexion(char *nombreProceso){
	return esUnTeam(nombreProceso) || esUnGameCard(nombreProceso) || esUnGameBoy(nombreProceso);
}

//Esta función permite determinar si la respuesta a un mensaje ya fue agregado a la cola de mensajes.
bool existeRespuestaEnCola(uint32_t ID_Correlativo,t_list *colaDeMensajes){
	t_list *IDs_Correlativos = list_map(colaDeMensajes,(void *)obtenerIDCorrelativo);

	return existeID(&ID_Correlativo,IDs_Correlativos,intComparator);
}

uint32_t *obtenerIDCorrelativo(t_mensaje *mensaje){
	return &(mensaje->ID_correlativo);
}

uint32_t *obtenerIDMensaje(t_mensaje *mensaje){
	return &(mensaje->ID_mensaje);
}


t_mensaje *obtenerMensaje(uint32_t ID,t_list *colaDeMensajes,t_FLAG flag){
	t_mensaje *mensaje;

	if(flag == BROKER_ID){
		mensaje = obtenerMensajeDeCola(ID,colaDeMensajes,obtenerIDMensaje);
		if(mensaje == NULL){
			log_error(logger,"No se pudo obtener el mensaje con BROKER ID [%d]",ID);
		}
	}

	if(flag == CORRELATIVE_ID){
		mensaje = obtenerMensajeDeCola(ID,colaDeMensajes,obtenerIDCorrelativo);
		if(mensaje == NULL){
			log_error(logger,"No se pudo obtener el mensaje con ID CORRELATIVO [%d]",ID);
		}
	}

	return mensaje;

}

t_mensaje *obtenerMensajeDeCola(uint32_t ID,t_list *colaDeMensajes,uint32_t *(*funcionObtencionID)(t_mensaje *)){
	t_mensaje *mensaje;

	for(int i = 0;i < list_size(colaDeMensajes);i++){
		mensaje = list_get(colaDeMensajes,i);
		uint32_t *IDMensaje = funcionObtencionID(mensaje);
		if(intComparator(&ID,IDMensaje)){
			return mensaje;
		}
	}

	return NULL;
}

void validarRecepcionMensaje(uint32_t ID_mensaje,t_operacion nombreCola,char *ID_proceso){
	t_mensaje *mensaje;

	switch (nombreCola) {
	         	case t_NEW:;
                    mensaje = obtenerMensaje(ID_mensaje,NEW_POKEMON,BROKER_ID);
                    list_add(mensaje->suscriptoresQueRecibieronMensaje,ID_proceso);
                    log_info(logger,"%s recibió satisfactoriamente el mensaje con ID %d de la cola NEW",ID_proceso,ID_mensaje);

	     			break;

	         	case t_LOCALIZED:;
                    mensaje = obtenerMensaje(ID_mensaje,LOCALIZED_POKEMON,BROKER_ID);
                    list_add(mensaje->suscriptoresQueRecibieronMensaje,ID_proceso);
                    log_info(logger,"%s recibió satisfactoriamente el mensaje con ID %d de la cola LOCALIZED",ID_proceso,ID_mensaje);

	     			break;

	         	case t_GET:;
                    mensaje = obtenerMensaje(ID_mensaje,GET_POKEMON,BROKER_ID);
                    list_add(mensaje->suscriptoresQueRecibieronMensaje,ID_proceso);
                    log_info(logger,"%s recibió satisfactoriamente el mensaje con ID %d de la cola GET",ID_proceso,ID_mensaje);

	     			break;

	         	case t_APPEARED:;
                    mensaje = obtenerMensaje(ID_mensaje,APPEARED_POKEMON,BROKER_ID);
                    list_add(mensaje->suscriptoresQueRecibieronMensaje,ID_proceso);
                    log_info(logger,"%s recibió satisfactoriamente el mensaje con ID %d de la cola APPEARED",ID_proceso,ID_mensaje);

	     			break;

	         	case t_CATCH:;
                    mensaje = obtenerMensaje(ID_mensaje,CATCH_POKEMON,BROKER_ID);
                    list_add(mensaje->suscriptoresQueRecibieronMensaje,ID_proceso);
                    log_info(logger,"%s recibió satisfactoriamente el mensaje con ID %d de CATCH",ID_proceso,ID_mensaje);

	     			break;

	         	case t_CAUGHT:;
                    mensaje = obtenerMensaje(ID_mensaje,CAUGHT_POKEMON,BROKER_ID);
                    list_add(mensaje->suscriptoresQueRecibieronMensaje,ID_proceso);
                    log_info(logger,"%s recibió satisfactoriamente el mensaje con ID %d de CAUGHT",ID_proceso,ID_mensaje);

	     			break;

	     		default:
	     			break;
	     		}
}

char *asignarIDProceso(char *nombreProceso){
	char *ID_generado;

	if(stringComparator("Team",nombreProceso)){
		pthread_mutex_lock(&semaforoIDTeam);
		contadorIDTeam++;
	    config_set_value(config,"CONTADOR_ID_TEAM",string_itoa(contadorIDTeam));
	    config_set_value(config,"CONTADOR_ID_MENSAJES",string_itoa(contadorIDMensaje));
	    config_set_value(config,"CONTADOR_ID_GAMECARD",string_itoa(contadorIDGameCard));
	    config_save(config);
		ID_generado = string_itoa(contadorIDTeam);
		pthread_mutex_unlock(&semaforoIDTeam);
	}
	else if(stringComparator("GameCard",nombreProceso)){
		pthread_mutex_lock(&semaforoIDGameCard);
		contadorIDGameCard++;
	    config_set_value(config,"CONTADOR_ID_TEAM",string_itoa(contadorIDTeam));
	    config_set_value(config,"CONTADOR_ID_MENSAJES",string_itoa(contadorIDMensaje));
	    config_set_value(config,"CONTADOR_ID_GAMECARD",string_itoa(contadorIDGameCard));
	    config_save(config);
		ID_generado = string_itoa(contadorIDGameCard);
		pthread_mutex_unlock(&semaforoIDGameCard);
	}

	char *identificadorProceso = string_from_format("%s-%s",nombreProceso,ID_generado);

	list_add(IDs_procesos,identificadorProceso);

	return identificadorProceso;
}

bool existeID(void *ID_a_buscar, t_list *lista,bool(*comparator)(void *,void *)){
	bool resultado = false;

	for (int i = 0; i < list_size(lista);i++){
		void *ID_a_comparar = list_get(lista,i);

		if(comparator(ID_a_buscar,ID_a_comparar)){
			resultado = true;
			break;
		}
	}

	return resultado;
}

bool intComparator(void *unNumero,void *otroNumero){
	uint32_t *valor = unNumero;
	uint32_t *otroValor = otroNumero;

	return *valor == *otroValor;
}


bool stringComparator(void *unString, void *otroString){
	return string_equals_ignore_case(unString,otroString);
}


uint32_t asignarIDMensaje(){
	uint32_t ID;
    pthread_mutex_lock(&semaforoIDMensaje);
	contadorIDMensaje++;
    config_set_value(config,"CONTADOR_ID_TEAM",string_itoa(contadorIDTeam));
    config_set_value(config,"CONTADOR_ID_MENSAJES",string_itoa(contadorIDMensaje));
    config_set_value(config,"CONTADOR_ID_GAMECARD",string_itoa(contadorIDGameCard));
    config_save(config);
	memcpy(&ID,&contadorIDMensaje,sizeof(uint32_t));
	list_add(IDs_mensajes,&contadorIDMensaje);
	pthread_mutex_unlock(&semaforoIDMensaje);

	return ID;
}


t_mensaje *crearMensaje(void *paquete,uint32_t ID,uint32_t ID_correlativo,uint32_t tamanioPaquete){
	t_mensaje *mensaje = malloc(sizeof(t_mensaje));
	mensaje->paquete = paquete;
	mensaje->tamanioPaquete = tamanioPaquete;
	mensaje->ID_mensaje = ID;
	mensaje->ID_correlativo = ID_correlativo;
	mensaje->suscriptoresQueRecibieronMensaje = list_create();
	mensaje->suscriptoresALosQueMandeMensaje = list_create();

	return mensaje;
}

t_suscriptor *crearSuscriptor(char *identificadorProceso,int cliente_fd){
	t_suscriptor *suscriptor = malloc(sizeof(t_suscriptor));
	suscriptor->identificadorProceso = identificadorProceso;
	suscriptor->socket_suscriptor = cliente_fd;

	return suscriptor;
}

void setearValoresConfig(){
	tamanio_memoria = config_get_int_value(config,"TAMANO_MEMORIA");
	tamanio_minimo_particion = config_get_int_value(config,"TAMANO_MINIMO_PARTICION");
	algoritmo_memoria = config_get_string_value(config,"ALGORITMO_MEMORIA");
	algoritmo_reemplazo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");
    algoritmo_particion_libre = config_get_string_value(config,"ALGORITMO_PARTICION_LIBRE");
    ip = config_get_string_value(config, "IP_BROKER");
    puerto = config_get_string_value(config,"PUERTO_BROKER");
    frecuencia_compactacion = config_get_int_value(config,"FRECUENCIA_COMPACTACION");
    contadorIDMensaje = config_get_int_value(config,"CONTADOR_ID_MENSAJES");
    contadorIDTeam = config_get_int_value(config,"CONTADOR_ID_TEAM");
    contadorIDGameCard = config_get_int_value(config,"CONTADOR_ID_GAMECARD");

}

void liberarRecursosAdministracionMemoria(){
	//liberar listas particiones
	free(funcionCacheo);
    list_destroy(particionesLibres);
    list_destroy(particiones);
}

void inicializarColas(){
	NEW_POKEMON = list_create();
	APPEARED_POKEMON = list_create();
	CATCH_POKEMON = list_create();
	CAUGHT_POKEMON = list_create();
	GET_POKEMON = list_create();
	LOCALIZED_POKEMON = list_create();
    IDs_mensajes = list_create();
}

void inicializarListasSuscriptores(){
	suscriptores_NEW_POKEMON = list_create();
	suscriptores_APPEARED_POKEMON = list_create();
	suscriptores_CATCH_POKEMON = list_create();
	suscriptores_CAUGHT_POKEMON = list_create();
	suscriptores_GET_POKEMON = list_create();
	suscriptores_LOCALIZED_POKEMON = list_create();
    IDs_procesos = list_create();
}

void destruirListasSuscriptores(){
	list_destroy(suscriptores_NEW_POKEMON);
	list_destroy(suscriptores_APPEARED_POKEMON);
	list_destroy(suscriptores_CATCH_POKEMON);
	list_destroy(suscriptores_CAUGHT_POKEMON);
	list_destroy(suscriptores_GET_POKEMON);
	list_destroy(suscriptores_LOCALIZED_POKEMON);
	list_destroy(IDs_mensajes);
}

void inicializarSemaforos(){
    pthread_mutex_init(&semaforoIDMensaje,NULL);
    pthread_mutex_init(&semaforoIDTeam,NULL);
    pthread_mutex_init(&semaforoIDGameCard,NULL);
    pthread_mutex_init(&semaforoMensajes,NULL);
    pthread_mutex_init(&semaforoSuscripcionProceso,NULL);
}

void destruirSemaforos(){
    pthread_mutex_destroy(&semaforoIDMensaje);
    pthread_mutex_destroy(&semaforoIDTeam);
    pthread_mutex_destroy(&semaforoIDGameCard);
    pthread_mutex_destroy(&semaforoMensajes);
    pthread_mutex_destroy(&semaforoSuscripcionProceso);
}

void destruirColas(){
	list_destroy(NEW_POKEMON);
	list_destroy(APPEARED_POKEMON);
	list_destroy(CATCH_POKEMON);
	list_destroy(CAUGHT_POKEMON);
	list_destroy(GET_POKEMON);
	list_destroy(LOCALIZED_POKEMON);
    list_destroy(IDs_procesos);

	//TODO
	//list_destroy_and_destroy_elements
	//Desarrollar función destructora de mensajes.
}


//Agregar envio de mensajes cacheados...
void suscribirProceso(char *identificadorProceso,int cliente_fd,t_operacion operacionSuscripcion){
	//Al suscribir a un proceso a una determinada cola adquiere el caracter de suscriptor.

	t_suscriptor *nuevoSuscriptor;
	nuevoSuscriptor = crearSuscriptor(identificadorProceso,cliente_fd);

	pthread_mutex_lock(&semaforoSuscripcionProceso);
	switch (operacionSuscripcion) {
	         	case t_NEW:;
	         		list_add(suscriptores_NEW_POKEMON,nuevoSuscriptor);
	         		log_info(logger,"Se suscribio al %s a la cola %s",identificadorProceso,"NEW_POKEMON");
	     			break;

	     		case t_LOCALIZED:;
	     			list_add(suscriptores_LOCALIZED_POKEMON,nuevoSuscriptor);
	     			log_info(logger,"Se suscribio al %s a la cola %s",identificadorProceso,"LOCALIZED_POKEMON");

	     			break;

	     		case t_GET:;
	     			list_add(suscriptores_GET_POKEMON,nuevoSuscriptor);
	     			log_info(logger,"Se suscribio al %s a la cola %s",identificadorProceso,"GET_POKEMON");
	     			break;

	     		case t_APPEARED:;
	     			list_add(suscriptores_APPEARED_POKEMON,nuevoSuscriptor);
	     			log_info(logger,"Se suscribio al %s a la cola %s",identificadorProceso,"APPEARED_POKEMON");

	     			break;

	     		case t_CATCH:;
                    list_add(suscriptores_CATCH_POKEMON,nuevoSuscriptor);
                    log_info(logger,"Se suscribio al %s a la cola %s",identificadorProceso,"CATCH_POKEMON");
	     			break;

	     		case t_CAUGHT:;
                    list_add(suscriptores_CAUGHT_POKEMON,nuevoSuscriptor);
                    log_info(logger,"Se suscribio al %s a la cola %s",identificadorProceso,"CAUGHT_POKEMON");

	     			break;

	     		default:
	     			break;
	     		}
	pthread_mutex_unlock(&semaforoSuscripcionProceso);
}
