#include "Broker.h"

int main(void) {
    config = crearConfig();
    logger = crearLogger();
    setearValores();
    inicializarColas();
    inicializarListasSuscriptores();
    IDs_mensajes = list_create();
    IDs_procesos = list_create();

    //Agrego esto para testear la comunicación con el Team...
    list_add(IDs_procesos,"Team-1");

    pthread_mutex_init(&semaforoIDMensaje,NULL);
    pthread_mutex_init(&semaforoIDTeam,NULL);
    pthread_mutex_init(&semaforoIDGameCard,NULL);

    socket_servidor = iniciar_servidor(ip,puerto);

    while(1){
    	int socket_cliente = esperar_cliente(socket_servidor);

    	if(pthread_create(&hiloAtencionCliente,NULL,(void*)atender_cliente,&socket_cliente) == 0){
    		pthread_join(hiloAtencionCliente,NULL);
    		//CORRECCIÓN FUNDAMENTAL!!!
    	}

    }

    pthread_mutex_destroy(&semaforoIDMensaje);
    pthread_mutex_destroy(&semaforoIDTeam);
    pthread_mutex_destroy(&semaforoIDGameCard);
    list_destroy(IDs_mensajes);
    list_destroy(IDs_procesos);
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

     void enviarMensajeA(t_suscriptor *suscriptor){
    	 int socket = suscriptor->socket_suscriptor;
    	 char *identificadorProceso = suscriptor->identificadorProceso;
    	 int resultado = packAndSend(socket,paquete,sizePaquete,codigo_operacion);

    	 if(resultado != -1){
    	 log_info(logger,"Le mande un mensaje a %s",identificadorProceso);
    	 }
    	 else{
    		 log_info(logger,"No le pude mandar un mensaje a %s",identificadorProceso);
    	 }
     }

     switch (codigo_operacion) {
     		case t_HANDSHAKE:;
     		    t_operacion operacionDeSuscripcion;
     		    char *nombreProceso;
     		    char *identificadorProceso;
     		    void *paqueteHANDSHAKE;

     			paquete = receiveAndUnpack(cliente_fd,sizePaquete);
     			nombreProceso = unpackProceso(paquete);
     			uint32_t sizeProceso = strlen(nombreProceso) + 1;
     			operacionDeSuscripcion = unpackOperacion(paquete,sizeProceso);

     			if(existeID(nombreProceso,IDs_procesos,stringComparator) || stringComparator(nombreProceso,"GameBoy")){
     				packAndSend(cliente_fd,paquete,sizePaquete,t_HANDSHAKE);
     				identificadorProceso = nombreProceso;
     			}
     			else{
     				identificadorProceso = asignarIDProceso(nombreProceso);
     				paqueteHANDSHAKE = pack_Handshake(identificadorProceso,operacionDeSuscripcion);
     				uint32_t sizePaquete = strlen(identificadorProceso) + 1 + sizeof(uint32_t) + sizeof(t_operacion);
     				packAndSend(cliente_fd,paqueteHANDSHAKE,sizePaquete,t_HANDSHAKE);
     			}

     			suscribirProceso(identificadorProceso,cliente_fd,operacionDeSuscripcion);

     			break;

     		case t_NEW:;
     			//TODO
     		    void *paqueteNEW;

     		    paquete = receiveAndUnpack(cliente_fd,sizePaquete);

     		    uint32_t idMensajeRecibido_New = unpackID(paquete);

     		    if(existeID(&idMensajeRecibido_New,IDs_mensajes,intComparator)){
     		    	//Es un ID correlativo. Ver como proceder.
     		    }

                char *pokemon = unpackPokemon(paquete);

                uint32_t sizePokemon = strlen(pokemon) + 1;

                uint32_t cantidadPokemon = unpackCantidadPokemons_New(paquete,sizePokemon);

                uint32_t coordenadaX = unpackCoordenadaX_New(paquete,sizePokemon);

                uint32_t coordenadaY = unpackCoordenadaY_New(paquete,sizePokemon);

                //Asigno un ID al mensaje recibido
                uint32_t ID_NEW = asignarIDMensaje();

                void *paqueteID = pack_ID(ID_NEW,t_NEW);

                uint32_t sizePaqueteID = sizeof(ID_NEW) + sizeof(t_NEW);

                //Le envio el ID al productor.
                packAndSend(cliente_fd,paqueteID,sizePaqueteID,t_ID);

                //Ahora armo el paquete que van a recibir los suscriptores (consumidores)
     			paqueteNEW = pack_New(ID_NEW,pokemon,cantidadPokemon,coordenadaX,coordenadaY);

     			paquete = paqueteNEW;

                mensaje = crearMensaje(paquete,ID_NEW);

     			agregarMensajeACola(mensaje,NEW_POKEMON,"NEW_POKEMON");

     			enviarMensajeRecibidoASuscriptores(suscriptores_NEW_POKEMON,enviarMensajeA);

     			break;

     		case t_LOCALIZED:;
     			//TODO
     			//agregar mensaje a la cola LOCALIZED_POKEMON y enviar mensaje a los suscriptores.
     			break;

     		case t_GET:;
     			//TODO
     			//agregar mensaje a la cola GET_POKEMON y enviar mensaje a los suscriptores.
     			break;

     		case t_APPEARED:;
     			//TODO
     		    void *paqueteAppeared;

     		    paquete = receiveAndUnpack(cliente_fd,sizePaquete);

     		    uint32_t idMensajeRecibido_Appeared = unpackID(paquete);

     		    if(existeID(&idMensajeRecibido_Appeared,IDs_mensajes,intComparator)){
     		    	//Correlativo...

     		    }

     		    char *pokemonAppeared = unpackPokemon(paquete);

                uint32_t sizePokemonAppeared = strlen(pokemonAppeared) + 1;

                uint32_t coordenadaX_Appeared = unpackCoordenadaX_Appeared(paquete,sizePokemonAppeared);

                uint32_t coordenadaY_Appeared = unpackCoordenadaY_Appeared(paquete,sizePokemonAppeared);

                uint32_t ID_APPEARED = asignarIDMensaje();

                void *paqueteID_Appeared = pack_ID(ID_APPEARED,t_APPEARED);

                uint32_t sizePaqueteID_Appeared = sizeof(uint32_t) + sizeof(t_operacion);

                //Le envio el ID al productor.
                packAndSend(cliente_fd,paqueteID_Appeared,sizePaqueteID_Appeared,t_ID);

                //Ahora armo el paquete que van a recibir los suscriptores (consumidores)
                paqueteAppeared = pack_Appeared(ID_APPEARED,pokemonAppeared,coordenadaX_Appeared,coordenadaY_Appeared);

                paquete = paqueteAppeared;

                mensaje = crearMensaje(paquete,ID_APPEARED);

                agregarMensajeACola(mensaje,APPEARED_POKEMON,"APPEARED_POKEMON");

            	enviarMensajeRecibidoASuscriptores(suscriptores_APPEARED_POKEMON,enviarMensajeA);

     			break;

     		case t_CATCH:;
     			//TODO
     			//agregar mensaje a la cola CATCH_POKEMON y enviar mensaje a los suscriptores.
     			break;

     		case t_CAUGHT:;

     			break;

     		case t_ACK:
     			paquete = receiveAndUnpack(cliente_fd,sizePaquete);
     			uint32_t ID_mensaje = unpackID(paquete);
     			t_operacion nombreCola = unpackOperacionACK(paquete);
     			char *ID_proceso = unpackIdentificadorProcesoACK(paquete);

     			validarRecepcionMensaje(ID_mensaje,nombreCola,ID_proceso);

     			break;

     			//Revisar bien esto.
     		default:
     			pthread_exit(NULL);
     		}

}


