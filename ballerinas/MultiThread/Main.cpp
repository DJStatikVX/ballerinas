/*
 *  Main.cpp
 * 
 *  Autores: Juan Carlos Gómez Conde       (UO237097)
 *           Samuel Rodríguez Ares         (UO271612)
 *           Eloy Afredo Schmidt Rodríguez (UO271588)
 */

// Importación de librerías
#include <CImg.h>
#include <math.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

using namespace cimg_library;

// Declaración de procedimientos para el cambio de contraste
void *TaskContrast(void *args);
void ChangeContrast(struct parameters *arg);

// Declaración de procedimientos para el efecto sepia
void *TaskSepia(void *args);
void SepiaEffect(struct parameters *arg);

// Estructura de parámetros para los hilos
struct parameters
{
	int start;	// punto de inicio
	int size;   // cantidad de píxeles a tratar

	// Punteros a los arrays RGB de la imagen original
	float *pRcomp,*pGcomp, *pBcomp;

	// Punteros a los arrays RGB de la imagen contrastada
	float *pRcont,*pGcont, *pBcont;

	// Punteros a los arrays RGB de la imagen con efecto sepia
	float *pRsepia,*pGsepia,*pBsepia;

	// Variable calculada a partir del % de contraste
	float C;
};

/*******************************************************
 * Método main() con ejecución de algoritmo
 * y medidas de respectivos tiempos
 * 
*/
int main() {
	/***************************************************
	 *
	 * Inicialización de variables
	 * y preparación de todos los elementos necesarios
	 * antes de comenzar la medición de tiempos
	 *
	 */
	CImg<float> srcImage("bailarina.bmp"); // carga la imagen original

	const int NUM_THREADS = 8;			   // número de hilos
	float T = 30;	                       // % de contraste a aplicar
	
	parameters args[NUM_THREADS];          // declaración del array de parámetros para cada hilo

	// srcImage.display();                 // si es necesario, se muestra la imagen original

	int width = srcImage.width(), height = srcImage.height(); // anchura y altura de la imagen

	float *pdstImage; // puntero a los píxeles de la imagen con cambio de contraste
	float *pdstSepia; // puntero a los píxeles de la imagen con efecto sepia

	int nComp = srcImage.spectrum(); // número de componentes de la imagen original

	pdstImage = (float *) malloc(width * height * nComp * sizeof(float));	// asignación de memoria

	// Control de errores al solicitar memoria
	if (pdstImage == NULL) {
        printf("\nMemory allocating error\n");
        exit(-2);
    }

	pdstSepia = (float *) malloc(width * height * nComp * sizeof(float));	// asignación de memoria

	// Control de errores al solicitar memoria
	if (pdstSepia == NULL) {
        printf("\nMemory allocating error\n");
        exit(-2);
    }

	pthread_t thread[NUM_THREADS];	 // declaración del array de hilos
	
	int nVeces = 30;		// número de veces a ejecutar el algoritmo
	double dElapsedTimeS;	// almacenará la diferencia de tiempo detallada
	timespec tStart, tEnd;	// estructuras para almacenar el tiempo inicial y final

	// Inicialización de atributos para cada conjunto de parámetros
	for (int a = 0; a < NUM_THREADS; a++){
		args[a].pRcomp = srcImage.data();
		args[a].pGcomp = args[a].pRcomp + (height * width);
		args[a].pBcomp = args[a].pGcomp + (height * width);

		args[a].start = 0; 							   // se modificará posteriormente
		args[a].size = (width * height) / NUM_THREADS; // mismo tamaño para cada hilo
		
		args[a].pRcont = pdstImage;
		args[a].pGcont = args[a].pRcont + (height * width);
		args[a].pBcont = args[a].pGcont + (height * width);

		while (abs(T) > 100) {	// T se comprende entre -100% y 100%
			T = T / 10;
		}

		args[a].C = pow((100 + T) / 100, 2); // cálculo del factor de cambio de contraste

		args[a].pRsepia = pdstSepia;
		args[a].pGsepia = args[a].pRsepia + (height * width);
		args[a].pBsepia = args[a].pGsepia + (height * width);
	}

	/*********************************************
	 * Inicio del algoritmo
	 *
	 * Comienza la medición de tiempo
	 *
	 */

	printf("Running task    : \n");

	// Se almecena el tiempo tomado en &tStart. En caso de fallo, se muestra por pantalla
	if (clock_gettime(CLOCK_REALTIME, &tStart) == -1)
	{
		printf("ERROR: clock_gettime: %d.\n", errno);
		exit(EXIT_FAILURE);
	}

	// El algoritmo se aplicará nVeces para obtener medidas significativas
	for (int n = 0; n < nVeces; n++){

		/************************************************
	 	* Algoritmo 1: Cambio de contraste 
		*/

		// Creación de los hilos
		for (int i = 0; i < NUM_THREADS; i++)
		{
			int start = args[i].size * i;	// se calcula el punto de inicio de cada hilo
			args[i].start = start;

			// Se crea cada hilo especificando la tarea y parámetros a utilizar
			if (pthread_create(&thread[i], NULL, TaskContrast, &args[i]) != 0)
			{
				fprintf(stderr, "ERROR: Creating thread %d\n", i);
				return EXIT_FAILURE;
			}
			
		}

	    // Ejecución de todos los hilos
		for (int i = 0; i < NUM_THREADS; i++)
		{
			pthread_join(thread[i], NULL);
		}

		/************************************************
	 	* Algoritmo 2: Efecto sepia 
		*/

		// Creación de los hilos
		for (int i = 0; i < NUM_THREADS; i++)
		{
			int start = args[i].size * i;	// se calcula el punto de inicio de cada hilo
			args[i].start = start;

			// Se crea cada hilo especificando la tarea y parámetros a utilizar
			if (pthread_create(&thread[i], NULL, TaskSepia, &args[i]) != 0)
			{
				fprintf(stderr, "ERROR: Creating thread %d\n", i);
				return EXIT_FAILURE;
			}
		}

	    // Ejecución de todos los hilos
		for (int i = 0; i < NUM_THREADS; i++)
		{
			pthread_join(thread[i], NULL);
		}
	}

	/***********************************************
	 * Fin del algoritmo
	 *
	 * Se toma de nuevo el tiempo
	 * y se calcula el tiempo transcurrido en la ejecución
	 *
	 */
	
	// Se mide el tiempo final
	if (clock_gettime(CLOCK_REALTIME, &tEnd) == -1)
	{
		printf("ERROR: clock_gettime: %d.\n", errno);
		exit(EXIT_FAILURE);
	}

	// printf("Finished\n");	

	// Tras finalizar, establece la diferencia de tiempo
	dElapsedTimeS = (tEnd.tv_sec - tStart.tv_sec);				// establece los segundos de diferencia
	dElapsedTimeS += (tEnd.tv_nsec - tStart.tv_nsec) / 1e+9;	// añade los nanosegundos a la diferencia
	printf("Elapsed time    : %f s.\n", dElapsedTimeS);			// muestra el resultado
	
	/******************************************************
	 * Se almacena el resultado 
	 * del algoritmo en el disco
	 *
	 */
	
	// Declaración de las imágenes destino a partir de sus respectivos punteros y propiedades
	CImg<float> dstImage(pdstImage, width, height, 1, nComp);
	CImg<float> dstSepia(pdstSepia, width, height, 1, nComp);

	// Almacena las imágenes destino en el disco
	dstImage.save("bailarina2.bmp");
	dstSepia.save("bailarina3.bmp");
	
	return EXIT_SUCCESS;	// operación realizada con éxito
}

