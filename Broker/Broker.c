#include "Broker.h"


int main(void){

    signal(SIGUSR1,&dumpDeCache);

    config = crearConfig();
    setearValoresConfig();
    logObligatorio = crearLogger(log_path_broker,1);
    logExtra = crearLogger("/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/Broker/logExtraBroker.log",0);
    inicializarColas();
    inicializarListasSuscriptores();
    inicializarSemaforos();
    iniciarMemoria();

    socket_servidor = iniciar_servidor(ip,puerto);

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
    log_destroy(logObligatorio);
    log_destroy(logExtra);
    return 0;
}

t_log *crearLogger(char *path,bool is_active_console){
    return log_create(path,"Broker",is_active_console,LOG_LEVEL_INFO);
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

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1){
            close(socket_servidor);
            continue;
        }
        break;
    }

    listen(socket_servidor, SOMAXCONN);

    freeaddrinfo(servinfo);

    return socket_servidor;
}

int esperar_cliente(int socket_servidor){
    struct sockaddr_in dir_cliente;

    int tam_direccion = sizeof(struct sockaddr_in);
    int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);
    log_info(logObligatorio,"Se conecto un proceso al Broker.");
    return socket_cliente;
}

void atender_cliente(int *socket){
    int cliente_fd = *socket;
    free(socket);
    Header header;
    header = receiveHeader(cliente_fd);
    procesar_solicitud(header,cliente_fd);
}


void enviarACKBroker(int socket,uint32_t ID,t_operacion operacion){
    void *paquete = pack_Ack(ID, operacion,"Broker");
    uint32_t tamPaquete = 3*sizeof(uint32_t) + strlen("Broker") + 1;
    packAndSend(socket, paquete, tamPaquete, t_ACK);
    log_info(logExtra, "El envio del ACK del mensaje con ID se realizo con exito");
    free(paquete);
}


void procesar_solicitud(Header header,int cliente_fd){
    uint32_t sizePaquete = header.tamanioMensaje;
    int codigo_operacion = header.operacion;
    t_mensaje *mensaje;
    void *paquete;

    void enviarMensajeA(t_suscriptor *suscriptor){
        char *identificadorProceso = suscriptor->identificadorProceso;
        packAndSend(suscriptor->socket_suscriptor,paquete,sizePaquete,codigo_operacion);
        char *idAAgregar = string_duplicate(identificadorProceso);
        list_add(mensaje->suscriptoresALosQueMandeMensaje,idAAgregar);

        printf("\033[1;31m");
        log_info(logObligatorio,"Le mandé a %s el mensaje con BROKER ID [%d] de la cola %s",idAAgregar,mensaje->ID_mensaje,mensaje->colaALaQuePertenece);
        printf("\033[0m");
    }

    switch(codigo_operacion){
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
                free(paqueteHANDSHAKE);
            }
            else{
                identificadorProceso = proceso;
                packAndSend(cliente_fd,paquete,sizePaquete,t_HANDSHAKE);
            }

            pthread_mutex_lock(&semaforoMensajes);
            suscribirProceso(identificadorProceso,cliente_fd,operacionDeSuscripcion);
            pthread_mutex_unlock(&semaforoMensajes);

            free(paquete);

            break;
    
        case t_NEW:;

            paquete = receiveAndUnpack(cliente_fd,sizePaquete);

            void *paqueteNewSinID = quitarIDPaquete(paquete,sizePaquete);

            uint32_t ID_NEW = asignarIDMensaje();

            enviarIDAlProductor(cliente_fd,ID_NEW,t_NEW);

            enviarACKBroker(cliente_fd,-1,t_NEW);

            void *paqueteNEW = insertarIDEnPaquete(ID_NEW,paqueteNewSinID,sizePaquete,0);

            free(paquete);

            paquete = paqueteNEW;

            mensaje = crearMensaje(ID_NEW,-1,sizePaquete - sizeof(uint32_t),"NEW_POKEMON");

            pthread_mutex_lock(&semaforoMensajes);

            singletonMemoria->paqueteACachear = paqueteNewSinID;

            agregarMensajeACola(mensaje,NEW_POKEMON);

            cachearMensaje(mensaje);

            enviarMensajeRecibidoASuscriptores(suscriptores_NEW_POKEMON,enviarMensajeA);

            pthread_mutex_unlock(&semaforoMensajes);

            free(paqueteNEW);

            break;    
        
        case t_LOCALIZED:;

            paquete = receiveAndUnpack(cliente_fd,sizePaquete);

            uint32_t ID_LOCALIZED_Correlativo = unpackID(paquete);

            if(existeRespuestaEnCola(ID_LOCALIZED_Correlativo,LOCALIZED_POKEMON)){
                log_error(logExtra,"El mensaje con ID Correlativo [%d] ya existe en la cola LOCALIZED_POKEMON",ID_LOCALIZED_Correlativo);
                enviarACKBroker(cliente_fd,ID_LOCALIZED_Correlativo,t_LOCALIZED);
                free(paquete);
            }
            else{
                uint32_t ID_LOCALIZED_Generado = asignarIDMensaje();

                void *paqueteLocalizedSinID = quitarIDPaquete(paquete,sizePaquete);

                enviarIDAlProductor(cliente_fd,ID_LOCALIZED_Generado,t_LOCALIZED);

                enviarACKBroker(cliente_fd,ID_LOCALIZED_Correlativo,t_LOCALIZED);

                void *paqueteLocalized = insertarIDEnPaquete(ID_LOCALIZED_Generado,paquete,sizePaquete,DOUBLE_ID);

                free(paquete);

                paquete = paqueteLocalized;

                mensaje = crearMensaje(ID_LOCALIZED_Generado,ID_LOCALIZED_Correlativo,sizePaquete - sizeof(uint32_t),"LOCALIZED_POKEMON");

                pthread_mutex_lock(&semaforoMensajes);

                singletonMemoria->paqueteACachear = paqueteLocalizedSinID;

                agregarMensajeACola(mensaje,LOCALIZED_POKEMON);

                cachearMensaje(mensaje);

                sizePaquete = sizePaquete + sizeof(uint32_t);

                enviarMensajeRecibidoASuscriptores(suscriptores_LOCALIZED_POKEMON,enviarMensajeA);

                pthread_mutex_unlock(&semaforoMensajes);

                free(paqueteLocalized);
            }

            break;    

        case t_GET:;

            paquete = receiveAndUnpack(cliente_fd,sizePaquete);

            void *paqueteGetSinID = quitarIDPaquete(paquete,sizePaquete);

            uint32_t ID_GET = asignarIDMensaje();

            enviarIDAlProductor(cliente_fd,ID_GET,t_GET);

            enviarACKBroker(cliente_fd,-1,t_GET);

            void *paqueteGET = insertarIDEnPaquete(ID_GET,paqueteGetSinID,sizePaquete,0);

            free(paquete);

            paquete = paqueteGET;
            
            mensaje = crearMensaje(ID_GET,-1,sizePaquete - sizeof(uint32_t),"GET_POKEMON");

            pthread_mutex_lock(&semaforoMensajes);

            singletonMemoria->paqueteACachear = paqueteGetSinID;

            agregarMensajeACola(mensaje,GET_POKEMON);

            cachearMensaje(mensaje);

            enviarMensajeRecibidoASuscriptores(suscriptores_GET_POKEMON,enviarMensajeA);

            pthread_mutex_unlock(&semaforoMensajes);

            free(paqueteGET);

            break;

        case t_APPEARED:;

            paquete = receiveAndUnpack(cliente_fd,sizePaquete);

            uint32_t ID_APPEARED_Correlativo = unpackID(paquete);

            if(existeRespuestaEnCola(ID_APPEARED_Correlativo,APPEARED_POKEMON)){
                log_error(logExtra,"El mensaje con ID Correlativo [%d] ya existe en la cola APPEARED POKEMON",ID_APPEARED_Correlativo);
                enviarACKBroker(cliente_fd,ID_APPEARED_Correlativo,t_APPEARED);
                free(paquete);
            }
            else{

                uint32_t ID_APPEARED_Generado = asignarIDMensaje();

                void *paqueteAppearedSinID = quitarIDPaquete(paquete,sizePaquete);

                enviarIDAlProductor(cliente_fd,ID_APPEARED_Generado,t_APPEARED);

                enviarACKBroker(cliente_fd,ID_APPEARED_Correlativo,t_APPEARED);

                void *paqueteAppeared = insertarIDEnPaquete(ID_APPEARED_Generado,paquete,sizePaquete,DOUBLE_ID);

                free(paquete);

                paquete = paqueteAppeared;

                mensaje = crearMensaje(ID_APPEARED_Generado,ID_APPEARED_Correlativo,sizePaquete - sizeof(uint32_t),"APPEARED_POKEMON");

                pthread_mutex_lock(&semaforoMensajes);

                singletonMemoria->paqueteACachear = paqueteAppearedSinID;

                agregarMensajeACola(mensaje,APPEARED_POKEMON);

                cachearMensaje(mensaje);

                sizePaquete = sizePaquete + sizeof(uint32_t);

                enviarMensajeRecibidoASuscriptores(suscriptores_APPEARED_POKEMON,enviarMensajeA);

                pthread_mutex_unlock(&semaforoMensajes);

                free(paqueteAppeared);

            }

            break;

        case t_CATCH:;

            paquete = receiveAndUnpack(cliente_fd,sizePaquete);

            void *paqueteCatchSinID = quitarIDPaquete(paquete,sizePaquete);

            uint32_t ID_CATCH = asignarIDMensaje();

            enviarIDAlProductor(cliente_fd,ID_CATCH,t_CATCH);

            enviarACKBroker(cliente_fd,-1,t_CATCH);

            void *paqueteCATCH = insertarIDEnPaquete(ID_CATCH,paqueteCatchSinID,sizePaquete,0);

            free(paquete);

            paquete = paqueteCATCH;

            mensaje = crearMensaje(ID_CATCH,-1,sizePaquete - sizeof(uint32_t),"CATCH_POKEMON");

            pthread_mutex_lock(&semaforoMensajes);

            singletonMemoria->paqueteACachear = paqueteCatchSinID;

            agregarMensajeACola(mensaje,CATCH_POKEMON);

            cachearMensaje(mensaje);

            enviarMensajeRecibidoASuscriptores(suscriptores_CATCH_POKEMON,enviarMensajeA);

            pthread_mutex_unlock(&semaforoMensajes);

            free(paqueteCATCH);

            break;

        case t_CAUGHT:

            paquete = receiveAndUnpack(cliente_fd,sizePaquete);

            uint32_t ID_CAUGHT_Correlativo = unpackID(paquete);

            if(existeRespuestaEnCola(ID_CAUGHT_Correlativo,CAUGHT_POKEMON)){
                log_error(logExtra,"El mensaje con ID_Correlativo [%d] ya existe en la cola CAUGHT POKEMON",ID_CAUGHT_Correlativo);
                enviarACKBroker(cliente_fd,ID_CAUGHT_Correlativo,t_CAUGHT);
                free(paquete);
            }
            else{

                uint32_t ID_CAUGHT_Generado = asignarIDMensaje();

                void *paqueteCaughtSinID = quitarIDPaquete(paquete,sizePaquete);

                enviarIDAlProductor(cliente_fd,ID_CAUGHT_Generado,t_CAUGHT);

                enviarACKBroker(cliente_fd,ID_CAUGHT_Correlativo,t_CAUGHT);

                void *paqueteCaught = insertarIDEnPaquete(ID_CAUGHT_Generado,paquete,sizePaquete,DOUBLE_ID);

                free(paquete);

                paquete = paqueteCaught;

                mensaje = crearMensaje(ID_CAUGHT_Generado,ID_CAUGHT_Correlativo,sizePaquete - sizeof(uint32_t),"CAUGHT_POKEMON");

                pthread_mutex_lock(&semaforoMensajes);

                singletonMemoria->paqueteACachear = paqueteCaughtSinID;

                agregarMensajeACola(mensaje,CAUGHT_POKEMON);

                cachearMensaje(mensaje);

                sizePaquete = sizePaquete + sizeof(uint32_t);

                enviarMensajeRecibidoASuscriptores(suscriptores_CAUGHT_POKEMON,enviarMensajeA);

                pthread_mutex_unlock(&semaforoMensajes);

                free(paqueteCaught);
            }

            break;

        case t_ACK:
            paquete = receiveAndUnpack(cliente_fd,sizePaquete);
            uint32_t ID_mensaje = unpackID(paquete);
            t_operacion nombreCola = unpackOperacionACK(paquete);
            char *ID_proceso = unpackIdentificadorProcesoACK(paquete);

            pthread_mutex_lock(&semaforoMensajes);
            validarRecepcionMensaje(ID_mensaje,nombreCola,ID_proceso);
            pthread_mutex_unlock(&semaforoMensajes);

            free(paquete);

            break;

        default:
            pthread_exit(NULL);
    }
}