//Posible funcion.
void agregarMensajeACola(t_mensaje *mensaje,t_list *colaDeMensajes,char *nombreCola){
	list_add(colaDeMensajes,mensaje);
	log_info(logger,"Un nuevo mensaje fue agregado a la cola %s",nombreCola);
	log_info(logger,"La cola %s tiene %d mensajes",nombreCola,list_size(colaDeMensajes));

}

void enviarMensajeRecibidoASuscriptores(t_list *listaSuscriptores,void(*funcionDeEnvio)(t_suscriptor *)){
	list_iterate(listaSuscriptores,(void *) funcionDeEnvio);
}

t_mensaje *obtenerMensaje(uint32_t ID_mensaje_a_modificar,t_list *colaDeMensajes){
	t_mensaje *mensaje;
	for(int i = 0;i < list_size(colaDeMensajes);i++){
		mensaje = list_get(colaDeMensajes,i);
		uint32_t ID_mensaje = mensaje->ID_mensaje;
		if(intComparator(&ID_mensaje_a_modificar,&(ID_mensaje))){
			break;
		}
	}
	return mensaje;
}

void validarRecepcionMensaje(uint32_t ID_mensaje,t_operacion nombreCola,char *ID_proceso){
	t_mensaje *mensaje;

	switch (nombreCola) {
	         	case t_NEW:;
                    mensaje = obtenerMensaje(ID_mensaje,NEW_POKEMON);
                    list_add(mensaje->suscriptoresQueRecibieronMensaje,ID_proceso);
                    log_info(logger,"%s recibió satisfactoriamente el mensaje con ID %d",ID_proceso,ID_mensaje);

	     			break;

	         	case t_LOCALIZED:;
                    mensaje = obtenerMensaje(ID_mensaje,LOCALIZED_POKEMON);
                    list_add(mensaje->suscriptoresQueRecibieronMensaje,ID_proceso);
                    log_info(logger,"%s recibió satisfactoriamente el mensaje con ID %d",ID_proceso,ID_mensaje);

	     			break;

	         	case t_GET:;
                    mensaje = obtenerMensaje(ID_mensaje,GET_POKEMON);
                    list_add(mensaje->suscriptoresQueRecibieronMensaje,ID_proceso);
                    log_info(logger,"%s recibió satisfactoriamente el mensaje con ID %d",ID_proceso,ID_mensaje);

	     			break;

	         	case t_APPEARED:;
                    mensaje = obtenerMensaje(ID_mensaje,APPEARED_POKEMON);
                    list_add(mensaje->suscriptoresQueRecibieronMensaje,ID_proceso);
                    log_info(logger,"%s recibió satisfactoriamente el mensaje con ID %d",ID_proceso,ID_mensaje);

	     			break;

	         	case t_CATCH:;
                    mensaje = obtenerMensaje(ID_mensaje,CATCH_POKEMON);
                    list_add(mensaje->suscriptoresQueRecibieronMensaje,ID_proceso);
                    log_info(logger,"%s recibió satisfactoriamente el mensaje con ID %d",ID_proceso,ID_mensaje);

	     			break;

	         	case t_CAUGHT:;
                    mensaje = obtenerMensaje(ID_mensaje,CAUGHT_POKEMON);
                    list_add(mensaje->suscriptoresQueRecibieronMensaje,ID_proceso);
                    log_info(logger,"%s recibió satisfactoriamente el mensaje con ID %d",ID_proceso,ID_mensaje);

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
		ID_generado = string_itoa(contadorIDTeam);
		pthread_mutex_unlock(&semaforoIDTeam);
	}
	else if(stringComparator("GameCard",nombreProceso)){
		pthread_mutex_lock(&semaforoIDGameCard);
		contadorIDGameCard++;
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
	memcpy(&ID,&contadorIDMensaje,sizeof(uint32_t));
	list_add(IDs_mensajes,&contadorIDMensaje);
	pthread_mutex_unlock(&semaforoIDMensaje);

	return ID;
}


t_mensaje *crearMensaje(void *paquete,uint32_t ID){
	//Creo el mensaje con el paquete y el ID que asigné.
	t_mensaje *mensaje = malloc(sizeof(t_mensaje));
	mensaje->paquete = paquete;
	mensaje->ID_mensaje = ID;
	mensaje->suscriptoresQueRecibieronMensaje = list_create();

	//Faltaría ver lo del id correlativo.

	return mensaje;
}

t_suscriptor *crearSuscriptor(char *identificadorProceso,int cliente_fd){
	t_suscriptor *suscriptor = malloc(sizeof(t_suscriptor));
	suscriptor->identificadorProceso = identificadorProceso;
	suscriptor->socket_suscriptor = cliente_fd;

	return suscriptor;
}

void setearValores(){
	tamanio_memoria = config_get_int_value(config,"TAMANO_MEMORIA");
	tamanio_minimo_particion = config_get_int_value(config,"TAMANO_MINIMO_PARTICION");
	algoritmo_memoria = config_get_string_value(config,"ALGORITMO_MEMORIA");
	algoritmo_reemplazo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");
    algoritmo_particion_libre = config_get_string_value(config,"ALGORITMO_PARTICION_LIBRE");
    ip = config_get_string_value(config, "IP_BROKER");
    puerto = config_get_string_value(config,"PUERTO_BROKER");
    frecuencia_compactacion = config_get_int_value(config,"FRECUENCIA_COMPACTACION");
}

void inicializarColas(){
	NEW_POKEMON = list_create();
	APPEARED_POKEMON = list_create();
	CATCH_POKEMON = list_create();
	CAUGHT_POKEMON = list_create();
	GET_POKEMON = list_create();
	LOCALIZED_POKEMON = list_create();
}

void inicializarListasSuscriptores(){
	suscriptores_NEW_POKEMON = list_create();
	suscriptores_APPEARED_POKEMON = list_create();
	suscriptores_CATCH_POKEMON = list_create();
	suscriptores_CAUGHT_POKEMON = list_create();
	suscriptores_GET_POKEMON = list_create();
	suscriptores_LOCALIZED_POKEMON = list_create();
}

void destruirListasSuscriptores(){
	list_destroy(suscriptores_NEW_POKEMON);
	list_destroy(suscriptores_APPEARED_POKEMON);
	list_destroy(suscriptores_CATCH_POKEMON);
	list_destroy(suscriptores_CAUGHT_POKEMON);
	list_destroy(suscriptores_GET_POKEMON);
	list_destroy(suscriptores_LOCALIZED_POKEMON);
}

void destruirColas(){
	list_destroy(NEW_POKEMON);
	list_destroy(APPEARED_POKEMON);
	list_destroy(CATCH_POKEMON);
	list_destroy(CAUGHT_POKEMON);
	list_destroy(GET_POKEMON);
	list_destroy(LOCALIZED_POKEMON);

	//TODO
	//list_destroy_and_destroy_elements
	//Desarrollar función destructora de mensajes.
}

void suscribirProceso(char *identificadorProceso,int cliente_fd,t_operacion operacionSuscripcion){
	//Al suscribir a un proceso a una determinada cola adquiere el caracter de suscriptor.

	t_suscriptor *nuevoSuscriptor;
	nuevoSuscriptor = crearSuscriptor(identificadorProceso,cliente_fd);

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
}
