#include "Broker.h"

int main(void) {
    config = crearConfig();
    logger = crearLogger();
    setearValores(config);
    inicializarColas();
    inicializarListasSuscriptores();
    pthread_mutex_init(&semaforoID,NULL);

    socket_servidor = iniciar_servidor(ip,puerto);

    while(1){
    	int socket_cliente = esperar_cliente(socket_servidor);

    	if(pthread_create(&hiloAtencionCliente,NULL,(void*)atender_cliente,&socket_cliente) == 0){
    		pthread_detach(hiloAtencionCliente);
    	}

    }

    pthread_mutex_destroy(&semaforoID);
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

    log_info(logger, "Listo para escuchar a mi cliente!");

    return socket_servidor;
}

int esperar_cliente(int socket_servidor){
	struct sockaddr_in dir_cliente;

	int tam_direccion = sizeof(struct sockaddr_in);
	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);
	log_info(logger,"Se conecto un cliente");
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
     t_operacion operacionDeSuscripcion;

     //--IMPORTANTE--
     //Este puntero a estructura va a representar el mensaje en la cola.
     t_mensaje *mensaje;

     void *paquete;
     char *proceso;

     //Creo una función auxiliar para poder enviar el mensaje recibido a todos los suscriptores.
     //La misma tiene que estar dentro del scope de procesar solicitud, para poder obtener detalles
     //tales como el paquete, tamaño del paquete, etc.

     void enviarMensajeA(int *socket){
    	 //if(conexion sigue activa) xd
    	 //{
    	 packAndSend(*socket,paquete,sizePaquete,codigo_operacion);
    	 validarRecepcionMensaje(*socket,mensaje);
    	 //}

    	 //validarRecepcioMensaje recibe mensaje para poder registrar que el suscriptor en cuestion
    	 //recibió el mensaje.
     }

     switch (codigo_operacion) {
     		case t_HANDSHAKE:
     			paquete = receiveAndUnpack(cliente_fd,sizePaquete);
     			proceso = unpackProceso(paquete);
     			uint32_t sizeProceso = sizeof(proceso);
     			operacionDeSuscripcion = unpackOperacion(paquete,sizeProceso);

     			suscribirProceso(proceso,&cliente_fd,operacionDeSuscripcion);

     			free(proceso);

     			break;

     		case t_NEW:
     			//TODO
     			paquete = receiveAndUnpack(cliente_fd,sizePaquete);
     			mensaje = crearMensaje(paquete,cliente_fd);

     			agregarMensajeACola(mensaje,NEW_POKEMON,"NEW_POKEMON");

     			//Recurro al casting (Ver prototipo de la función enviarMensajeA)
     			enviarMensajeRecibidoASuscriptores(suscriptores_NEW_POKEMON,(void *) enviarMensajeA);

     			break;

     		case t_LOCALIZED:
     			//TODO
     			//agregar mensaje a la cola LOCALIZED_POKEMON y enviar mensaje a los suscriptores.
     			break;

     		case t_GET:
     			//TODO
     			//agregar mensaje a la cola GET_POKEMON y enviar mensaje a los suscriptores.
     			break;

     		case t_APPEARED:
     			//TODO
     			//agregar mensaje a la cola APPEARED_POKEMON y enviar mensaje a los suscriptores.
     			break;

     		case t_CATCH:
     			//TODO
     			//agregar mensaje a la cola CATCH_POKEMON y enviar mensaje a los suscriptores.
     			break;

     		case t_CAUGHT:
     			//TODO
     			//agregar mensaje a la cola CAUGHT_POKEMON y enviar mensaje a los suscriptores.
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

void enviarMensajeRecibidoASuscriptores(t_list *listaSuscriptores,void(*funcionDeEnvio)(void *)){
	list_iterate(listaSuscriptores,funcionDeEnvio);
}

void validarRecepcionMensaje(int cliente_fd,t_mensaje *mensaje){
	//TODO
	//1° EL suscriptor me avisa que recibió el mensaje.
	//Luego:
	list_add(mensaje->suscriptoresQueRecibieronMensaje,&cliente_fd);
}

uint32_t asignarID(){
	//Primera aproximación..

    //pthread_mutex_lock(&semaforoID);
	contadorID++;
	//pthread_mutex_unlock(&semaforoID);

	return contadorID;
}

//Primera aproximación..
t_mensaje *crearMensaje(void *paquete,int cliente_fd){
	t_mensaje *mensaje = malloc(sizeof(mensaje));
	mensaje->paquete = paquete;
	mensaje->ID_mensaje = asignarID();
	if(list_is_empty(mensaje->suscriptoresQueRecibieronMensaje)){
		mensaje->suscriptoresQueRecibieronMensaje = list_create();
	}
	list_add(mensaje->suscriptoresQueRecibieronMensaje,&cliente_fd);
	return mensaje;
}

void setearValores(t_config *config){
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

void suscribirProceso(char *proceso,int *cliente_fd,t_operacion operacionSuscripcion){
	switch (operacionSuscripcion) {
	         	case t_NEW:
	         		list_add(suscriptores_NEW_POKEMON,cliente_fd);
	         		log_info(logger,"Se suscribio el proceso %s a la cola %s",proceso,"NEW_POKEMON");
	     			break;

	     		case t_LOCALIZED:
	     			list_add(suscriptores_LOCALIZED_POKEMON,cliente_fd);
	     			log_info(logger,"Se suscribio el proceso %s a la cola %s",proceso,"LOCALIZED_POKEMON");
	     			break;

	     		case t_GET:
	     			list_add(suscriptores_GET_POKEMON,cliente_fd);
	     			log_info(logger,"Se suscribio el proceso %s a la cola %s",proceso,"GET_POKEMON");
	     			break;

	     		case t_APPEARED:
	     			list_add(suscriptores_APPEARED_POKEMON,cliente_fd);
	     			log_info(logger,"Se suscribio el proceso %s a la cola %s",proceso,"APPEARED_POKEMON");
	     			break;

	     		case t_CATCH:
                    list_add(suscriptores_CATCH_POKEMON,cliente_fd);
                    log_info(logger,"Se suscribio el proceso %s a la cola %s",proceso,"CATCH_POKEMON");
	     			break;

	     		case t_CAUGHT:
                    list_add(suscriptores_CATCH_POKEMON,cliente_fd);
                    log_info(logger,"Se suscribio el proceso %s a la cola %s",proceso,"CAUGHT_POKEMON");
	     			break;

	     		default:
	     			break;
	     		}
}

