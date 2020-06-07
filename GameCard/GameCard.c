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
	mostrarBitmap(); // ---------- SACAR
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
			pokemon = unpackPokemonGet(paqueteGet);
			tamanioPokemon = strlen(pokemon) + 1;
			id = unpackID(paqueteGet);


			log_info(logger,"Pokemon: %s id: %d",pokemon,id);


			procedimientoGET(id,pokemon);
			//
			// EL ENVIO DE MENSAJE QUEDA EN VERSION DE PRUEBA
			// TODAVIA NO FUNCIONA int seEnvioGet = envioDeMensajeLocalize(pokemon,id); // Prueba

			free(paqueteGet);
			break;

		case t_CATCH:
			log_info(logger,"Me llegaron mensajes de Suscriber Catch");
			void* paqueteCatch = receiveAndUnpack(cliente_fd, tamanio);
			pokemon = unpackPokemonCatch(paqueteCatch);
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
			pokemon = unpackPokemonNew(paqueteNew);
			tamanioPokemon = strlen(pokemon) + 1;


			id = unpackID(paqueteNew);
			posX = unpackCoordenadaX_New(paqueteNew, tamanioPokemon);
			posY = unpackCoordenadaY_New(paqueteNew, tamanioPokemon);
			uint32_t cantPokemon = unpackCantidadPokemons_New(paqueteNew, tamanioPokemon);


			procedimientoNEW(id,pokemon,posX,posY,cantPokemon);
			log_info(logger,"Pokemon: %s posX: %d posY: %d cantidad: %d id: %d",pokemon,posX,posY,cantPokemon, id);
			mostrarBitmap();
			//
			// Procedimiento NEW casi terminado
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
	tiempo_de_retardo = config_get_int_value(config,"TIEMPO_RETARDO_OPERACION");
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
		iniciarBitmap();

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
	for(i = 1; i <= CANTIDAD_DE_BLOQUES ; i++){
		char* bloque_a_crear = string_new();
		string_append(&bloque_a_crear,RUTA_DE_BLOQUES);
		string_append(&bloque_a_crear,"/");
		string_append(&bloque_a_crear,string_itoa(i));
		string_append(&bloque_a_crear,".bin");
		bloque = fopen(bloque_a_crear,"a");
		fclose(bloque);
		free(bloque_a_crear);
	}

	log_info(logger,"Bloques creados");
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
	CANTIDAD_DE_BLOQUES = config_get_int_value(config_fs,"BLOCKS");
	MAGIC_NUMBER = config_get_string_value(config_fs,"MAGIC_NUMBER");
}

// Actualiza el bitmap. Busca en los bloques a los bloque que estan ocupados o contienen datos y coloca
// en el bitmap 1 para indicar que esta ocupado ese bloque
void actualizarBitmap(){
	int i;
	int tamanio;
	for(i = 1; i <= CANTIDAD_DE_BLOQUES; i++){
		FILE *bloque;
		char* rutaDeBloque = string_new();
		string_append(&rutaDeBloque,RUTA_DE_BLOQUES);
		string_append(&rutaDeBloque,"/");
		string_append(&rutaDeBloque,string_itoa(i));
		string_append(&rutaDeBloque,".bin");

		bloque = fopen(rutaDeBloque,"r");
		fseek(bloque,0,SEEK_END);
		tamanio = ftell(bloque);
		if(tamanio != 0){

			bitarray_set_bit(bitmap,i-1);
		}else bitarray_clean_bit(bitmap,i-1);

		fclose(bloque);
		free(rutaDeBloque);

	}

}

// Inicia el bitmap si existe el archivo bitmap.bin, si no existe crea un bitmap basado en el metadata.
void iniciarBitmap(){
	FILE *archi;
	int tamanio;

	char* directorioBitmap = string_new();
	string_append(&directorioBitmap,RUTA_DE_METADATA_MONTAJE);
	string_append(&directorioBitmap,"/Bitmap.bin");
	archi = fopen(directorioBitmap,"wb+");
	fseek(archi,0,SEEK_END);
	tamanio = ftell(archi);
	fseek(archi,0,SEEK_SET);
	if(tamanio == 0){
	//SE CREA EL BITMAP VACIO
	archi = fopen(directorioBitmap,"wb+");
	char* bitArray_vacio = calloc(1,((CANTIDAD_DE_BLOQUES+7)/8));
	fwrite((void* )bitArray_vacio,((CANTIDAD_DE_BLOQUES+7)/8),1,archi);
	//fclose(archi);
	free(bitArray_vacio);
	}

	int fd = fileno(archi);
	fseek(archi,0,SEEK_END);
	int tamanioArchi = ftell(archi);
	fseek(archi,0,SEEK_SET);

	//SE "LEE" el contenido del archivo
	char* bitarray = (char*) mmap(NULL, tamanioArchi, PROT_READ | PROT_WRITE , MAP_SHARED, fd,0);

	bitmap = bitarray_create_with_mode(bitarray, tamanioArchi, LSB_FIRST);

	actualizarBitmap();
	log_info(logger,"Bitmap cargado y actualizado");

	free(directorioBitmap);
	fclose(archi);
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
		printf("La cantidad de bloques es: %i \n",CANTIDAD_DE_BLOQUES);
		printf("El magic number es: %s \n",MAGIC_NUMBER);
}

