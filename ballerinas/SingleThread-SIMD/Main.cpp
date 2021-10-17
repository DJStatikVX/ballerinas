/*
 *  Main.cpp
 *  Autores: Juan Carlos Gómez Conde        (UO237097)
 *           Samuel Rodríguez Ares          (UO271612)
 *           Eloy Alfredo Schmidt Rodríguez (UO271588)
 */

// Importación de librerías
#include <errno.h>
#include <CImg.h>
#include <immintrin.h> // necesaria para utilizar las funciones intrínsecas
#include <malloc.h>
#include <stdio.h>

using namespace cimg_library;

// Número de elementos por paquete
#define ELEMENTSPERPACKET (sizeof(__m256) / sizeof(float));

/************************************
 * Método main con inicialización
 * de variables, ejecución de algoritmos
 * y medición de tiempos
 */
int main() {
    
    /*************************************************
     * Inicialización de variables
     * y preparación de todos los elementos necesarios
     * antes de comenzar la medición de tiempos
     */
	CImg<float> srcImage("bailarina.bmp"); // carga la imagen original
    
	int showImage = 0;          // a 0 si no se quieren mostrar las imágenes en ejecución
    
    if (showImage) {
		srcImage.display();     // si es necesario, se muestra la imagen original
    }
    
    int width, height, size;    // anchura y altura de la imagen
	int nComp;                  // número de componentes de la imagen original

    // Se asigna el tamaño y número de componentes en base a la información de srcImage
	width = srcImage.width();
	height  = srcImage.height();
	nComp = srcImage.spectrum();

    size = width * height;      // tamaño = anchura * altura
 
    int range = size / ELEMENTSPERPACKET;   // rango a recorrer al aplicar algoritmos

    // Cálculo del número de paquetes necesarios para aplicar cada algoritmo a la imagen completa
    int numParts = (size * nComp * sizeof(float) / sizeof(__m256));

    // Declaración de variables para medición de tiempos
    double dElapsedTimeS;
    timespec tStart, tEnd;

    int nVeces = 45;

    // Declaración de punteros RGB
	float *pRcomp, *pGcomp, *pBcomp;    // Pointers to the R, G and B components (original)
	float *pRcont, *pGcont, *pBcont;    // Pointers to the R, G and B components (contrast)
    float *pRsep, *pGsep, *pBsep;       // Pointers to the R, G and B components (sepia)

    // Se pide memoria para trabajar en la imagen a generar con cambio de contraste
    float *pdstImage = (float *)_mm_malloc(sizeof(__m256) * numParts, sizeof(__m256));  // Pointer to the contrast change image pixels

    // Control de errores al solicitar memoria
    if (pdstImage == NULL) {
        printf("\nMemory allocating error\n");
        exit(-2);
    }

    // Se pide memoria para trabajar en la imagen a generar con efecto sepia
    float *pdstImage1 = (float *)_mm_malloc(sizeof(__m256) * numParts, sizeof(__m256));  // Pointer to the sepia change image pixels

    // Control de errores al solicitar memoria
    if (pdstImage1 == NULL) {
        printf("\nMemory allocating error\n");
        exit(-2);
    }

	// Inicialización de punteros a los arrays RGB de la imagen original
	pRcomp = srcImage.data(); // R
	pGcomp = pRcomp + size;   // G
	pBcomp = pGcomp + size;   // B
	
	// Inicialización de punteros a los arrays RGB de la imagen con cambio de contraste
	pRcont = pdstImage;       // R
	pGcont = pRcont + size;   // G
	pBcont = pGcont + size;   // B
    
    // Vectores __m256 para el cambio de contraste
    __m256 vR, vG, vB;                  // cargarán los punteros de la imagen original
    __m256 vC;                          // cargarán la constante de cambio de contraste

    __m256 max = _mm256_set1_ps(255);   // paquete con valores 255 para controlar la saturación

	// Inicialización de punteros a los arrays RGB de la imagen con efecto sepia
	pRsep = pdstImage1;       // R
	pGsep = pRsep + size;     // G
	pBsep = pGsep + size;     // B

    // Vectores __m256 para el efecto sepia
    __m256 vsR, vsG, vsB;       // cargarán los punteros de la imagen con cambio de contraste
    __m256 vcRR, vcRG, vcRB;
    __m256 vcGR, vcGG, vcGB;    // vectores con valores para aplicar la fórmula del efecto sepia
    __m256 vcBR, vcBG, vcBB;
    
    // Comienza la medición de tiempos
    printf("Running task    : \n");

    // Si se produce algún error al medir, termina el programa
    if (clock_gettime(CLOCK_REALTIME, &tStart) == -1) {
        printf("ERROR: clock_gettime: %d.\n", errno);
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < nVeces ; i++){

        /*
        *	1)      Contraste
        *
        *	Aumenta o disminuye el contraste de una imagen. Algoritmo:
        *	T = porcentaje de variación del contraste [-100, 100]. El valor 0 no hace nada.
        *	C = ((100 + T) / 100)²
        *
        *	vRd = _mm256_mul_ps(vR, vC);
        *	vGd = _mm256_mul_ps(vG, vC);
        *	vBd = _mm256_mul_ps(vB, vC);
        */

        float T = 30;	// cantidad de contraste a aplicar

        if (T != 0) {	// el valor 0 no hace nada

            while(abs(T) > 100) {	// T se comprende entre -100 % y 100 %
                T = T / 10;
            }

            // Cálculo de la constante de cambio de contraste
            float C = pow((100 + T) / 100, 2);

            // Carga el vector con el valor de C
            vC  = _mm256_set1_ps(C);

            // Se aplica el algoritmo para todo el rango
            for (int j = 0; j < range; j++) {
                int i = j * ELEMENTSPERPACKET;  // desplazamiento

                // Se cargan las componentes RGB en los vectores __m256
                vR = _mm256_loadu_ps((pRcomp + i));
                vG = _mm256_loadu_ps((pGcomp + i));
                vB = _mm256_loadu_ps((pBcomp + i));

                // Se multiplican los vectores RGB por el vector vC, evitando que sobresature
                *(__m256 *) (pRcont + i) = _mm256_min_ps(max,_mm256_mul_ps(vR,vC));
                *(__m256 *) (pGcont + i) = _mm256_min_ps(max,_mm256_mul_ps(vG,vC));
                *(__m256 *) (pBcont + i) = _mm256_min_ps(max,_mm256_mul_ps(vB,vC));
            }
        }

        /************************************************
         *  2)      Convertir a sepia el resultado
         * 
         *	    Fórmula:
        *
        * 		R’ = 0,393 * R + 0,769 * G + 0,189 * B
        *		G’ = 0,349 * R + 0,686 * G + 0,168 * B
        *      B’ = 0,272 * R + 0,534 * G + 0,131 * B
        */ 

        // Inicialización de los vectores para las componentes

        // Vectores para la componente R
        vcRR = _mm256_set1_ps(0.393);
        vcRG = _mm256_set1_ps(0.769);
        vcRB = _mm256_set1_ps(0.189);

        // Vectores para la componente G
        vcGR = _mm256_set1_ps(0.349);
        vcGG = _mm256_set1_ps(0.686);
        vcGB = _mm256_set1_ps(0.168);

        // Vectores para la componente B
        vcBR = _mm256_set1_ps(0.272);
        vcBG = _mm256_set1_ps(0.534);
        vcBB = _mm256_set1_ps(0.131);

        // Se aplica el algoritmo para todo el rango
        for (int j = 0; j < range; j++) {
            int i = j * ELEMENTSPERPACKET;  // desplazamiento

            // Se cargan las componentes RGB en los vectores __m256
            vsR = _mm256_loadu_ps((pRcont + i));
            vsG = _mm256_loadu_ps((pGcont + i));
            vsB = _mm256_loadu_ps((pBcont + i));

            // Se aplica la fórmula, evitando que sobresature
            *(__m256 *) (pRsep + i) = _mm256_min_ps(max, _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(vsR, vcRR), _mm256_mul_ps(vsG, vcRG)), _mm256_mul_ps(vsB, vcRB)));
            *(__m256 *) (pGsep + i) = _mm256_min_ps(max, _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(vsR, vcGR), _mm256_mul_ps(vsG, vcGG)), _mm256_mul_ps(vsB, vcGB)));
            *(__m256 *) (pBsep + i) = _mm256_min_ps(max, _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(vsR, vcBR), _mm256_mul_ps(vsG, vcBG)), _mm256_mul_ps(vsB, vcBB)));
        }
    }

    // Medición de tiempo final. Si se produce algún error de medición, termina el programa
    if (clock_gettime(CLOCK_REALTIME, &tEnd) == -1) {
        printf("ERROR: clock_gettime: %d.\n", errno);
        exit(EXIT_FAILURE);
    }

    printf("Finished\n");

    // Se calcula el tiempo empleado a partir de los tiempos tomados
    dElapsedTimeS = (tEnd.tv_sec - tStart.tv_sec);
    dElapsedTimeS += (tEnd.tv_nsec - tStart.tv_nsec) / 1e+9;

    printf("Elapsed time    : %f s.\n", dElapsedTimeS);

    // Se genera la imagen con cambio de contraste
    CImg<float> dstImage(pdstImage, width, height, 1, nComp);

    // Se genera la imagen con cambio de contraste + efecto sepia
    CImg<float> dstImage1(pdstImage1, width, height, 1, nComp);
    
    // Se guardan las imágenes en disco
    dstImage.save("bailarina2.bmp");
    dstImage1.save("bailarina3.bmp");

    // Si está habilitado, se muestran los resultados finales
    if (showImage) {
        dstImage.display();
        dstImage1.display();
    }

    // Se libera la memoria solicitada
    _mm_free(pdstImage);
    _mm_free(pdstImage1);

    return (0); // finalizado con éxito
}