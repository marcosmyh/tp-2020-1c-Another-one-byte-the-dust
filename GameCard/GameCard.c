#include "GameCard.h"

int main(void){
	crearLogger();
	crearLoggerObligatorio();


	leerArchivoDeConfiguracion();
	//doy inicio a mi servidor y obtengo el socket
	socket_servidor = iniciar_servidor(ip_gc,puerto_gc,logger);
	//Servidor iniciado
	log_info(logger,"Server ready for action!");

	iniciar_punto_de_montaje(path_de_tallgrass);


	while(1){
	int cliente = esperar_cliente(socket_servidor,logger);
	if(pthread_create(&thread,NULL,(void*)atender_cliente,&cliente) == 0){
		pthread_detach(thread);
		}

	}
	config_destroy(archivoConfig);
	log_destroy(logger);
	return 0;
}




void crearLogger(){
	char *path = "/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/GameCard/GameCardInformal.log";
	char *nombreArchi = "GamecardInformal";
	logger = log_create(path,nombreArchi,true, LOG_LEVEL_INFO);

}

void crearLoggerObligatorio(){
	char *path = "/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/GameCard/GameCardServerObligatorio.log";
	char *nombrePrograma = "Log_Gamecard";
	loggerObligatorio = log_create(path,nombrePrograma,true, LOG_LEVEL_INFO);
}


int iniciar_servidor(char* ip, char* puerto, t_log* log){
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

    log_info(log, "Listo para escuchar a mi cliente");

    return socket_servidor;
}

int esperar_cliente(int socket_servidor, t_log* log){
	struct sockaddr_in dir_cliente;

	int tam_direccion = sizeof(struct sockaddr_in);
	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);
	log_info(log,"Se conecto un tipazo.");
	return socket_cliente;
}

int crear_conexion(char *ip, char* puerto){
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
		log_error(loggerObligatorio,"No se pudo conectar al broker");
	else log_info(loggerObligatorio,"Hubo conexion ");
	freeaddrinfo(server_info);

	return socket_cliente;
}


void atender_cliente(int* socket){
	Header headerRecibido;
	headerRecibido = receiveHeader(*socket);

	procesar_solicitud(headerRecibido, *socket);
}

void procesar_solicitud(Header headerRecibido, int cliente_fd) {
	//int size;
	//void* msg;
	uint32_t id;
	uint32_t tamanio = headerRecibido.tamanioMensaje;
	uint32_t tamanioPokemon;
	uint32_t posX;
	uint32_t posY;

	char* pokemon;

		switch (headerRecibido.operacion) {
		case t_GET:
			log_info(logger,"Me llegaron mensajes de Suscriber get");

			void* paqueteGet = receiveAndUnpack(cliente_fd, tamanio);
			pokemon = unpackPokemon(paqueteGet);
			tamanioPokemon = strlen(pokemon) + 1;
			id = unpackID(paqueteGet);


			log_info(logger,"Pokemon: %s id: %d",pokemon,id);


			//
			// ACA DEBERIA IR LO QUE DEBE HACERSE CON GET Y PASAR LAS VARIABLES A ESA FUNCION
			//
			// EL ENVIO DE MENSAJE QUEDA EN VERSION DE PRUEBA
			// TODAVIA NO FUNCIONA int seEnvioGet = envioDeMensajeLocalize(pokemon,id); // Prueba

			free(paqueteGet);
			break;

		case t_CATCH:
			log_info(logger,"Me llegaron mensajes de Suscriber Catch");
			void* paqueteCatch = receiveAndUnpack(cliente_fd, tamanio);
			pokemon = unpackPokemon(paqueteCatch);
			tamanioPokemon = strlen(pokemon) + 1;


			id = unpackID(paqueteCatch);
			posX = unpackCoordenadaX_Catch(paqueteCatch, tamanioPokemon);
			posY = unpackCoordenadaY_Catch(paqueteCatch, tamanioPokemon);

			log_info(logger,"Pokemon: %s posX: %d posY: %d id: %d",pokemon,posX,posY,id);


			//
			// ACA DEBERIA IR LO QUE DEBE HACERSE CON CATCH Y PASAR LAS VARIABLES A ESA FUNCION
			//
			uint32_t resultado = 0; // Prueba

			int seEnvioCaught = envioDeMensajeCaught(resultado,id); // Prueba

			free(paqueteCatch);
			break;
		case t_NEW:
			log_info(logger,"Me llegaron mensajes de Suscriber New");
			void* paqueteNew = receiveAndUnpack(cliente_fd, tamanio);
			pokemon = unpackPokemon(paqueteNew);
			tamanioPokemon = strlen(pokemon) + 1;


			id = unpackID(paqueteNew);
			posX = unpackCoordenadaX_New(paqueteNew, tamanioPokemon);
			posY = unpackCoordenadaY_New(paqueteNew, tamanioPokemon);
			uint32_t cantPokemon = unpackCantidadPokemons_New(paqueteNew, tamanioPokemon);

			log_info(logger,"Pokemon: %s posX: %d posY: %d cantidad: %d id: %d",pokemon,posX,posY,cantPokemon, id);


			//
			// ACA DEBERIA IR LO QUE DEBE HACERSE CON NEW Y PASAR LAS VARIABLES A ESA FUNCION
			//

			int seEnvioNew = envioDeMensajeAppeared(pokemon,posX,posY,id); // Prueba

			free(paqueteNew);
			break;
		case 0:
			printf("Se desconecto.");
			pthread_exit(NULL);


		default:
			pthread_exit(NULL);
			break;
		}
}