// Retorna el nombre del archivo dado el path
// Es decir retorna el último string despues del último "/"

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

// Funcion que sirve solo para orientarse. No es necesaria ni nada.
void mostrarBitmap(){
	int i;
	printf("Mostrando bitmap: ");
	for(i = 0; i < CANTIDAD_DE_BLOQUES; i++){
		if(bitarray_test_bit(bitmap,i)) printf("%d",1);
		else printf("%d",0);
	}
	printf("\n");
}

// Retorna el length de un puntero a puntero
int length_punteroAPuntero(char** punteroAPuntero){
	int i = 0;

	while(punteroAPuntero[i] != NULL){
		i++;
	}
	return i;
}

// Limpia un puntero a puntero
void limpiarPunteroAPuntero(char** puntero){
	int i = 0;

	while(puntero[i] != NULL){
		free(puntero[i]);
		i++;
	}
	free(puntero);
}

// En construccion
void refrescarBloquesLibres(){

		//bloquesLibres = 0;
		int i;
		for(i = 0; i < CANTIDAD_DE_BLOQUES; i++){
			if(bitarray_test_bit(bitmap,i));
			//else bloquesLibres++;
		}


}



// Recibe un arreglo de arrays y el arrayObjetivo.
// Retorna el numero de linea donde se encuentra dicho array
int obtenerLinea(char** lineas,char* array){
				int i = 0;
				while(lineas[i] != NULL){

				if(string_contains(lineas[i],array)){
					 return i;
					}else i++;
				}
				return -1;
}
// Recibe un array y un caracter
// Busca la posicion de un caracter de un array y retorna su posicion
int buscarPosicionDeCaracter(char* array,char *caracter){
	int i;
	int tamanio = strlen(array);
	for(i = 0; i < tamanio; i++){

		if(array[i] == caracter) return i;
	}

	return -1;
}

// Tan solo crea un path del tipo bloque y lo retorna
char* crearPathDeBloque(char* nroDeBloque){
	char* rutaBloque = string_from_format("%s/%s.bin",RUTA_DE_BLOQUES,nroDeBloque);
	return rutaBloque;
}

// Recibe un arreglo de posiciones con sus cantidades(1-1=10), la nueva linea que va a reemplazar en el nroDeLinea pasado
// y el nroDeLinea que se va a realizar el reemplazo
// retorna un string que contiene todas las lineas (Ya habiendo reemplazado la linea deseada)
char* actualizarPosicion(char** posicionesSeparadas,char* lineaAActualizar,int nroDeLinea){
	int i = 0;
	char* posicionActualizada = string_new();


	while(posicionesSeparadas[i] != NULL){

		if(i != nroDeLinea) {

			string_append(&posicionActualizada,posicionesSeparadas[i]);

		}else{
			string_append(&posicionActualizada,lineaAActualizar);
		}

		if(posicionesSeparadas[i+1] != NULL){
			string_append(&posicionActualizada,"\n");
		}


		i++;
	}

	return posicionActualizada;
}

// Escribe un bloque, le debemos decir el nombre/numero de bloque, lo que deseamos escribir y
// el desplazamiento dentro de ese bloque(pisa los datos que se encuentren en esa posicion y las que siguen).
// Si es 0, empieza a escribir como si estuviera vacio. (Va a pisar lo que haya dentro)
int escribirBloquePosicionandoPuntero(char* nombreDeBloque,char* stringAEscribir,int desplazamiento){

	char* rutaBloque = crearPathDeBloque(nombreDeBloque);

	FILE* bloque = fopen(rutaBloque,"r+w");
	//fseek(bloque,0,SEEK_END);
	//int tamanioBloque = ftell(bloque);


	int bytesAEscribir = TAMANIO_DE_BLOQUE - desplazamiento;

	if(strlen(stringAEscribir) < bytesAEscribir){
		bytesAEscribir = strlen(stringAEscribir);
	}

	fseek(bloque,desplazamiento,SEEK_CUR);

	fwrite(stringAEscribir,bytesAEscribir,1,bloque);

	fclose(bloque);

	free(rutaBloque);

	return bytesAEscribir;

}

//Recibe una posicion y su cantidad (1-1=10) y le suma cantidadASumar(1)
// retorna el nuevo array con la nueva cantidad (1-1=11)
char* sumarCantidadPokemon(char* lineaPokemon,int cantidadASumar){

	int cantidadPokemon = obtenerCantidad(lineaPokemon);

	cantidadPokemon += cantidadASumar;

	char** arraySeparado = string_split(lineaPokemon,"=");

	char* nuevaLineaPokemon = string_from_format("%s=%d",arraySeparado[0],cantidadPokemon);

	limpiarPunteroAPuntero(arraySeparado);

	return nuevaLineaPokemon;
}

