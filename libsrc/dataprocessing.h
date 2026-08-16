#ifndef DATAPROCESSING_H
#define DATAPROCESSING_H

#include "earlybirdlib.h" /* Necesario para la estructura STATION */

/* Aplica un filtro Butterworth IIR a un arreglo de datos.
 * type: 1 = High-Pass, 2 = Low-Pass
 * order: 2 o 4 (Forzado a pares)
 */
void aplicar_filtro_iir(long *data, long size, double fs, int type, double fc, int order);

/* Recorta los ceros finales (datos futuros no grabados) del buffer */
void FindDataEndHypoLocal(STATION *pSta);

/* Rellena los gaps de la red (ceros o INT_MAX) con interpolación lineal */
void FillInternalGaps(STATION *pSta);

/* Remueve el offset DC (línea base) de la traza para centrarla en cero */
void DemeanTrace(STATION *pSta);

/* Convierte a INT_MAX los dropouts (runs de ceros exactos >= ~0.5 s) para que
 * los huecos grabados en disco se dibujen como espacio vacío y no como línea
 * plana en cero. */
void MarkZeroDropouts(STATION *pSta);

#endif /* DATAPROCESSING_H */
