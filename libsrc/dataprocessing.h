#ifndef DATAPROCESSING_H
#define DATAPROCESSING_H

#include "earlybirdlib.h" /* Required for the STATION structure */

/* Applies a Butterworth IIR filter to an array of data.
 * type: 1 = High-Pass, 2 = Low-Pass
 * order: 2 or 4 (Forced to even values)
 */
void aplicar_filtro_iir(long *data, long size, double fs, int type, double fc, int order);

/* Trims the trailing zeros (future unrecorded data) from the buffer */
void FindDataEndHypoLocal(STATION *pSta);

/* Fills the network gaps (zeros or INT_MAX) with linear interpolation */
void FillInternalGaps(STATION *pSta);

/* Removes the DC offset (baseline) from the trace to center it on zero */
void DemeanTrace(STATION *pSta);

/* Converts the dropouts (exact zero runs >= ~0.5 s) to INT_MAX so that
 * the gaps recorded to disk are drawn as empty space and not as a flat
 * zero line. */
void MarkZeroDropouts(STATION *pSta);

#endif /* DATAPROCESSING_H */