//	Recibe un arreglo de posiciones con sus cantidades y una posicion objetivo
//	retorna el desplazamiento que habría desde el iniico hasta el caracter despues del "=" de esa posicion objetivo. (es decir la primera
//	posicion de la cantidad pokemon)
//	como si fuera que lineasSeparadas estuviera concatenada con \n en cada una de las concatenaciones de cada linea
int desplazamientoDelArrayHastaCantPokemon(char** posicionesSeparadas,char* posObjetivo){
	int i = 0;
	int offset = 0;

	//Vemos si es el fin de posicionesSeparadas
	while(posicionesSeparadas[i] != NULL){
		//Vemos si esa posicion coincide con nuestra posicion objetivo
		if(string_contains(posicionesSeparadas[i],posObjetivo))break;
		//Si no sumamos al desplazamiento la cantidad de caracteres de esa posicion
		offset += strlen(posicionesSeparadas[i]);
		//Avanzamos de linea
		i++;
	}
	// suma el offset, numero de linea(ya que hay que sumar 1 caracter por cada linea saltada ya que en el posicionesSeparadas no esta
	// contemplado el \n) ,la posicion del caracter "=" en la linea y el +1 porque avanzamos un caracter más despues del "="

	offset = offset + i + 1 + buscarPosicionDeCaracter(posicionesSeparadas[i],'=');

	return offset;
}

// Retorna la posicion del bloque afectado
int bloqueAfectado(int desplazamiento){
	return desplazamiento/TAMANIO_DE_BLOQUE;
}

// Retorna el desplazamiento que hay que tener en el bloque.
//  El desplazamiento dado por parametro debe ser basado en el desplazamiento TOTAL del array que contiene todas las posiciones.
int desplazamientoEnBloque(int desplazamiento){
	return desplazamiento%TAMANIO_DE_BLOQUE;
}

// Retorna la cantidad de pokemones dado una lineaPokemon/posicionYCantidad (1-1=10)
// retorna 10
int obtenerCantidad(char* lineaPokemon){

	char** posicionYCantidad = string_split(lineaPokemon,"=");

			int numeroBuscado = atoi(posicionYCantidad[1]);



			printf("Cantidad: %d\n",numeroBuscado);
			limpiarPunteroAPuntero(posicionYCantidad);
			return numeroBuscado;
}

//	Recibe el nombre del bloque, el desplazamiento que debe haber en el bloque y los caracter que hay que colocar
// pisa al caracter que se encuentre en dicha posicion
// Es util solo cuando la  cantidadDePokemones antigua y actualizada tienen el mismo length
// Es decir: Que si antes de agregar tiene 10 y leugo de agregar tiene 18 es util usar esta funcion
// Ya que nos ahorramos el hecho de reescribir todos los bloques
void sobreescribirUnCaracter(char* nombreDeBloque,int desplazamiento,char* caracterAEscribir){

	char* rutaBloque = crearPathDeBloque(nombreDeBloque);

	printf("Ruta de bloque %s\n",rutaBloque);
	FILE* bloque = fopen(rutaBloque,"r+w");

	fseek(bloque,desplazamiento,SEEK_CUR);

	fputs(caracterAEscribir,bloque);

	fclose(bloque);

	free(rutaBloque);
}

// Funcion en proceso. dado un string va a retornar la cantidad de bloques que se necesitan para guardar
// ese string
int bloqueNecesariosParaEscribir(char* datos){
	int  cantidadDeBytes = strlen(datos);

	int bloquesNecesarios = (cantidadDeBytes/TAMANIO_DE_BLOQUE) + 1;

	return bloquesNecesarios;
}

// Funcion en proceso. Se le da la cantidad de bloques que debe reservar el pokemon
// Deberia retornar 0 si fueron solicitados todos
// o -1 si es que no hay suficiente espacio para asignar todos esos bloques
int solicitarBloquesParaPokemon(int cantidadDeBloques,char* pokemon){
	int i;


		for(i = 0; i < cantidadDeBloques ; i++){
		int nuevoBloque = solicitarBloqueVacio();
		printf("Voy a agregar el nuevo bloque\n");
		agregarBloqueAPokemon(pokemon,nuevoBloque);

		}
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
	string_append(&ruta_pokemon,"/Metadata.bin");
	configPokemon = config_create(ruta_pokemon);
	free(ruta_pokemon);

	return configPokemon;
}

// Retorna si existe el pokemon: NO GENERA LEAKS
int existePokemon(char* nombrePokemon){

	 // --- aca hay que usar la global path de files
	//char* pathDePokemon = string_new();
//	string_append(&pathDePokemon,RUTA_DE_POKEMON);
	//string_append(&pathDePokemon,"/");
//	string_append(&pathDePokemon,nombrePokemon);
	//string_append(&pathDePokemon,"/");
//	string_append(&pathDePokemon,"Metadata.bin");

	t_config* config_pokemon = obtener_metadata_de_pokemon(nombrePokemon);

	if(config_pokemon == NULL){
		printf("No existe el pokemon %s\n",nombrePokemon);
		return 0;
	}
	printf("Existe el pokemon %s\n",nombrePokemon);
	config_destroy(config_pokemon);
	return 1;
}