void enviarIDAlProductor(int socketSuscriptor,uint32_t IDMensaje,int codigoOperacion){
    void *paqueteAEnviar = pack_ID(IDMensaje,codigoOperacion);
    uint32_t sizePaquete = 2*sizeof(uint32_t);
    packAndSend(socketSuscriptor,paqueteAEnviar,sizePaquete,t_ID);
    free(paqueteAEnviar);
}


void ordenarParticionesLibres(bool(*comparator)(void *,void*)){
    t_list *particionesAOrdenar = list_create();

    for(int i = 0;i < list_size(particionesLibres);i++){
        uint32_t *ID = list_get(particionesLibres,i);
        t_particion *particion = obtenerParticion(*ID,PARTITION_ID);
        list_add(particionesAOrdenar,particion);
    }

    list_sort(particionesAOrdenar,comparator);

    t_list *IDParticiones = list_map(particionesAOrdenar,(void *) obtenerIDParticion);

    list_clean(particionesLibres);

    list_add_all(particionesLibres,IDParticiones);

    list_clean(IDParticiones);
    list_destroy(IDParticiones);
    list_clean(particionesAOrdenar);
    list_destroy(particionesAOrdenar);
}


void ordenarParticionesLibresSegun(t_FLAG flag){

    if(flag == SIZE_PARTITION){
        ordenarParticionesLibres((void *) tieneMenorTamanio);
    }

    if(flag == OFFSET){
        ordenarParticionesLibres((void *) tieneMenorOffset);
    }

}

bool tieneMenorTamanio(t_particion *unaParticion,t_particion *otraParticion){
    return unaParticion->tamanioParticion < otraParticion->tamanioParticion;
}

t_particion *obtenerParticionLibre(uint32_t tamPaquete){
    t_particion *particion;

    for(int i = 0; i < list_size(particionesLibres);i++){
        uint32_t *IDParticion = list_get(particionesLibres,i);
        particion = obtenerParticion(*IDParticion,PARTITION_ID);
        uint32_t tamanioParticion = particion->tamanioParticion;
        if(tamanioParticion >= tamPaquete){
            return particion;
        }
    }

    return NULL;
}

t_particion *firstFit(uint32_t tamPaquete){
    t_particion *particion;

    ordenarParticionesLibresSegun(OFFSET);

    particion = obtenerParticionLibre(tamPaquete);

    return particion;
}

//Actualmente, la precisión para el timestamp es del orden de los microsegundos.
uint64_t getHorario(){
    struct timeval time;
    gettimeofday(&time,NULL);
    return time.tv_sec*(uint64_t)1000000+time.tv_usec;    
}

bool tieneMenorIDMensaje(t_particion *unaParticion,t_particion *otraParticion){
    return unaParticion->ID_mensaje < otraParticion->ID_mensaje;
}

void recorrerParticionesYLiberar(t_list *particionesOrdenadas,char *funcionCorrespondencia){
    for(int i = 0; i < list_size(particionesOrdenadas);i++){
        t_particion *particion = list_get(particionesOrdenadas,i);

        if(!estaLibre(particion)){
            printf("\033[1;34m");
            log_info(logObligatorio,"%s: Se procederá a eliminar la partición que tiene por offset %d",funcionCorrespondencia,particion->offset);
            printf("\033[0m");
            eliminarMensaje(particion->ID_mensaje,obtenerColaMensaje(particion->colaDeMensaje),particion->colaDeMensaje,REPLACEMENT_ALGORITHM);
            singletonMemoria->liberarParticion(particion);

            return;
        }
    }
}

t_list *obtenerColaMensaje(char *nombreCola){

    if(stringComparator(nombreCola,"NEW_POKEMON")){
        return NEW_POKEMON;
    }

    if(stringComparator(nombreCola,"LOCALIZED_POKEMON")){
        return LOCALIZED_POKEMON;
    }

    if(stringComparator(nombreCola,"CAUGHT_POKEMON")){
        return CAUGHT_POKEMON;
    }

    if(stringComparator(nombreCola,"GET_POKEMON")){
        return GET_POKEMON;
    }

    if(stringComparator(nombreCola,"CATCH_POKEMON")){
        return CATCH_POKEMON;
    }else{
        return APPEARED_POKEMON;
    }


}