void leerArchivoDeConfiguracion(void){
	char* path = "/home/utnso/workspace/tp-2020-1c-Another-one-byte-the-dust/GameCard/GameCard.config";
	archivoConfig = config_create(path);
	if (archivoConfig == NULL){
		log_error(logger,"Archivo de configuracion no encontrado");
	}
	setearValoresDeGame(archivoConfig);
	log_info(logger,"La configuracion fue cargada exitosamente");
}

void setearValoresDeGame(t_config *config){
	ip_gc = config_get_string_value(config,"IP_GAMECARD");
	puerto_gc = config_get_string_value(config,"PUERTO_GAMECARD");
	ip_broker = config_get_string_value(config,"IP_BROKER");
	puerto_broker = config_get_string_value(config,"PUERTO_BROKER");
	path_de_tallgrass = config_get_string_value(config,"PUNTO_MONTAJE_TALLGRASS");
	tiempo_de_reconexion = config_get_int_value(config,"TIEMPO_DE_REINTENTO_CONEXION");
	tiempo_de_reintento_operacion = config_get_int_value(config,"TIEMPO_DE_REINTENTO_OPERACION");
}

int envioDeMensajeCaught(uint32_t atrapado, uint32_t idmensaje){
		void* paqueteCaught = pack_Caught(idmensaje,atrapado);
		int socket = crear_conexion(ip_broker,puerto_broker);
		uint32_t tamPaquete =  2*sizeof(uint32_t);
		int resultado = packAndSend(socket,paqueteCaught,tamPaquete,t_CAUGHT);
		close(socket);
		free(paqueteCaught);
		return resultado;
}

int envioDeMensajeAppeared(char* pokemon, uint32_t posx, uint32_t posy, uint32_t idmensaje){
	void* paqueteAppeared = pack_Appeared(idmensaje,pokemon,posx,posy);
	int socket = crear_conexion(ip_broker,puerto_broker);
	uint32_t tamPaquete =  strlen(pokemon) + 1 + 4*sizeof(uint32_t);
	int resultado = packAndSend(socket,paqueteAppeared,tamPaquete,t_APPEARED);
	close(socket);
	free(paqueteAppeared);
  	return resultado;
}

int envioDeMensajeLocalize(char* pokemon,uint32_t idmensaje,uint32_t cantidadParesCoordenadas,uint32_t arrayCoordenadas){
	void* paqueteGet = pack_Localized(idmensaje,pokemon,cantidadParesCoordenadas,arrayCoordenadas);
	//
	//	NO USAR. EN FASE DE PRUEBA
	//
	int socket = crear_conexion(ip_broker,puerto_broker);
	uint32_t tamPaquete = strlen(pokemon) + 1 + 2*sizeof(uint32_t);
	int resultado = packAndSend(socket,paqueteGet,tamPaquete,t_GET);
	close(socket);
	free(paqueteGet);
	return resultado;
}