//	Crea un archivo con su metadata del nombre del pokemon
//	Obviamente el archivo va a estar vacio hasta ser llenado
void crearPokemon(char* nombrePokemon){

	char* path_pokemon = string_new();
	string_append(&path_pokemon,RUTA_DE_POKEMON);
	string_append(&path_pokemon,"/");
	string_append(&path_pokemon,nombrePokemon);

	mkdir(path_pokemon,0777);

	string_append(&path_pokemon,"/Metadata.bin");
	FILE *archivoVacioMetadata = fopen(path_pokemon,"a");
	fclose(archivoVacioMetadata);

	t_config* metaPokemon = config_create(path_pokemon);
	config_set_value(metaPokemon,"DIRECTORY","N");
	config_set_value(metaPokemon,"SIZE","0");
	config_set_value(metaPokemon,"BLOCKS","[]");
	config_set_value(metaPokemon,"OPEN","N");
	config_save(metaPokemon);
	config_destroy(metaPokemon);

	free(path_pokemon);
}

//Abre el archiv ode pokemon
void abrirArchivo(char* nombrePokemon){
	// CAPAZ UN MUTEX
	t_config* metaPokemon = obtener_metadata_de_pokemon(nombrePokemon);
	config_set_value(metaPokemon,"OPEN","Y");
	config_save(metaPokemon);
	config_destroy(metaPokemon);
}

// Cierra el archivo de pokemon
void cerrarArchivo(char* nombrePokemon){
	// CAPAZ UN MUTEX
	t_config* metaPokemon = obtener_metadata_de_pokemon(nombrePokemon);
	config_set_value(metaPokemon,"OPEN","N");
	config_save(metaPokemon);
	config_destroy(metaPokemon);
}
// Dice si el archivo del pokemon esta abierto o no
bool archivoAbierto(char* nombrePokemon){
	int valor;
	t_config* metaPokemon = obtener_metadata_de_pokemon(nombrePokemon);
	char *estaAbierto = config_get_string_value(metaPokemon,"OPEN");
	if(string_equals_ignore_case(estaAbierto,"Y")) valor = 1;
	else valor = 0;

	config_destroy(metaPokemon);
	return valor;
}

void *reintentoAbrir(void* argumentos){
	struct arg_estructura* valores = argumentos;

	while(1){
	if(!archivoAbierto(valores->nombrePokemon)){
		printf("El Archivo se puede abrir! \n");
		pthread_exit(NULL);
		}
	printf("Esperando .... %d segundos \n",valores->tiempo);
	sleep(valores->tiempo);
	}

}

// RETORNA LOS BLOQUES DEL METADATA FUNCIONA COMO STRING: NO DA LEAKS
char* obtenerArrayDebloques(char* pokemon){
			char* direccion = string_new();
			string_append(&direccion,RUTA_DE_POKEMON);
			string_append(&direccion,"/");
			string_append(&direccion,pokemon);
			string_append(&direccion,"/Metadata.bin");

			t_config* archiPokemon = config_create(direccion);
			char* auxBloques = string_duplicate(config_get_string_value(archiPokemon,"BLOCKS"));

			config_destroy(archiPokemon);

			free(direccion);
			return auxBloques;
}

// Funcion auxiliar de  obtenerContenidoDeArchivos()
// lo que hace es recibir un char con el numero del archivo
// y llena un string con el contenido que tiene dicho archivo
// Se debe agregar al final \0 NO GENERA LEAKS
char* obtener_contenido_de_archivo(char* nroDeArchivo){
	char* rutaDeFile = string_new();
	string_append(&rutaDeFile,RUTA_DE_BLOQUES);
	string_append(&rutaDeFile,"/");
	string_append(&rutaDeFile,nroDeArchivo);
	string_append(&rutaDeFile,".bin");

	FILE* archi = fopen(rutaDeFile,"r");
		fseek(archi, 0, SEEK_END);
		int size = ftell(archi);
		fseek(archi,0,SEEK_SET);
		char* array = malloc(size+1);
		fread(array,size,1,archi);
		free(rutaDeFile);
		fclose(archi);
		array[size] = '\0';
		return array;
}

//-----------------___ESTOY TRABAJANDO EN ESTA . GENERA LEAKS MIRAR PROXIMAMENTE
char* obtenerContenidoDeArchivos(char* bloques){
	// Los separa ["10","2","3"]
	char** arrayBloques = string_get_string_as_array(bloques);
	char* arrayDeArchivo = string_new();
	int i = 0;
	//Recorre cada numero y obtiene el contenido

	char* contenidoAux;

	while(arrayBloques[i] != NULL){
		contenidoAux = obtener_contenido_de_archivo(arrayBloques[i]);
		//	La linea obtenida es concatenada
		//	asi forma un string largo con todo el contenido del archivo
		string_append(&arrayDeArchivo,contenidoAux);

		//printf("arrayDeArchivo: %s \n",arrayDeArchivo);
		//free(contenidoAux);// MIRAR ACA
		i++;

	}
	// MIRAR ESTO se necesita funcion que elimine puntero a punteros
	free(contenidoAux);
	limpiarPunteroAPuntero(arrayBloques);
	return arrayDeArchivo;

}
// Dice que si el arrayPosicion pertenece a las array de lineas(Que es la informacion de los bloques en un array)
// Hay que ver otra manera porque supongo que genera mucha carga a la memoria cargar todo un archivo completo
// El array posicion debe ser "2-2=" o "2-2" por ejemplo
bool contieneEstaPosicion(char* lineas,char* arrayPosicion){
			return string_contains(lineas,arrayPosicion);
}