// Procedimiento a realizar por los hilos para el cambio de contraste
void *TaskContrast(void *arg) {

	// Se convierte el parámetro en un puntero a una estructura de parámetros
	struct parameters params = *((struct parameters *) arg);

	// Se llama al método de cambio de contraste utilizando esa estructura de parámetros como argumento
	ChangeContrast(&params);

	return NULL;
}

// Procedimiento que modifica las componentes RGB para el cambio de contraste
void ChangeContrast(struct parameters *arg){
	float C = arg -> C;

	if (C != 1) {	// el valor T = 0 no hace nada

		for (int i = arg -> start; i < arg -> size + arg -> start; i++) {

			// Se aplica la fórmula al contenido de los punteros R
			*(arg -> pRcont + i) = ((*(arg -> pRcomp + i)) * C);

			// Se comprueba si la componente R sobresatura
			if (*(arg -> pRcont + i) > 255) {
				(*(arg -> pRcont + i) = 255);
			}
			
			// Se aplica la fórmula al contenido de los punteros G
			*(arg -> pGcont + i) = ((*(arg -> pGcomp + i)) * C);

			// Se comprueba si la componente G sobresatura
			if (*(arg -> pGcont + i) > 255) {
				(*(arg -> pGcont + i) = 255);
			}
			
			// Se aplica la fórmula al contenido de los punteros B
			*(arg -> pBcont + i) = ((*(arg -> pBcomp + i)) * C);

			// Se comprueba si la componente B sobresatura
			if (*(arg -> pBcont + i) > 255) {
				(*(arg -> pBcont + i) = 255);
			}
		}
	}
}

// Procedimiento a realizar por los hilos para el efecto sepia
void *TaskSepia(void *arg) {

	// Se convierte el parámetro en un puntero a una estructura de parámetros
	struct parameters params = *((struct parameters *) arg);

	// Se llama al método de cambio de contraste utilizando esa estructura de parámetros como argumento
	SepiaEffect(&params);

	return NULL;
}

// Procedimiento que modifica las componentes RGB para el efecto sepia
void SepiaEffect(struct parameters *arg) {
	for (int i = arg -> start; i < (arg -> size + arg -> start); i++) {

		// Se aplica la fórmula al contenido de los punteros RGB

		*(arg -> pRsepia + i) = ((*(arg -> pRcont + i)) * 0.393) + ((*(arg -> pGcont + i)) * 0.769) + ((*(arg -> pBcont + i)) * 0.189);
		*(arg -> pGsepia + i) = ((*(arg -> pRcont + i)) * 0.349) + ((*(arg -> pGcont + i)) * 0.686) + ((*(arg -> pBcont + i)) * 0.168);
		*(arg -> pBsepia + i) = ((*(arg -> pRcont + i)) * 0.272) + ((*(arg -> pGcont + i)) * 0.534) + ((*(arg -> pBcont + i)) * 0.131);

		// Se comprueba si alguna de las componentes sobresatura

		if (*(arg -> pRsepia + i) > 255) {
			*(arg -> pRsepia + i) = 255;	// componente R
		}

		if (*(arg -> pGsepia + i) > 255) {
			*(arg -> pGsepia + i) = 255;	// componente G
		}

		if (*(arg -> pBsepia + i) > 255) {
			*(arg -> pBsepia + i) = 255;	// componente B
		}
	}
}