//////////////////////////////////////////////////
//////////////	UTILIDADES PARA DAR INICIO AL FS
////////////////////////////////////////////////////


// da inicio al punto de montaje
//

void iniciar_punto_de_montaje(char *puntoDeMontaje){
		carpetas_iniciar(puntoDeMontaje);

		//Creamos un string vacio
		char* auxConfigMetadata = string_new();

		//Cargamos el string con la ruta del metadata.bin del FS
		string_append(&auxConfigMetadata,RUTA_DE_METADATA_MONTAJE);
		string_append(&auxConfigMetadata,"/Metadata.bin");

		config_fs = config_create(auxConfigMetadata);

		// Vemos si existe el .bin con la configuracion del FS y cargamos el config a las variables globales
		// En el caso que no exista la carpeta de configuracion crea una por default (Con el ejemplo que estaba en el pdf)
		if (config_fs == NULL || config_keys_amount(config_fs) == 0){
			inicio_default(puntoDeMontaje);
		}


		carga_config_fs();
		bloques_iniciar();
		mostrarRutas();

		// Limpieza de variables

		free(auxConfigMetadata);
}

// Da un inicio default con la configuracion que esta en el pdf
void inicio_default(char* puntoDeMontaje){

	FILE* archivoAux;

	// Cargamos la ruta de metadata.bin
	// Y creamos el archivo metadata.bin VACIO
	char* auxMetadata = string_new();
	string_append(&auxMetadata,puntoDeMontaje);
	string_append(&auxMetadata,"/Metadata/Metadata.bin");
	archivoAux = fopen(auxMetadata,"a");
	fclose(archivoAux);

	// Tratamos al archivo metadata.bin como un config
	// Y le agregamos la configuracion del pdf como default
	// Tamaño del bloque 64
	// cantidad de bloques 5192
	// Numero mágico TALL_GRASS
	config_fs = config_create(auxMetadata);
	config_set_value(config_fs, "BLOCK_SIZE", "10");
	config_set_value(config_fs, "BLOCKS", "5");
	config_set_value(config_fs, "MAGIC_NUMBER", "TALL_GRASS");
	config_save(config_fs);


	// Se crea el archivo del bitmap
	char* auxBitMap = string_new();
	string_append(&auxBitMap,puntoDeMontaje);
	string_append(&auxBitMap,"/Metadata/Bitmap.bin");
	archivoAux = fopen(auxBitMap,"a");
	fclose(archivoAux);
	free(auxMetadata);
	free(auxBitMap);
}

// Inicializa los bloques
// Por ahora solo los inicia en 0 y sobreescribe. Hay que ver como hacer para que no
// sobreescriba datos anteriores
void bloques_iniciar(){
	FILE *bloque;
	int i;
	// Se crean los bloques pero hay un problema porque elimina los bloques
	// que anteriormente fueron creados
	// en esta funcion tengo mucha perdida de memoria. Revisar
	for(i = 1; i <= BLOQUES_DE_BITMAP ; i++){
		printf("Creo el bloque %i \n",i);
		char* bloque_a_crear = string_new();
		string_append(&bloque_a_crear,RUTA_DE_BLOQUES);
		string_append(&bloque_a_crear,"/");
		string_append(&bloque_a_crear,string_itoa(i));
		string_append(&bloque_a_crear,".bin");
		bloque = fopen(bloque_a_crear,"a");
		fclose(bloque);
		free(bloque_a_crear);
	}
}