// Retorna un entero que indica el numero de bloque vacio
int solicitarBloqueVacio(){
	int i;
	for(i = 0; i < CANTIDAD_DE_BLOQUES; i++){
		if(!bitarray_test_bit(bitmap,i))return i+1;
	}
	return -1;
}


//Recibe el nombre del pokemon y un numero de bloque que va a ser
// añadido BLOCKS de su metadata
void agregarBloqueAPokemon(char* nombrePokemon,int numeroDeBloque){
	t_config* pokemon = obtener_metadata_de_pokemon(nombrePokemon);

	char** bloques = config_get_array_value(pokemon,"BLOCKS");
	int i = 0;
	char* cargaDeBloques = string_new();

	while(bloques[i] != NULL){
		string_append(&cargaDeBloques,bloques[i]);
		string_append(&cargaDeBloques,",");
		i++;
	}
	string_append(&cargaDeBloques,string_itoa(numeroDeBloque));
	bitarray_set_bit(bitmap,numeroDeBloque -1);
	char* bloquesActualizados = string_from_format("[%s]",cargaDeBloques);

	config_set_value(pokemon,"BLOCKS",bloquesActualizados);
	config_save(pokemon);

	config_destroy(pokemon);
	free(cargaDeBloques);
	limpiarPunteroAPuntero(bloques);
}
// Agrega la cantidad a size. Si es negativo el resta
void editarTamanioPokemon(char* nombrePokemon,int cantidad){
	t_config* pokemon = obtener_metadata_de_pokemon(nombrePokemon);
	int tamanioViejo = config_get_int_value(pokemon,"SIZE");
	tamanioViejo += cantidad;
	config_set_value(pokemon,"SIZE",string_itoa(tamanioViejo));
	config_save(pokemon);

	config_destroy(pokemon);
}


// Recibe un fd, el contenido que va a ser agregado, su longitud y el nombre del pokemon
// Escribe en el ultimo bloque y su última linea Funcion auxiliar de agregarNuevaPosicion
void escribirEnArchivo(FILE *bloque, char* contenidoAagregar,int caracteresAEscribir,char* nombrePokemon){

	fseek(bloque,0,SEEK_END);
	int tamanioDeArchivo = ftell(bloque);
	int cantidadQueSePuedeEscribir = TAMANIO_DE_BLOQUE - tamanioDeArchivo;
	int nuevaCantidad = caracteresAEscribir;
	printf("Tamaño del archivo: %d\nCantidad que se puede escribir: %d\n",tamanioDeArchivo,cantidadQueSePuedeEscribir);
	printf("Apunto de escribir \n");

	printf("La cantidad que quiero escribir: %d\n",caracteresAEscribir);
	if(caracteresAEscribir == 0) return;

	if(tamanioDeArchivo <= TAMANIO_DE_BLOQUE){

		printf("Escribiendo y mi tamaño es menor al bloque\n");
			if(caracteresAEscribir <= cantidadQueSePuedeEscribir){
				fwrite(contenidoAagregar,caracteresAEscribir,1,bloque);
				editarTamanioPokemon(nombrePokemon,caracteresAEscribir);
			}	else {
			fwrite(contenidoAagregar,cantidadQueSePuedeEscribir,1,bloque);
			editarTamanioPokemon(nombrePokemon,cantidadQueSePuedeEscribir);
			}
		nuevaCantidad = caracteresAEscribir - cantidadQueSePuedeEscribir;
		}

	printf("Cantidad que me queda por escribir: %d\n",nuevaCantidad);
	if(nuevaCantidad > 0){
		printf("Solicito nuevo bloque para escribir \n");
		FILE* nuevoBloque;
		char* rutaNuevoBloque = string_new();
		int numeroDeBloque = solicitarBloqueVacio();
		string_append(&rutaNuevoBloque,RUTA_DE_BLOQUES);
		string_append(&rutaNuevoBloque,"/");
		string_append(&rutaNuevoBloque,string_itoa(numeroDeBloque));
		string_append(&rutaNuevoBloque,".bin");
		nuevoBloque = fopen(rutaNuevoBloque,"a");

		char* string_nuevo = string_substring_from(contenidoAagregar,cantidadQueSePuedeEscribir);
		//printf("Nuevo string %s\n",string_nuevo);
		escribirEnArchivo(nuevoBloque,string_nuevo,strlen(string_nuevo),nombrePokemon);

		printf("Numero de bloque: %d\n",numeroDeBloque);

		//t_config* pokemon = obtener_metadata_de_pokemon(nombrePokemon);
		agregarBloqueAPokemon(nombrePokemon,numeroDeBloque);

		free(string_nuevo);
		free(rutaNuevoBloque);
		fclose(nuevoBloque);
	}

}

