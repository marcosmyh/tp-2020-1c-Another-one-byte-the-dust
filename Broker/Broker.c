#include "Broker.h"

int main(void) {
    config = crearConfig();
    logger = crearLogger();
    setearValores(config);
    inicializarColas();

    socket_servidor = iniciar_servidor(ip,puerto);

    while(1){
    	int socket_cliente = esperar_cliente(socket_servidor);

    	if(pthread_create(&hiloAtencionCliente,NULL,(void*)atender_cliente,&socket_cliente) == 0){
    		pthread_detach(hiloAtencionCliente);
    	}

    }

    destruirColas();
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
     uint32_t size = header.tamanioMensaje;
     int cod_op = header.operaciones;
     void *paquete;
     char *proceso;

     switch (cod_op) {
     		case t_HANDSHAKE:
     			//TODO
     			proceso = malloc(size);
     			paquete = receiveAndUnpack(cliente_fd,size);
     			proceso = unpackProceso(paquete);

     			//Una vez identificado el proceso hay que suscribirlo a las colas correspondientes.

     			free(proceso);
     			//log_info(logger,"Se suscribio el proceso %s a la cola %s",nombreProceso,nombreCola);
     			break;

     		case t_NEW:
     			//TODO
     			//agregar mensaje a la cola NEW_POKEMON y enviar mensaje a los suscriptores.

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
	//TODO
	//Asignar ID al mensaje.
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