void FIFO(){

    t_list *particionesOrdenadas = list_sorted(particiones,(void *) tieneMenorIDMensaje);

    recorrerParticionesYLiberar(particionesOrdenadas,"FIFO");

    list_clean(particionesOrdenadas);
    list_destroy(particionesOrdenadas);
}

void LRU(){

    t_list *particionesOrdenadas = list_sorted(particiones,(void *)seUsoMenosRecientemente);

    recorrerParticionesYLiberar(particionesOrdenadas,"LRU");

    list_clean(particionesOrdenadas);
    list_destroy(particionesOrdenadas);
}

t_particion *bestFit(uint32_t tamPaquete){

    t_particion *particion;

    ordenarParticionesLibresSegun(SIZE_PARTITION);

    particion = obtenerParticionLibre(tamPaquete);

    return particion;
}

void imprimirNumero(void *contenidoAMostrar){
    printf("ID: %d \n",*(uint32_t *) contenidoAMostrar);
}

void imprimirString(void *contenidoAMostrar){
    printf("ULTIMO ACCESO: %s \n",(char *) contenidoAMostrar);
}

void mostrarContenidoLista(t_list* lista,void(*printer)(void *)){
    list_iterate(lista,printer);
}


void liberarParticionParticionesDinamicas(t_particion *particion){

    void *paquete = calloc(1,particion->tamanioParticion);

    escribirEnMemoria(paquete,particion->tamanioParticion,particion->offset);

    list_add(particionesLibres,&(particion->ID_Particion));

    list_sort(particiones,(void *) tieneMenorOffset);

    consolidarParticion(particion);

    free(paquete);
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

    t_particion *particionAnterior = list_get(particiones,posAnterior);
    t_particion *particionSiguiente = list_get(particiones,posSiguiente);

    if(particionAnterior != NULL){
        if(estaLibre(particionAnterior)){
            uint32_t tamanioParticionAnterior = particionAnterior->tamanioParticion;
            uint32_t offsetAnterior = particionAnterior->offset;
            uint32_t tamanioTotal = tamanioParticion + tamanioParticionAnterior;
            t_particion *particionConsolidadaIzquierda = crearParticion(tamanioTotal);

            setearOffset(offsetAnterior,particionConsolidadaIzquierda);

            list_remove(particiones,posActual);
            list_remove(particionesLibres,obtenerPosicionIDParticion(particionActual->ID_Particion));
            list_replace(particiones,posAnterior,particionConsolidadaIzquierda);
            list_remove(particionesLibres,obtenerPosicionIDParticion(particionAnterior->ID_Particion));
            list_add(particionesLibres,&(particionConsolidadaIzquierda->ID_Particion));


            if(particionSiguiente != NULL && estaLibre(particionSiguiente)){
                consolidarParticion(particionSiguiente);
            }

            destruirParticion(particionActual);

        }
    }

    if(particionSiguiente != NULL){
        if(estaLibre(particionSiguiente)){

            uint32_t tamanioParticionSiguiente = particionSiguiente->tamanioParticion;
            uint32_t tamanioTotal = tamanioParticion + tamanioParticionSiguiente;
            t_particion *particionConsolidadaDerecha = crearParticion(tamanioTotal);
            setearOffset(offsetActual,particionConsolidadaDerecha);
            t_particion *particionSiguiente = list_remove(particiones,posSiguiente);
            list_remove(particionesLibres,obtenerPosicionIDParticion(particionSiguiente->ID_Particion));
            list_replace(particiones,posActual,particionConsolidadaDerecha);
            list_remove(particionesLibres,obtenerPosicionIDParticion(particionActual->ID_Particion));
            list_add(particionesLibres,&(particionConsolidadaDerecha->ID_Particion));

            if(particionAnterior != NULL && estaLibre(particionAnterior)){
                consolidarParticion(particionAnterior);
            }

            destruirParticion(particionSiguiente);
        }
    }

    return;
}

//////////////////////////////////////////////////////////////////////////////////////////////7
//////////////////////	Zona de trabajo del Gaby	   ///////////////////////////////////////7
//////////////////////////////////////////////////////////////////////////////////////////////7
//////////////////////////ZONA EN MANTENIMIENTO  /////////////////////////////////////////////7
//////////////////////////HALF LIFE 3 ES REAL    /////////////////////////////////////////////7
//////////////////////////////////////////////////////////////////////////////////////////////7