// Se agrega una posicion al final del archivo, es decir en el ultimo bloque
void agregarNuevaPosicion(char* contenidoAagregar,char* bloques,char* nombrePokemon){
	char** bloquesSeparados = string_get_string_as_array(bloques);


	int ultimaPosicion = length_punteroAPuntero(bloquesSeparados);
	printf("Ultimaposicion: %d\n",ultimaPosicion);
	if(ultimaPosicion == 0){
				int nuevoBloque = solicitarBloqueVacio();
				if(nuevoBloque == -1){
					log_error(logger,"No hay espacio en el disco");
					return;
				}
				printf("Voy a agregar el nuevo bloque\n");
				agregarBloqueAPokemon(nombrePokemon,nuevoBloque);
				bloquesSeparados = string_get_string_as_array(obtenerArrayDebloques(nombrePokemon));
				ultimaPosicion = length_punteroAPuntero(bloquesSeparados);
	}

	char* ultimoBloque = bloquesSeparados[ultimaPosicion-1];
	printf("Ultima posicion: %d\n",ultimaPosicion);
	printf("El ultimo bloque es: %s \n",ultimoBloque);
	char* direccionDeBloque = string_new();
	string_append(&direccionDeBloque,RUTA_DE_BLOQUES);
	string_append(&direccionDeBloque,"/");
	string_append(&direccionDeBloque,ultimoBloque);
	string_append(&direccionDeBloque,".bin");

	FILE* bloque = fopen(direccionDeBloque,"a");
	int cantidadDeCaracteres = strlen(contenidoAagregar);

	escribirEnArchivo(bloque,contenidoAagregar,cantidadDeCaracteres,nombrePokemon);
	fclose(bloque);
	printf("El ultimo bloque es: %s \n",ultimoBloque);
	limpiarPunteroAPuntero(bloquesSeparados);

}

// Funcion en revision. Sirve pero esta al tanto de modificaciones
// Problema: Escribe a la marcha los bloques. Por lo tanto si no alcanza los blqoues hay inconsistencia de datos
// Recibe un arreglo de los nombres de los bloques, el desplazamiento en el que empezó a diferir la posicion antigua con la actual
// , el nuevo string de posiciones y el nombre de pokemon
//
//	Escribe en los bloques y si se necesitan más bytes para una escritura porque el arrayy nuevo creció pide bloques y los añade
//	al pokemon.
void actualizacionDeBloques(char** bloques,int desplazamientoDeCambio,char* posiciones,char* pokemon){
	int posicionBloque = bloqueAfectado(desplazamientoDeCambio);
	char* bloqueAfectado = bloques[posicionBloque];
	int desplazamiento = desplazamientoEnBloque(desplazamientoDeCambio);

	char* stringParaEscribir = string_substring_from(posiciones,desplazamientoDeCambio);


	/////	Deberia crear una funcion para verificar si existe la cantidad necesaria de bloques antes
	/// de empezar a escribir para que no haya inconsistencia de datos

	int bytesEscritos = escribirBloquePosicionandoPuntero(bloqueAfectado,stringParaEscribir,desplazamiento);

	stringParaEscribir = string_substring_from(stringParaEscribir,bytesEscritos);

	posicionBloque++;

	while(bloques[posicionBloque] != NULL){
		bytesEscritos = escribirBloquePosicionandoPuntero(bloques[posicionBloque],stringParaEscribir,0);

		stringParaEscribir = string_substring_from(stringParaEscribir,bytesEscritos);

		posicionBloque++;
	}

	if(strlen(stringParaEscribir) != 0){
		printf("Se necesitan bloques \n");
		while(strlen(stringParaEscribir)){

			int nuevoBloque = solicitarBloqueVacio();
			printf("Voy a agregar el nuevo bloque\n");
			agregarBloqueAPokemon(pokemon,nuevoBloque);

			bytesEscritos = escribirBloquePosicionandoPuntero(string_itoa(nuevoBloque),stringParaEscribir,0);

			stringParaEscribir = string_substring_from(stringParaEscribir,bytesEscritos);
		}
		printf("Bloques asignados \n");

	}else printf("Todo escrito\n");
}