//	Crea las carpetas que debe tener el fs (las carpetas bases)
// 	que son metadata,files y blocks.
void carpetas_iniciar(char* puntoDeMontaje){

	// Se crea la carpeta de metadata
	RUTA_DE_METADATA_MONTAJE = string_new();
	string_append(&RUTA_DE_METADATA_MONTAJE,puntoDeMontaje);
	string_append(&RUTA_DE_METADATA_MONTAJE,"/Metadata");
	mkdir(RUTA_DE_METADATA_MONTAJE,0777);
	// Se crea la carpeta de FILES
	RUTA_DE_FILES = string_new();
	string_append(&RUTA_DE_FILES,puntoDeMontaje);
	string_append(&RUTA_DE_FILES,"/Files");
	mkdir(RUTA_DE_FILES,0777);

	// Se crea la carpeta pokemon dentro de files
	RUTA_DE_POKEMON = string_new();
	string_append(&RUTA_DE_POKEMON,RUTA_DE_FILES);
	string_append(&RUTA_DE_POKEMON,"/Pokemon");
	mkdir(RUTA_DE_POKEMON,0777);

	// Se crea el metadata del pokemon diciendo que es Directorio
	FILE* metaPKM = fopen(obtener_direccion_metadata(RUTA_DE_POKEMON),"a");
	fclose(metaPKM);
	t_config* metadataDeCarpetaPokemon = obtener_metadata_de_ruta(RUTA_DE_POKEMON);
	config_set_value(metadataDeCarpetaPokemon,"DIRECTORY","Y");
	config_save(metadataDeCarpetaPokemon);
	config_destroy(metadataDeCarpetaPokemon);


	// Se crea la carpeta de Blocks
	RUTA_DE_BLOQUES = string_new();
	string_append(&RUTA_DE_BLOQUES,puntoDeMontaje);
	string_append(&RUTA_DE_BLOQUES,"/Blocks");
	mkdir(RUTA_DE_BLOQUES,0777);
}


// Carga la configuracion que está en el metadata.bin
// del fs. Toma el tamaño de bloque, bloques del bitmap y el magicnumber
void carga_config_fs(){
	TAMANIO_DE_BLOQUE = config_get_int_value(config_fs,"BLOCK_SIZE");
	BLOQUES_DE_BITMAP = config_get_int_value(config_fs,"BLOCKS");
	MAGIC_NUMBER = config_get_string_value(config_fs,"MAGIC_NUMBER");
}


////////////////////////////////////////////////
///////////////////////UTILIDADES VARIAS
////////////////////////////////////////////////



// MUESTRA LAS RUTAS AUNQUE ESTARIA BUENO USAR EL LOG(UNO CAPAZ PARA NOSOTROS) PARA INFORMARs
// NO SIRVE PARA EL TP PERO PODEMOS VERIFICAR COSAS CON ESTO
void mostrarRutas(){
	printf("------------------------\n");
		printf("Mostrando rutas de trabajo \n");
		printf("Metadata: %s \n",RUTA_DE_METADATA_MONTAJE);
		printf("Files: %s \n",RUTA_DE_FILES);
		printf("Blocks: %s \n",RUTA_DE_BLOQUES);
		printf("\n");
		printf("Mostrando config del FS \n");
		printf("El tamaño del bloque del metadata es: %i \n",TAMANIO_DE_BLOQUE);
		printf("La cantidad de bloques es: %i \n",BLOQUES_DE_BITMAP);
		printf("El magic number es: %s \n",MAGIC_NUMBER);
}

// Retorna el nombre del archivo dado el path
// Es decir retorna el último string despues del último "/"
// PARA FUSE
char* obtener_nombre_de_archivo(char *path){
	char **aux;
	char *nombre;

	aux = string_split(path, "/");

	// Le restamos dos ya si hacemos -1 retornamos NULL por el string split
	// y el -2 retorna el último nombre

	nombre = aux[(string_length(*aux) - 2)];
	free(aux);
	return nombre;
}

//Agrega la direccion del metadata de la ruta ingresada
char* obtener_direccion_metadata(char *path){

	char* rutaMetadata;
	rutaMetadata = string_new();

	string_append(&rutaMetadata,path);
	string_append(&rutaMetadata,"/Metadata.bin");

	return rutaMetadata;
}

//	retorna el metadata del archivo
// y hay que tratarlo como un config
// Si retorna NULL no existe la metadata o la ruta
t_config* obtener_metadata_de_ruta(char *path){
	char* rutaMetadata;
	t_config *metadata;
	rutaMetadata = obtener_direccion_metadata(path);

	metadata = config_create(rutaMetadata);
	free(rutaMetadata);
	return metadata;
}