void mostrarEstadoDeMemoria(){
    int i;
    time_t horaDelSistema = time(NULL);
    struct tm *tiempo;
    tiempo = localtime(&horaDelSistema);

    FILE* dump = fopen("Dump.txt","a");


    fprintf(dump,"------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    fprintf(dump,"Dump: %d/%d/%d %d:%d:%d\n",tiempo->tm_mday,tiempo->tm_mon+1,tiempo->tm_year+1900,tiempo->tm_hour,tiempo->tm_min,tiempo->tm_sec);


    list_sort(particiones,(void *) tieneMenorOffset);
    for(i = 0 ;  i < list_size(particiones); i++){
        t_particion *particion = list_get(particiones,i);
        if(estaLibre(particion)){

            fprintf(dump,"Partición %d: 0x%08X-0x%08X [L] Size:%dB\n",(i+1),particion->offset,(particion->offset + particion->tamanioParticion - 1),particion->tamanioParticion);
        }else{

            fprintf(dump,"Partición %d: 0x%08X-0x%08X [X] Size:%dB LRU:%lld Cola:%s ID:%d\n",(i+1),particion->offset,(particion->offset + particion->tamanioParticion - 1),particion->tamanioParticion,particion->ultimoAcceso,particion->colaDeMensaje,particion->ID_mensaje);
        }
    }
    fprintf(dump,"------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    fclose(dump);
}

void dumpDeCache(int signal){

    pthread_mutex_lock(&semaforoMensajes);
    printf("\033[1;36m");
    log_info(logObligatorio,"Se solicitó un dump de la caché");
    printf("\033[0m");
    mostrarEstadoDeMemoria();
    pthread_mutex_unlock(&semaforoMensajes);
}

t_nodo* crearNodo(t_particion *particion){

    t_nodo* nodo = malloc(sizeof(t_nodo));
    nodo->id = particion->ID_Particion;
    nodo->tamanioQueOcupa = particion->tamanioParticion;
    nodo->offset = particion->offset;
    nodo->ocupado = 0;
    nodo->padre = NULL;
    nodo->hijoDerecho = NULL;
    nodo->hijoIzquierdo = NULL;
    return nodo;
}

void limpiarNodo(t_nodo* unNodo){
    free(unNodo);
}

void crearNodoPadre(){
    t_particion *particion = obtenerParticion(1,PARTITION_ID);
    nodoRaiz = crearNodo(particion);
}

void asignarPadre(t_nodo* padre, t_nodo* hijo){
    hijo->padre = padre;
}

void asignarHijos(t_nodo* padre, t_nodo* hijoIzquierdo,t_nodo* hijoDerecho){
    padre->ocupado = 1;
    padre->hijoIzquierdo = hijoIzquierdo;
    padre->hijoDerecho = hijoDerecho;
}

void escribirEnMemoria(void *paquete,uint32_t tamanioAEscribir,uint32_t offset){
    memcpy(memoria+offset,paquete,tamanioAEscribir);
}

int obtenerExponente(int unTamanio){
    double i = 0;
    while(1){
        int exponente = pow(2,i);

        if(exponente == unTamanio) return i;
        i++;
    }
}

int obtenerTamanioMinimoBuddy(int unTamanio){

    int tamanioMinimo = 0;
    int exponente = 0;

    while(tamanioMinimo < unTamanio){
        tamanioMinimo = pow(2,exponente);
        exponente++;
    }

    if(tamanioMinimo < tamanio_minimo_particion) tamanioMinimo = tamanio_minimo_particion;
    return tamanioMinimo;
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

int crearHijos(t_nodo* nodoPadre){

    if(nodoPadre->tamanioQueOcupa == tamanio_minimo_particion) return -1;

    uint32_t posicionPadre = obtenerPosicionParticion(nodoPadre->id);
    t_particion *particionPadre = obtenerParticion(nodoPadre->id,PARTITION_ID);

    uint32_t tamanioDeHijo = particionPadre->tamanioParticion/2;
    
    t_particion *particionPrimerHijo = crearParticion(tamanioDeHijo);
    t_particion *particionSegundoHijo = crearParticion(tamanioDeHijo);

    list_add(particiones,particionPrimerHijo);
    list_add(particiones,particionSegundoHijo);
    list_add(particionesLibres,&particionPrimerHijo->ID_Particion);
    list_add(particionesLibres,&particionSegundoHijo->ID_Particion);
    t_nodo* primerHijo = crearNodo(particionPrimerHijo);
    t_nodo* segundoHijo = crearNodo(particionSegundoHijo);

    primerHijo->offset = particionPadre->offset;
    segundoHijo->offset = particionPadre->offset + tamanioDeHijo;

    setearOffset(particionPadre->offset,particionPrimerHijo);
    setearOffset(particionPadre->offset + tamanioDeHijo,particionSegundoHijo);

    asignarHijos(nodoPadre,primerHijo,segundoHijo);

    asignarPadre(nodoPadre,primerHijo);
    asignarPadre(nodoPadre,segundoHijo);

    list_remove(particionesLibres,obtenerPosicionIDParticion(particionPadre->ID_Particion));
    list_remove(particiones,posicionPadre);

    destruirParticion(particionPadre);

    return 0;
}


t_nodo* buscarNodo(t_nodo* nodoPadre,t_particion *particion){

    if(nodoPadre->id == particion->ID_Particion){
        return nodoPadre;
    }

    if(nodoPadre->hijoIzquierdo != NULL && nodoPadre->hijoIzquierdo->id == particion->ID_Particion){
        return nodoPadre->hijoIzquierdo;
    }

    if(nodoPadre->hijoDerecho != NULL && nodoPadre->hijoDerecho->id == particion->ID_Particion){
        return nodoPadre->hijoDerecho;
    }

    if(particion->offset < (nodoPadre->offset + nodoPadre->tamanioQueOcupa/2) ){
        return buscarNodo(nodoPadre->hijoIzquierdo,particion);
    }else{
        return buscarNodo(nodoPadre->hijoDerecho,particion);
    }
}

void ocuparNodo(t_nodo *unNodo,t_particion* unaParticion){
    unNodo->id = unaParticion->ID_Particion;
    unNodo->ocupado = 1;
}

void liberarNodo(t_nodo* unNodo){
    unNodo->ocupado = 0;
}

bool hermanoLibre(t_nodo* unNodo){

    t_nodo* nodoPadre = unNodo->padre;
    t_nodo* nodoHermano;

    if(nodoPadre == NULL) return 0;

    if(nodoPadre->hijoDerecho == unNodo){
        nodoHermano = nodoPadre->hijoIzquierdo;
        return !(nodoHermano->ocupado);
    }

    nodoHermano = nodoPadre->hijoDerecho;
    return !(nodoHermano->ocupado);
}

void matarNodo(t_nodo* unNodo){
    free(unNodo);
}

void reunirHermanos(t_nodo* nodoPadre){

    matarNodo(nodoPadre->hijoDerecho);
    matarNodo(nodoPadre->hijoIzquierdo);

    nodoPadre->hijoDerecho = NULL;
    nodoPadre->hijoIzquierdo = NULL;

    t_particion *particionPadre = crearParticion(nodoPadre->tamanioQueOcupa);

    setearOffset(nodoPadre->offset,particionPadre);
    ocuparNodo(nodoPadre,particionPadre);
    liberarNodo(nodoPadre);


    list_add(particiones,particionPadre);
    list_add(particionesLibres,&particionPadre->ID_Particion);

}

t_nodo *obtenerHermano(t_nodo *unNodo){
    t_nodo* nodoPadre = unNodo->padre;
    t_nodo* nodoHermano;

    if(nodoPadre->hijoDerecho == unNodo){
        nodoHermano = nodoPadre->hijoIzquierdo;

        return nodoHermano;
    }

    nodoHermano = nodoPadre->hijoDerecho;
    return nodoHermano;
}

void consolidarBuddy(t_nodo *unNodo){
    
    if(!hermanoLibre(unNodo)){
        return;
    }
    t_nodo *hermano = obtenerHermano(unNodo);

    printf("\033[1;31m");
    log_info(logObligatorio,"El bloque %d con offset %d y su buddy %d con offset %d se encuentran libres. Se procede a consolidar",unNodo->id,unNodo->offset,hermano->id,hermano->offset);
    printf("\033[0m");

    uint32_t posicionParticionNodo = obtenerPosicionIDParticion(unNodo->id);
    uint32_t posicionParticionHermano = obtenerPosicionIDParticion(hermano->id);

    list_remove(particionesLibres,posicionParticionNodo);
    list_remove(particionesLibres,posicionParticionHermano);

    uint32_t posParticionNodo = obtenerPosicionParticion(unNodo->id);

    list_remove(particiones,posParticionNodo);

    uint32_t posParticionHermano = obtenerPosicionParticion(hermano->id);

    list_remove(particiones,posParticionHermano);

    t_nodo *nodoPadre = unNodo->padre;

    reunirHermanos(nodoPadre);

    return consolidarBuddy(nodoPadre);
}

uint32_t dividirYObtenerIDParticionExacta(t_nodo *unNodo, uint32_t tamanioSolicitado){

    if(unNodo->tamanioQueOcupa == tamanioSolicitado) return unNodo->id;

    if(crearHijos(unNodo) == -1) return -1;

    return dividirYObtenerIDParticionExacta(unNodo->hijoIzquierdo,tamanioSolicitado);
}


void buddySystem(t_mensaje *mensaje,uint32_t frecuenciaLocal){

    uint32_t tamPaquete = mensaje->tamanioPaquete;

    uint32_t IDMensaje = mensaje->ID_mensaje;

    uint32_t tamPaqueteAuxiliar = obtenerTamanioMinimoBuddy(tamPaquete);

    if(tamPaqueteAuxiliar > tamanio_memoria){
        return;
    }

    if(!existeParticionLibreParaCachearMensaje(tamPaqueteAuxiliar)){

        singletonMemoria->ejecutarAlgoritmoDeReemplazo();

        buddySystem(mensaje,frecuenciaLocal);

    }else{
        t_particion *particionLibre = singletonMemoria->obtenerParticionLibre(tamPaqueteAuxiliar);

        uint32_t posicionParticionLibre = obtenerPosicionIDParticion(particionLibre->ID_Particion);

        uint32_t offsetParticionLibre = particionLibre->offset;

        uint32_t tamanioRestante = particionLibre->tamanioParticion - tamPaqueteAuxiliar;

        if(tamanioRestante == 0){

            modificarParticion(particionLibre,IDMensaje,mensaje->colaALaQuePertenece);

            escribirEnMemoria(singletonMemoria->paqueteACachear,tamPaqueteAuxiliar,offsetParticionLibre);

            free(singletonMemoria->paqueteACachear);

            printf("\033[1;32m");
            log_info(logObligatorio,"Se cacheó correctamente el mensaje con BROKER ID [%d]. La posición de inicio de la particion es %d",mensaje->ID_mensaje,offsetParticionLibre);
            printf("\033[0m");

            setearHorarioAcceso(particionLibre);

            list_remove(particionesLibres,posicionParticionLibre);

            t_nodo* nodoParticion = buscarNodo(nodoRaiz,particionLibre);

            ocuparNodo(nodoParticion,particionLibre);

        }else{

            t_nodo* nodoParticion = buscarNodo(nodoRaiz,particionLibre);

            uint32_t idParticionLibre = dividirYObtenerIDParticionExacta(nodoParticion,tamPaqueteAuxiliar);

            if(idParticionLibre == -1) return;

            t_particion *particionNuevaAEscribir = obtenerParticion(idParticionLibre,PARTITION_ID);

            modificarParticion(particionNuevaAEscribir,IDMensaje,mensaje->colaALaQuePertenece);

            escribirEnMemoria(singletonMemoria->paqueteACachear,tamPaqueteAuxiliar,offsetParticionLibre);

            free(singletonMemoria->paqueteACachear);

            printf("\033[1;32m");
            log_info(logObligatorio,"Se cacheó correctamente el mensaje con BROKER ID [%d]. La posición de inicio de la particion es %d",mensaje->ID_mensaje,offsetParticionLibre);
            printf("\033[0m");

            setearHorarioAcceso(particionNuevaAEscribir);

            list_remove(particionesLibres,obtenerPosicionIDParticion(idParticionLibre));

            t_nodo* nodoParticionNueva = buscarNodo(nodoRaiz,particionNuevaAEscribir);

            ocuparNodo(nodoParticionNueva,particionNuevaAEscribir);
        }

    }
}

//////////////////////////////////////////////////////////////////////////////////////////////7
//////////////////////	Zona de trabajo del Gaby	   ///////////////////////////////////////7
//////////////////////////////////////////////////////////////////////////////////////////////7
//////////////////////////ZONA EN MANTENIMIENTO  /////////////////////////////////////////////7
//////////////////////////HALF LIFE 3 ES REAL    /////////////////////////////////////////////7
//////////////////////////////////////////////////////////////////////////////////////////////7


void setearHorarioAcceso(t_particion *particion){
    particion->ultimoAcceso = getHorario();
}

bool seUsoMenosRecientemente(t_particion *unaParticion,t_particion *otraParticion){
    return unaParticion->ultimoAcceso < otraParticion->ultimoAcceso;
}

void liberarParticionBuddySystem(t_particion *particion){

    uint32_t offset = particion->offset;

    uint32_t tamanioParticion = particion->tamanioParticion;

    list_add(particionesLibres,&(particion->ID_Particion));

    void *paqueteVacio = calloc(1,tamanioParticion);

    escribirEnMemoria(paqueteVacio,tamanioParticion,offset);

    list_sort(particiones,(void *) tieneMenorOffset);

    t_nodo *nodoALiberar = buscarNodo(nodoRaiz,particion);

    liberarNodo(nodoALiberar);

    consolidarBuddy(nodoALiberar);

    list_sort(particiones,(void *) tieneMenorOffset);

    free(paqueteVacio);
}

void particionesDinamicas(t_mensaje *mensaje,uint32_t frecuenciaLocal){

    uint32_t tamPaquete = mensaje->tamanioPaquete;

    uint32_t IDMensaje = mensaje->ID_mensaje;

    uint32_t tamPaqueteAuxiliar = tamPaquete;

    if(tamPaquete < tamanio_minimo_particion){
        tamPaqueteAuxiliar = tamanio_minimo_particion;
    }

    if(!existeParticionLibreParaCachearMensaje(tamPaqueteAuxiliar)){

        singletonMemoria->ejecutarAlgoritmoDeReemplazo();

        if(frecuenciaLocal > 0){
            frecuenciaLocal--;
        }

        if(frecuenciaLocal == 0)
        {
            printf("\033[1;31m");
            log_info(logObligatorio,"Frecuencia de compactación: %d. Se procederá a compactar la memoria",frecuenciaLocal);
            printf("\033[0m");

            compactarMemoria();

            frecuenciaLocal = frecuencia_compactacion;
        }

        particionesDinamicas(mensaje,frecuenciaLocal);

    }
    else{
        t_particion *particionLibre = singletonMemoria->obtenerParticionLibre(tamPaqueteAuxiliar);
        
        uint32_t posicionParticionLibre = obtenerPosicionIDParticion(particionLibre->ID_Particion);

        modificarParticion(particionLibre,IDMensaje,mensaje->colaALaQuePertenece);

        uint32_t offsetParticionLibre = particionLibre->offset;

        uint32_t tamanioRestante = particionLibre->tamanioParticion - tamPaqueteAuxiliar;

        if(tamanioRestante == 0){

            escribirEnMemoria(singletonMemoria->paqueteACachear,tamPaqueteAuxiliar,offsetParticionLibre);

            free(singletonMemoria->paqueteACachear);

            printf("\033[1;32m");
            log_info(logObligatorio,"Se cacheó correctamente el mensaje con BROKER ID [%d]. La posición de inicio de la particion es %d",mensaje->ID_mensaje,offsetParticionLibre);
            printf("\033[0m");

            setearHorarioAcceso(particionLibre);

            list_remove(particionesLibres,posicionParticionLibre);
        }
        else{

            escribirEnMemoria(singletonMemoria->paqueteACachear,tamPaqueteAuxiliar,offsetParticionLibre);

            free(singletonMemoria->paqueteACachear);

            printf("\033[1;32m");
            log_info(logObligatorio,"Se cacheó correctamente el mensaje con BROKER ID [%d]. La posición de inicio de la particion es %d",mensaje->ID_mensaje,offsetParticionLibre);
            printf("\033[0m");

            setearHorarioAcceso(particionLibre);

            particionLibre->tamanioParticion = tamPaqueteAuxiliar;

            t_particion *nuevaParticion = crearParticion(tamanioRestante);

            list_add(particiones,nuevaParticion);
            list_replace(particionesLibres,posicionParticionLibre,&(nuevaParticion->ID_Particion));

            setearOffset(offsetParticionLibre+tamPaqueteAuxiliar,nuevaParticion);
        }
    } 
}

void cachearMensaje(t_mensaje *mensaje){
    singletonMemoria->cachearMensaje(mensaje,frecuencia_compactacion);
}

void construirSingletonMemoria(){

    singletonMemoria = malloc(sizeof(t_singletonMemoria));

    if(stringComparator("PARTICIONES",algoritmo_memoria)){
        singletonMemoria->cachearMensaje = particionesDinamicas;
        singletonMemoria->liberarParticion = liberarParticionParticionesDinamicas;
    }else{
        singletonMemoria->cachearMensaje = buddySystem;
        singletonMemoria->liberarParticion = liberarParticionBuddySystem;
        crearNodoPadre();
    }

    if(stringComparator("FF",algoritmo_particion_libre)){
        singletonMemoria->obtenerParticionLibre = firstFit;
    }
    else{
        singletonMemoria->obtenerParticionLibre = bestFit;
    }

    if(stringComparator("FIFO",algoritmo_reemplazo)){
        singletonMemoria->ejecutarAlgoritmoDeReemplazo = FIFO;
    }
    else{
        singletonMemoria->ejecutarAlgoritmoDeReemplazo = LRU;
    }

}

void agregarMensajeACola(t_mensaje *mensaje,t_list *colaDeMensajes){
    list_add(colaDeMensajes,mensaje);
    log_info(logObligatorio,"Un nuevo mensaje fue agregado a la cola %s. Su BROKER ID es [%d]",mensaje->colaALaQuePertenece,mensaje->ID_mensaje);
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

    t_particion *particionActual;
    t_particion *particionSiguiente;
    uint32_t espacioLibre = 0;
    uint32_t offsetLocalCache = 0;

    t_list *listaAux = list_sorted(particiones,(void *) tieneMenorOffset);

    for(int i = 0; i < list_size(listaAux);i++){

        particionActual = list_get(listaAux,i);

        if( i+1 >= list_size(listaAux)){

            particionSiguiente = NULL;
        }else{

            particionSiguiente = list_get(listaAux,i+1);
        }

        
        uint32_t tamParticionActual = particionActual->tamanioParticion;

        if(particionSiguiente == NULL){
            if(estaLibre(particionActual)){

                espacioLibre += tamParticionActual;

                uint32_t posParticionLibre = obtenerPosicionParticion(particionActual->ID_Particion);

                t_particion *particionAux = list_remove(particiones,posParticionLibre);

                destruirParticion(particionAux);
                
            }
            else{

                offsetLocalCache += tamParticionActual;
            }

            break;       
        }

        if(estaLibre(particionActual)){

            uint32_t tamParticionSiguiente = particionSiguiente->tamanioParticion;

            uint32_t offsetSiguiente = particionSiguiente->offset;

            void *particion = malloc(tamParticionSiguiente);

            memcpy(particion,memoria+offsetSiguiente,tamParticionSiguiente);

            memcpy(memoria+offsetLocalCache,particion,tamParticionSiguiente);

            setearOffset(offsetLocalCache,particionSiguiente);

            offsetLocalCache += tamParticionSiguiente;

            espacioLibre += tamParticionActual;

            uint32_t posParticionLibre = obtenerPosicionParticion(particionActual->ID_Particion);

            list_remove(particiones,posParticionLibre);

            i++;

            free(particion);
        }else{

            void *particion = malloc(particionActual->tamanioParticion);

            memcpy(particion,memoria+particionActual->offset,particionActual->tamanioParticion);

            memcpy(memoria+offsetLocalCache,particion,particionActual->tamanioParticion);

            free(particion);

            setearOffset(offsetLocalCache,particionActual);

            offsetLocalCache += tamParticionActual;
        }
    }

    list_clean(particionesLibres);

    t_particion *nuevaParticion = crearParticion(espacioLibre);

    void *paquete = malloc(espacioLibre);

    memcpy(memoria+offsetLocalCache,paquete,espacioLibre);

    setearOffset(offsetLocalCache,nuevaParticion);

    list_add(particiones,nuevaParticion);

    list_add(particionesLibres,&(nuevaParticion->ID_Particion));

    free(paquete);
    list_clean(listaAux);
    list_destroy(listaAux);
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

void eliminarMensaje(uint32_t ID, t_list *colaDeMensajes,char *nombreCola,t_FLAG flag){
    t_mensaje *mensajeAEliminar = obtenerMensaje(ID,colaDeMensajes,BROKER_ID);
    uint32_t posicionMensaje = obtenerPosicionMensaje(mensajeAEliminar,colaDeMensajes);
    list_remove_and_destroy_element(colaDeMensajes,posicionMensaje,(void *) destruirMensaje);

    if(flag == ACK){
        log_info(logExtra,"El mensaje con BROKER_ID [%d] se eliminó de la cola %s. Motivo: Todos los suscriptores recibieron el mensaje.",ID,nombreCola);
    }

    if(flag == REPLACEMENT_ALGORITHM){
        log_info(logExtra,"El mensaje con BROKER_ID [%d] se eliminó de la cola %s. Motivo: Fue seleccionado para su reemplazo.",ID,nombreCola);
    }

}

void destruirMensaje(t_mensaje *mensaje){
    t_list *suscriptoresQueRecibieronMensaje = mensaje->suscriptoresQueRecibieronMensaje;
    t_list *suscriptoresALosQueMandeMensaje = mensaje->suscriptoresALosQueMandeMensaje;
    list_destroy_and_destroy_elements(suscriptoresALosQueMandeMensaje,(void *) destruirIdentificador);
    list_destroy_and_destroy_elements(suscriptoresQueRecibieronMensaje,(void *) destruirIdentificador);
    free(mensaje);
}

void destruirIdentificador(char *identificadorProceso){
    free(identificadorProceso);
}

void iniciarMemoria(){
    memoria = calloc(1,tamanio_memoria);
    particionesLibres = list_create();
    particiones = list_create();

    t_particion *particionInicial = crearParticion(tamanio_memoria);

    setearOffset(0,particionInicial);

    list_add(particiones,particionInicial);
    list_add(particionesLibres,&(particionInicial->ID_Particion));

    construirSingletonMemoria();
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

t_particion *crearParticion(uint32_t tamanioParticion){
    t_particion *particion = malloc(sizeof(t_particion));
    particion->ID_Particion = asignarIDParticion();
    particion->ID_mensaje = -1;
    particion->colaDeMensaje = "";
    particion->tamanioParticion = tamanioParticion;
    particion->ultimoAcceso = getHorario();

    return particion;
}

void destruirParticion(t_particion *particion){
    free(particion);
}

void setearOffset(uint32_t offset,t_particion *particion){
    particion->offset = offset;
}

uint32_t *obtenerIDParticion(t_particion *particion){
    return &(particion->ID_Particion);
}

uint32_t *obtenerOffsetParticion(t_particion *particion){
    return &(particion->offset);
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
            log_error(logExtra,"No se pudo obtener la particion según BROKER ID [%d]",elem);
        }
    }

    if(flag == SIZE_PARTITION){
        particion = obtenerParticionDeParticiones(elem,obtenerTamanioParticion);
        if(particion == NULL){
            log_error(logExtra,"No se pudo obtener la particion según el tamanio [%d]",elem);
        }
    }

    if(flag == PARTITION_ID){
        particion = obtenerParticionDeParticiones(elem,obtenerIDParticion);
        if(particion == NULL){
            log_error(logExtra,"No se pudo obtener la particion con ID [%d]",elem);
        }
    }

    return particion;
}


void *descachearPaquete(t_mensaje *mensaje,char *colaDePokemon){
    uint32_t IDMensaje = mensaje->ID_mensaje;

    uint32_t tamPaquete = mensaje->tamanioPaquete;

    t_particion *particionMensaje = obtenerParticion(IDMensaje,BROKER_ID);

    void *paquete = malloc(tamPaquete);

    memcpy(paquete,memoria+particionMensaje->offset,tamPaquete);

    setearHorarioAcceso(particionMensaje);

    if(tienenIDCorrelativoLosMensajes(colaDePokemon)){
        uint32_t IDCorrelativo = mensaje->ID_correlativo;
        void *paqueteConUnID = insertarIDEnPaquete(IDCorrelativo,paquete,tamPaquete+sizeof(uint32_t),0);
        free(paquete);
        void *paqueteConDosID = insertarIDEnPaquete(IDMensaje,paqueteConUnID,tamPaquete + sizeof(uint32_t),DOUBLE_ID);
        free(paqueteConUnID);
        return paqueteConDosID;
    }
    else{
        void *paqueteConUnID = insertarIDEnPaquete(IDMensaje,paquete,tamPaquete+sizeof(uint32_t),0);
        free(paquete);
        return paqueteConUnID;
    }       
}

bool tienenIDCorrelativoLosMensajes(char *nombreCola){
    return stringComparator(nombreCola,"APPEARED_POKEMON") || stringComparator(nombreCola,"CAUGHT_POKEMON") || stringComparator(nombreCola,"LOCALIZED_POKEMON");
}

bool tieneMenorOffset(t_particion *unaParticion,t_particion *otraParticion){
    return unaParticion->offset < otraParticion->offset;
}

void enviarMensajeRecibidoASuscriptores(t_list *listaSuscriptores,void(*funcionDeEnvio)(t_suscriptor *)){
    list_iterate(listaSuscriptores,(void *) funcionDeEnvio);
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

bool esPrimeraConexion(char *nombreProceso){
    return stringComparator(nombreProceso,"Team") || stringComparator(nombreProceso,"GameCard");
}

//Esta función permite determinar si la respuesta a un mensaje ya fue agregado a la cola de mensajes.
bool existeRespuestaEnCola(uint32_t ID_Correlativo,t_list *colaDeMensajes){
    bool resultado;

    t_list *IDs_Correlativos = list_map(colaDeMensajes,(void *)obtenerIDCorrelativo);

    resultado = existeID(&ID_Correlativo,IDs_Correlativos,intComparator);

    list_clean(IDs_Correlativos);
    list_destroy(IDs_Correlativos);

    return resultado;    
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
            log_info(logExtra,"No se pudo obtener el mensaje con BROKER ID [%d]",ID);
        }
    }

    if(flag == CORRELATIVE_ID){
        mensaje = obtenerMensajeDeCola(ID,colaDeMensajes,obtenerIDCorrelativo);
        if(mensaje == NULL){
            log_info(logExtra,"No se pudo obtener el mensaje con ID CORRELATIVO [%d]",ID);
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


bool todosRecibieronElMensaje(t_mensaje *mensaje,t_list *listaSuscriptores){
    bool resultado = false;

    t_list *idsSuscriptores = list_map(listaSuscriptores,(void *)obtenerIDSuscriptor);

    t_list *acks = mensaje->suscriptoresQueRecibieronMensaje;

    bool existeIDEnACks(char *identificadorSuscriptor){
        return existeID(identificadorSuscriptor,acks,stringComparator);
    }

    for(int i = 0; i < list_size(idsSuscriptores);i++){
        char *idSuscriptor = list_get(idsSuscriptores,i);
        if(!existeIDEnACks(idSuscriptor)){
            return resultado;
        }
    }

    list_clean(idsSuscriptores);
    list_destroy(idsSuscriptores);

    return !resultado;
}

//NO hay que borrar el mensaje una vez que todos los suscriptores lo recibieron.
void validarRecepcionMensaje(uint32_t ID_mensaje,t_operacion nombreCola,char *ID_proceso){
    t_mensaje *mensaje;

    char *idAAgregar = string_duplicate(ID_proceso);
    free(ID_proceso);

    switch (nombreCola) {
        case t_NEW:;
            mensaje = obtenerMensaje(ID_mensaje,NEW_POKEMON,BROKER_ID);

            if(mensaje == NULL){
                log_info(logObligatorio,"%s recibió satisfactoriamente el mensaje con ID %d de la cola NEW_POKEMON",idAAgregar,ID_mensaje);
                log_error(logExtra,"El mensaje con BROKER ID [%d] ya no se encuentra en la cola NEW_POKEMON",ID_mensaje);
                break;
            }

            list_add(mensaje->suscriptoresQueRecibieronMensaje,idAAgregar);
            log_info(logObligatorio,"%s recibió satisfactoriamente el mensaje con ID %d de la cola NEW_POKEMON",idAAgregar,ID_mensaje);

            /*
            if(todosRecibieronElMensaje(mensaje,suscriptores_NEW_POKEMON)){
                eliminarMensaje(ID_mensaje,NEW_POKEMON,"NEW_POKEMON",ACK);
                singletonMemoria->liberadorDeParticiones(obtenerParticion(ID_mensaje,BROKER_ID));
            }
            */

           break;

        case t_LOCALIZED:;
            mensaje = obtenerMensaje(ID_mensaje,LOCALIZED_POKEMON,BROKER_ID);

            if(mensaje == NULL){
                log_info(logObligatorio,"%s recibió satisfactoriamente el mensaje con ID %d de la cola LOCALIZED_POKEMON",idAAgregar,ID_mensaje);
                log_error(logExtra,"El mensaje con BROKER ID [%d] ya no se encuentra en la cola LOCALIZED_POKEMON",ID_mensaje);
                break;
            }

            list_add(mensaje->suscriptoresQueRecibieronMensaje,idAAgregar);
            log_info(logObligatorio,"%s recibió satisfactoriamente el mensaje con ID %d de la cola LOCALIZED_POKEMON",idAAgregar,ID_mensaje);


            /*
            if(todosRecibieronElMensaje(mensaje,suscriptores_LOCALIZED_POKEMON)){
                eliminarMensaje(ID_mensaje,LOCALIZED_POKEMON,"LOCALIZED_POKEMON",ACK);
                singletonMemoria->liberadorDeParticiones(obtenerParticion(ID_mensaje,BROKER_ID));
            }
            */

           break;
        
        case t_GET:;
            mensaje = obtenerMensaje(ID_mensaje,GET_POKEMON,BROKER_ID);

            if(mensaje == NULL){
                log_info(logObligatorio,"%s recibió satisfactoriamente el mensaje con ID %d de la cola GET_POKEMON",idAAgregar,ID_mensaje);
                log_error(logExtra,"El mensaje con BROKER ID [%d] ya no se encuentra en la cola GET_POKEMON",ID_mensaje);
                break;
            }


            list_add(mensaje->suscriptoresQueRecibieronMensaje,idAAgregar);
            log_info(logObligatorio,"%s recibió satisfactoriamente el mensaje con ID %d de la cola GET_POKEMON",idAAgregar,ID_mensaje);

            /*
            if(todosRecibieronElMensaje(mensaje,suscriptores_GET_POKEMON)){
                eliminarMensaje(ID_mensaje,GET_POKEMON,"GET_POKEMON",ACK);
                singletonMemoria->liberadorDeParticiones(obtenerParticion(ID_mensaje,BROKER_ID));
            }
            */

           break;

        case t_APPEARED:;

            mensaje = obtenerMensaje(ID_mensaje,APPEARED_POKEMON,BROKER_ID);

            if(mensaje == NULL){
                log_info(logObligatorio,"%s recibió satisfactoriamente el mensaje con ID %d de la cola APPEARED_POKEMON",idAAgregar,ID_mensaje);
                log_error(logExtra,"El mensaje con BROKER ID [%d] ya no se encuentra en la cola APPEARED_POKEMON",ID_mensaje);
                break;
            }

            list_add(mensaje->suscriptoresQueRecibieronMensaje,idAAgregar);
            log_info(logObligatorio,"%s recibió satisfactoriamente el mensaje con ID %d de la cola APPEARED_POKEMON",idAAgregar,ID_mensaje);

            /*
            if(todosRecibieronElMensaje(mensaje,suscriptores_APPEARED_POKEMON)){
                eliminarMensaje(ID_mensaje,APPEARED_POKEMON,"APPEARED_POKEMON",ACK);
                singletonMemoria->liberadorDeParticiones(obtenerParticion(ID_mensaje,BROKER_ID));
            }
            */

            break;

        case t_CATCH:;
            mensaje = obtenerMensaje(ID_mensaje,CATCH_POKEMON,BROKER_ID);

            if(mensaje == NULL){
                log_info(logObligatorio,"%s recibió satisfactoriamente el mensaje con ID %d de CATCH_POKEMON",idAAgregar,ID_mensaje);
                log_error(logExtra,"El mensaje con BROKER ID [%d] ya no se encuentra en la cola CATCH_POKEMON",ID_mensaje);
                break;
            }

            list_add(mensaje->suscriptoresQueRecibieronMensaje,idAAgregar);
            log_info(logObligatorio,"%s recibió satisfactoriamente el mensaje con ID %d de CATCH_POKEMON",idAAgregar,ID_mensaje);

            /*
            if(todosRecibieronElMensaje(mensaje,suscriptores_CATCH_POKEMON)){
                eliminarMensaje(ID_mensaje,CATCH_POKEMON,"CATCH_POKEMON",ACK);
                singletonMemoria->liberadorDeParticiones(obtenerParticion(ID_mensaje,BROKER_ID));       
            }
            */

           break;

        case t_CAUGHT:;
            mensaje = obtenerMensaje(ID_mensaje,CAUGHT_POKEMON,BROKER_ID);

            if(mensaje == NULL){
                log_info(logObligatorio,"%s recibió satisfactoriamente el mensaje con ID %d de CAUGHT_POKEMON",idAAgregar,ID_mensaje);
                log_error(logExtra,"El mensaje con BROKER ID [%d] ya no se encuentra en la cola CAUGHT_POKEMON",ID_mensaje);
                break;
            }  

            list_add(mensaje->suscriptoresQueRecibieronMensaje,idAAgregar);
            log_info(logObligatorio,"%s recibió satisfactoriamente el mensaje con ID %d de CAUGHT_POKEMON",idAAgregar,ID_mensaje);

            /*
            if(todosRecibieronElMensaje(mensaje,suscriptores_CAUGHT_POKEMON)){
                eliminarMensaje(ID_mensaje,CAUGHT_POKEMON,"CAUGHT_POKEMON",ACK);
                singletonMemoria->liberadorDeParticiones(obtenerParticion(ID_mensaje,BROKER_ID));
            }
            */

            break;

        default:
            break;                 
    }
}

char *asignarIDProceso(char *nombreProceso){
    char *ID_generado;
    char *contadorTeam;
    char *contadorMensajes;
    char *contadorGameCard;

    if(stringComparator("Team",nombreProceso)){
        pthread_mutex_lock(&semaforoIDMensaje);
        contadorIDTeam++;
        contadorTeam = string_itoa(contadorIDTeam);
        contadorMensajes = string_itoa(contadorIDMensaje);
        contadorGameCard = string_itoa(contadorIDGameCard);
        config_set_value(config,"CONTADOR_ID_TEAM",contadorTeam);
        config_set_value(config,"CONTADOR_ID_MENSAJES",contadorMensajes);
        config_set_value(config,"CONTADOR_ID_GAMECARD",contadorGameCard);
        config_save(config);
        ID_generado = string_itoa(contadorIDTeam);
        pthread_mutex_unlock(&semaforoIDMensaje);
    }
    else if(stringComparator("GameCard",nombreProceso)){
        pthread_mutex_lock(&semaforoIDMensaje);
        contadorIDGameCard++;
        contadorTeam = string_itoa(contadorIDTeam);
        contadorMensajes = string_itoa(contadorIDMensaje);
        contadorGameCard = string_itoa(contadorIDGameCard);
        config_set_value(config,"CONTADOR_ID_TEAM",contadorTeam);
        config_set_value(config,"CONTADOR_ID_MENSAJES",contadorMensajes);
        config_set_value(config,"CONTADOR_ID_GAMECARD",contadorGameCard);
        config_save(config);
        ID_generado = string_itoa(contadorIDGameCard);
        pthread_mutex_unlock(&semaforoIDMensaje);
    }

    char *identificadorProceso = string_from_format("%s-%s",nombreProceso,ID_generado);

    free(ID_generado);
    free(contadorTeam);
    free(contadorMensajes);
    free(contadorGameCard);
    
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
    char *contadorTeam = string_itoa(contadorIDTeam);
    char *contadorMensajes = string_itoa(contadorIDMensaje);
    char *contadorGameCard = string_itoa(contadorIDGameCard);

    config_set_value(config,"CONTADOR_ID_TEAM",contadorTeam);
    config_set_value(config,"CONTADOR_ID_MENSAJES",contadorMensajes);
    config_set_value(config,"CONTADOR_ID_GAMECARD",contadorGameCard);
    config_save(config);
    memcpy(&ID,&contadorIDMensaje,sizeof(uint32_t));

    free(contadorTeam);
    free(contadorMensajes);
    free(contadorGameCard);

    pthread_mutex_unlock(&semaforoIDMensaje);

    return ID;
}


t_mensaje *crearMensaje(uint32_t ID,uint32_t ID_correlativo,uint32_t tamanioPaquete,char *nombreCola){
    t_mensaje *mensaje = malloc(sizeof(t_mensaje));
    mensaje->tamanioPaquete = tamanioPaquete;
    mensaje->ID_mensaje = ID;
    mensaje->ID_correlativo = ID_correlativo;
    mensaje->colaALaQuePertenece = nombreCola;
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
    log_path_broker = config_get_string_value(config,"LOG_FILE");

}

void liberarRecursosAdministracionMemoria(){
    free(singletonMemoria);
    list_destroy_and_destroy_elements(particiones,(void * )destruirParticion);
    list_destroy(particionesLibres);
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

void inicializarSemaforos(){
    pthread_mutex_init(&semaforoIDMensaje,NULL);
    pthread_mutex_init(&semaforoMensajes,NULL);
}

void destruirSemaforos(){
    pthread_mutex_destroy(&semaforoIDMensaje);
    pthread_mutex_destroy(&semaforoMensajes);
}

void destruirColas(){
    list_destroy(NEW_POKEMON);
    list_destroy(APPEARED_POKEMON);
    list_destroy(CATCH_POKEMON);
    list_destroy(CAUGHT_POKEMON);
    list_destroy(GET_POKEMON);
    list_destroy(LOCALIZED_POKEMON);
}

uint32_t obtenerPosicionSuscriptor(char *identificadorProceso,t_list *listaSuscriptores){
    t_suscriptor *suscriptor;
    for(int i = 0; i < list_size(listaSuscriptores);i++){
        suscriptor = list_get(listaSuscriptores,i);
        char *IDSuscriptorAComparar = suscriptor->identificadorProceso;
        if(stringComparator(identificadorProceso,IDSuscriptorAComparar)){
            return i;
        }
    }
    return -1;
}

bool existeSuscriptor(char *identificadorProceso,t_list *listaSuscriptores){
    bool resultado;

    t_list *IDs_suscriptores = list_map(listaSuscriptores,(void *) obtenerIDSuscriptor);
    
    resultado = existeID(identificadorProceso,IDs_suscriptores,stringComparator);

    list_clean(IDs_suscriptores);
    list_destroy(IDs_suscriptores);

    return resultado;
}

char *obtenerIDSuscriptor(t_suscriptor *suscriptor){
    return suscriptor->identificadorProceso;
}

void enviarMensajesCacheados(t_suscriptor *suscriptor,t_list *colaMensajes,char *nombreCola,t_operacion operacion,t_FLAG flag){
    t_mensaje *mensaje;
    uint32_t tamPaquete;
    uint32_t tamID = 0;
    void *paqueteAEnviar;
    t_list *acks;

    if(flag == DOUBLE_ID){
        tamID = 2*sizeof(uint32_t);
    }
    else{
        tamID = sizeof(uint32_t);
    }

    if(!existeAlgunACK(suscriptor,colaMensajes)){
        for(int i = 0 ; i < list_size(colaMensajes);i++){
            mensaje = list_get(colaMensajes,i);
            tamPaquete = mensaje->tamanioPaquete;
            paqueteAEnviar = descachearPaquete(mensaje,nombreCola);
            packAndSend(suscriptor->socket_suscriptor,paqueteAEnviar,tamPaquete+tamID,operacion);
            free(paqueteAEnviar);
            printf("\033[1;31m");
            log_info(logObligatorio,"Le mandé a %s el mensaje con BROKER ID [%d] de la cola %s",suscriptor->identificadorProceso,mensaje->ID_mensaje,nombreCola);
            printf("\033[0m");
        }
    }else{
        for(int i = 0; i < list_size(colaMensajes);i++){
            mensaje = list_get(colaMensajes,i);
            acks = mensaje->suscriptoresQueRecibieronMensaje;
            if(!existeACK(suscriptor,acks)){
                paqueteAEnviar = descachearPaquete(mensaje,nombreCola);
                tamPaquete = mensaje->tamanioPaquete;
                packAndSend(suscriptor->socket_suscriptor,paqueteAEnviar,tamPaquete+tamID,operacion);
                free(paqueteAEnviar);
                printf("\033[1;31m");
                log_info(logObligatorio,"Le mandé a %s el mensaje con BROKER ID [%d] de la cola %s",suscriptor->identificadorProceso,mensaje->ID_mensaje,nombreCola);
                printf("\033[0m");
            }
        }
    }
}


void destruirSuscriptor(t_suscriptor *suscriptor){
    char *identificador = suscriptor->identificadorProceso;
    free(identificador);
    free(suscriptor);
}

void agregarSuscriptor(t_suscriptor *suscriptor,t_list *listaSuscriptores,char *nombreCola){
    char *identificadorProceso = suscriptor->identificadorProceso;

    if(existeSuscriptor(identificadorProceso,listaSuscriptores)){
        log_info(logExtra,"Se reconectó %s. Se procederá a actualizar la información del suscriptor.",identificadorProceso,nombreCola);
        uint32_t posSuscriptor = obtenerPosicionSuscriptor(identificadorProceso,listaSuscriptores);
        list_remove_and_destroy_element(listaSuscriptores,posSuscriptor,(void *) destruirSuscriptor);
        list_add(listaSuscriptores,suscriptor);
    }
    else{
        list_add(listaSuscriptores,suscriptor);
        log_info(logObligatorio,"Se suscribió al %s a la cola %s",identificadorProceso,nombreCola);
    }
}


void suscribirProceso(char *idProceso,int cliente_fd,t_operacion operacionSuscripcion){
    //Al suscribir a un proceso a una determinada cola adquiere el caracter de suscriptor.

    t_suscriptor *nuevoSuscriptor;
    char *identificadorProceso = string_duplicate(idProceso);
    free(idProceso);
    nuevoSuscriptor = crearSuscriptor(identificadorProceso,cliente_fd);

    switch (operacionSuscripcion) {
        case t_NEW:;

            agregarSuscriptor(nuevoSuscriptor,suscriptores_NEW_POKEMON,"NEW_POKEMON");

            enviarMensajesCacheados(nuevoSuscriptor,NEW_POKEMON,"NEW_POKEMON",t_NEW,0);
        
            break;

        case t_LOCALIZED:;

            agregarSuscriptor(nuevoSuscriptor,suscriptores_LOCALIZED_POKEMON,"LOCALIZED_POKEMON");

            enviarMensajesCacheados(nuevoSuscriptor,LOCALIZED_POKEMON,"LOCALIZED_POKEMON",t_LOCALIZED,DOUBLE_ID);
            
            break;    

        case t_GET:;
            agregarSuscriptor(nuevoSuscriptor,suscriptores_GET_POKEMON,"GET_POKEMON");

            enviarMensajesCacheados(nuevoSuscriptor,GET_POKEMON,"GET_POKEMON",t_GET,0);

            break;

        case t_APPEARED:;

            agregarSuscriptor(nuevoSuscriptor,suscriptores_APPEARED_POKEMON,"APPEARED_POKEMON");

            enviarMensajesCacheados(nuevoSuscriptor,APPEARED_POKEMON,"APPEARED_POKEMON",t_APPEARED,DOUBLE_ID);

            break; 

        case t_CATCH:;

            agregarSuscriptor(nuevoSuscriptor,suscriptores_CATCH_POKEMON,"CATCH_POKEMON");

            enviarMensajesCacheados(nuevoSuscriptor,CATCH_POKEMON,"CATCH_POKEMON",t_CATCH,0);


            break;

        case t_CAUGHT:;

            agregarSuscriptor(nuevoSuscriptor,suscriptores_CAUGHT_POKEMON,"CAUGHT_POKEMON");

            enviarMensajesCacheados(nuevoSuscriptor,CAUGHT_POKEMON,"CAUGHT_POKEMON",t_CAUGHT,DOUBLE_ID);

            break;

        default:
            break;            
    }    
}