// Funcion que se encarga de añadir la cantidad al pokemon
//	Recibe el string que contiene todas las posiciones(todo el contenido del archivo), la posicion que se busca modificar
//	la cantidad que se desea sumar en esa posicion, el string que contiene los bloques y un nombre pokemon
int anadirCantidad(char* posiciones,char* posicionBuscada,int cantidadASumar,char* bloquesString,char* pokemon){

	char** bloques = string_get_string_as_array(bloquesString);
	// Separo cada linea por \n
	char** posicionesSeparadas = string_split(posiciones,"\n");

	// Obtengo el numero de linea que contiene esa posicon
	int posContenida = obtenerLinea(posicionesSeparadas,posicionBuscada);


	// sumo la cantidad con esta funcion. Me retorna la linea actualizada.
	char* nuevaLineaPokemon = sumarCantidadPokemon(posicionesSeparadas[posContenida],cantidadASumar);
	printf("La nueva linea pokemon a  agregar es: %s\n",nuevaLineaPokemon);

// Verifico si me ocupa lo mismo. Si ocupa lo mismo entonces no hace falta reescribir todos. SIno solo cambiar en el byte
	// que hubo el cambio
	if(strlen(posicionesSeparadas[posContenida]) == strlen(nuevaLineaPokemon)){

		int primeraPosicionCantPoke = desplazamientoDelArrayHastaCantPokemon(posicionesSeparadas,posicionBuscada);
		int posicionBloque = bloqueAfectado(primeraPosicionCantPoke);
		char* bloqueAfectado = bloques[posicionBloque];
		sobreescribirUnCaracter(bloqueAfectado,desplazamientoEnBloque(primeraPosicionCantPoke),string_itoa(obtenerCantidad(nuevaLineaPokemon)));

		return 0;
	}

	// Sino debo reescribir a partir del bloque necesitado.
	printf("Hay que aumentar bytes\n");

	char* posicionActualizada = actualizarPosicion(posicionesSeparadas,nuevaLineaPokemon,posContenida);

	int primeraPosicionCantPoke = desplazamientoDelArrayHastaCantPokemon(posicionesSeparadas,posicionBuscada);
	int posicionBloque = bloqueAfectado(primeraPosicionCantPoke);

	printf("Preparando archivo. posicion actualizada:\n%s\n",posicionActualizada);
	actualizacionDeBloques(bloques,primeraPosicionCantPoke,posicionActualizada,pokemon);
	return 0;
}

void procedimientoNEW(uint32_t idMensaje,char* pokemon,uint32_t posx,uint32_t posy,uint32_t cantidad){

	if(!existePokemon(pokemon)){
		crearPokemon(pokemon);

	}else log_info(logger,"Existe el pokemon");

	if(archivoAbierto(pokemon)){
		log_error(logger,"Archivo no se puede abrir");

		// MATAR HILO
		// REINTENTAR EN X TIEMPO

		struct arg_estructura argumentos;
		argumentos.nombrePokemon = pokemon;
		argumentos.tiempo = tiempo_de_reintento_operacion;

		pthread_t hiloDelReintento;
		if(!pthread_create(&hiloDelReintento,NULL,&reintentoAbrir,(void*)&argumentos)) printf("Creadu \n");

			pthread_join(hiloDelReintento,NULL);
	}
	log_info(logger,"Se puede abrir el archivo");
	abrirArchivo(pokemon);

	char* bloques = obtenerArrayDebloques(pokemon);

	char* posiciones = obtenerContenidoDeArchivos(bloques);

	char* posicionBuscada = string_from_format("%s-%s",string_itoa(posx),string_itoa(posy));


	if(!contieneEstaPosicion(posiciones,posicionBuscada)){
		// agregar al final del archivos
		log_error(logger,"No contengo la posicion");

		char* posicionYCantidad = string_from_format("%s=%s\n",posicionBuscada,string_itoa(cantidad));

		agregarNuevaPosicion(posicionYCantidad,bloques,pokemon);


	}else {
		log_info(logger,"Contengo esa posicion");

		int resultando = anadirCantidad(posiciones,posicionBuscada,cantidad,bloques,pokemon);

	}
	// Agregar sleep
	// CERRAR ARCHIVO

	cerrarArchivo(pokemon);
	log_info(logger,"Archivo cerrado");
	free(posicionBuscada);
	free(posiciones);
	free(bloques);
}

void procedimientoCATCH(uint32_t idMensaje,char* pokemon,uint32_t posx,uint32_t posy,uint32_t cantidad){

	if(!existePokemon(pokemon)){
		crearPokemon(pokemon);
		return;
	}else log_info(logger,"Existe el pokemon");




	if(archivoAbierto(pokemon)){
		log_error(logger,"Archivo no se puede abrir");

		// MATAR HILO
		// REINTENTAR EN X TIEMPO

		struct arg_estructura argumentos;
		argumentos.nombrePokemon = pokemon;
		argumentos.tiempo = tiempo_de_reintento_operacion;

		pthread_t hiloDelReintento;
		if(!pthread_create(&hiloDelReintento,NULL,&reintentoAbrir,(void*)&argumentos)) printf("Creadu \n");

			pthread_join(hiloDelReintento,NULL);
	}
	log_info(logger,"Se puede abrir el archivo");
	abrirArchivo(pokemon);
}