// Mediante una ruta retorna si es un directorio o registro
// 1 si es registro
// 2 si es directorio
// -1 si es error

int esDirectorio(char* ruta){
	t_config* unMeta;
	char* tipoDeArchivo;
	unMeta = obtener_metadata_de_ruta(ruta);
	if(unMeta == NULL) return -1;

	tipoDeArchivo = config_get_string_value(unMeta,"DIRECTORY");

	if(string_equals_ignore_case(tipoDeArchivo,"Y")){
		config_destroy(unMeta);
		return 2;
	}
	if(string_equals_ignore_case(tipoDeArchivo,"N")){
		config_destroy(unMeta);
		return 1;
	}
	else return -1;
}


////////////////////////////////////////////////
////////////////////////// COSAS DEL POKEMON
//////////////////////////////////////////

// Retorna el metada de un pokemon
// dado solo su nombre (Puede retornar el metadata del directorio, cuidado)
//
t_config* obtener_metadata_de_pokemon(char *nombrePokemon){
	char *ruta_pokemon;
	t_config* configPokemon;

	ruta_pokemon = string_new();
	string_append(&ruta_pokemon,RUTA_DE_FILES);
	string_append(&ruta_pokemon,"/Pokemon/");
	string_append(&ruta_pokemon,nombrePokemon);

	configPokemon = config_create(ruta_pokemon);
	free(ruta_pokemon);

	return configPokemon;
}

// Busca si existe el pokemon buscado encontrando su metadata
// Aunque se encuentra en fase de prueba ya que si existe solo el directorio
// va a retornar que si existe el pokemon
// Retorna 0 si no existe el pokemon
// sino retorna 1 si existe

bool existePokemon(char* nombrePokemon){
	t_config *metadataPokemon;

	metadataPokemon = obtener_metadata_de_pokemon(nombrePokemon);

	if(metadataPokemon == NULL) {
		return 0;
	}

	config_destroy(metadataPokemon);
	return 1;
}

//	Crea un archivo con su metadata del nombre del pokemon
//	Obviamente el archivo va a estar vacio hasta ser llenado
void crearPokemon(char* nombrePokemon){
	char* path_pokemon = string_new();
	string_append(&path_pokemon,RUTA_DE_POKEMON);
	string_append(&RUTA_DE_POKEMON,"/");
	string_append(&RUTA_DE_POKEMON,nombrePokemon);
	mkdir(path_pokemon,0777);

	string_append(&RUTA_DE_POKEMON,"/Metadata.bin");
	FILE *archivoVacioMetadata = fopen(RUTA_DE_POKEMON,"rw");
	fclose(archivoVacioMetadata);

	t_config* metaPokemon = config_create(RUTA_DE_POKEMON);
	config_set_value(metaPokemon,"DIRECTORY","N");
	config_set_value(metaPokemon,"SIZE","0");
	config_set_value(metaPokemon,"BLOCKS","[]");
	config_set_value(metaPokemon,"OPEN","N");
	config_save(metaPokemon);
	config_destroy(metaPokemon);
	free(path_pokemon);
}

void abrirArchivo(char* nombrePokemon){
	// CAPAZ UN MUTEX
	t_config* metaPokemon = obtener_metadata_de_pokemon(nombrePokemon);
	config_set_value(metaPokemon,"OPEN","Y");
	config_save(metaPokemon);
	config_destroy(metaPokemon);
}

void cerrarArchivo(char* nombrePokemon){
	// CAPAZ UN MUTEX
	t_config* metaPokemon = obtener_metadata_de_pokemon(nombrePokemon);
	config_set_value(metaPokemon,"OPEN","N");
	config_save(metaPokemon);
	config_destroy(metaPokemon);
}

bool archivoAbierto(char* nombrePokemon){
	int valor;
	t_config* metaPokemon = obtener_metadata_de_pokemon(nombrePokemon);
	char *estaAbierto = config_get_string_value(metaPokemon,"OPEN");
	if(string_equals_ignore_case(estaAbierto,"Y")) valor = 1;
	else valor = 0;

	config_destroy(metaPokemon);
	return valor;
}