void procedimientoGET(uint32_t idMensaje,char* pokemon){

	log_info(logger,"comienza a ejecutar GET");
	if(!existePokemon(pokemon)){
		crearPokemon(pokemon);
		}else log_info(logger,"Existe el pokemon");


	if(archivoAbierto(pokemon)){
		log_error(logger,"Archivo no se puede abrir");

		// MATAR HILO
		// REINTENTAR EN X TIEMPO

		struct arg_estructura argumentos;
		argumentos.nombrePokemon = pokemon;
		argumentos.tiempo = tiempo_de_reintento_operacion;

		pthread_t hiloDelReintento;
		if(!pthread_create(&hiloDelReintento,NULL,&reintentoAbrir,(void*)&argumentos)) printf("Creadu \n");

			pthread_join(hiloDelReintento,NULL);
		}
		log_info(logger,"Se puede abrir el archivo");
		abrirArchivo(pokemon);

		char* bloques = obtenerArrayDebloques(pokemon);

		char* arrayDeArchivo = obtenerContenidoDeArchivos(bloques);
		char** posicionesConCantidad = string_split(arrayDeArchivo,"\n");

		int longitud;

		for(int i=0;i<length_punteroAPuntero(posicionesConCantidad);i++){
			longitud = longitud + strlen(posicionesConCantidad[i])+1;

		}


		char** posicionesSeparadas = malloc(longitud);
		int j=0;
		while(posicionesConCantidad[j] != NULL){

			posicionesSeparadas[j] = getToken(posicionesConCantidad[j],'=');
			j++;
		}




		uint32_t cantidadParesCoordenadas = length_punteroAPuntero(posicionesConCantidad);
		log_info(logger,"La cantidad de pares de coordenadas es:%d",cantidadParesCoordenadas);

		t_list* coordenadasSeparadas = guardarCoordenadas(posicionesSeparadas);
		uint32_t coordenadasAEnviar[cantidadParesCoordenadas*2];
		insertarCoordenadas(coordenadasSeparadas,coordenadasAEnviar);
		imprimirContenido(coordenadasAEnviar,cantidadParesCoordenadas*2);

		uint32_t tamanioCoordenadasAEnviar = sizeof(uint32_t)*cantidadParesCoordenadas*2;
		log_info(logger,"tamanioCoordenadasAEnviar:%d",tamanioCoordenadasAEnviar);
		//envioDeMensajeLocalize(pokemon,idMensaje,cantidadParesCoordenadas,coordenadasAEnviar);

		    cerrarArchivo(pokemon);
			log_info(logger,"Archivo cerrado");
			free(arrayDeArchivo);
			free(bloques);

}

char *getToken(char *unString,char delimitador){
		    int length = strlen(unString) + 1;
		    char *stringARetornar = malloc(length);
		    int i =0;
		    while(unString[i] != delimitador){
		      stringARetornar[i] = unString[i];
		      i++;
		    }
		    stringARetornar[i] = '\0';
		    return stringARetornar;
		}

char *getCoordenadaX(char *coordenada){
    int i = 0;
    while(coordenada[i] != '-'){
            i++;
    }
    char *coordenadaX = malloc(i);

    for(int j = 0; j <= i; j++){
        coordenadaX[j] = coordenada[j];
    }
    coordenadaX[i] = '\0';
    return coordenadaX;
}

char *getCoordenadaY(char *coordenada){

    int i = 0;
    while(coordenada[i] != '-'){
            i++;
    }

    int j = 0;
    int k = i+1;

    for(int l = k; l < strlen(coordenada);l++){
            j++;
    }

    char *coordenadaY = malloc(j);

    j = 0;

    for(int g = k; g < strlen(coordenada);g++){
        coordenadaY[j] = coordenada[g];
        j++;
    }

    coordenadaY[j] = '\0';

    return coordenadaY;
}



t_list* guardarCoordenadas(char** posiciones){
	t_list* coordenadasx = list_create();
	for(int i = 0; i < length_punteroAPuntero(posiciones);i++){
	    list_add(coordenadasx,getCoordenadaX(posiciones[i]));



	}
	t_list* coordenadasy = list_create();

	for(int j = 0; j < length_punteroAPuntero(posiciones);j++){
		    list_add(coordenadasy,getCoordenadaY(posiciones[j]));



		}

	t_list* coordenadas = list_create();
	for(int k=0;k<list_size(coordenadasx);k++){
		char *coordx = list_get(coordenadasx,k);
		list_add(coordenadas,coordx);
		char *coordy = list_get(coordenadasy,k);
		list_add(coordenadas,coordy);

	}
	list_destroy(coordenadasx);
	list_destroy(coordenadasy);
	return coordenadas;


}

void mostrarCoordenadas(t_list* lista){
	for(int i=0;i<list_size(lista);i++){
		char* coordenada = list_get(lista,i);
		log_info(logger,"coordenada:%s",coordenada);
	}

}

void insertarCoordenadas(t_list *lista,uint32_t* vector){
    for(int i = 0; i < list_size(lista);i++){
        char *elem = list_get(lista,i);
        uint32_t elemento = atoi(elem);
        vector[i] = elemento;
    }
}

void imprimirContenido(uint32_t *vector,uint32_t size){
		    for(int i = 0; i < size;i++){
		        printf("posicion%d: %d \n",i,vector[i]);
		    }

		}
