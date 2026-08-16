/******************************************************************
 * dataprocessing.c                                               *
 * *
 * Librería de procesamiento digital de señales (DSP) para        *
 * visualizadores y análisis sismológico interactivo.             *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include "earlybirdlib.h"
#include "dataprocessing.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* --------------------------------------------------------------------
 * Filtro Butterworth IIR Dinámico (Soporta Orden 2 y 4)
 * -------------------------------------------------------------------- */
void aplicar_filtro_iir(long *data, long size, double fs, int type, double fc, int order) {
    if (fs <= 0.0 || size == 0 || fc <= 0.0) return;
    
    double w0 = 2.0 * M_PI * fc / fs;
    double cosW = cos(w0);
    double sinW = sin(w0);
    
    int n_biquads = (order >= 4) ? 2 : 1;
    
    /* Factores Q para Butterworth */
    double Q[2] = {0.70710678, 0.0}; /* Orden 2 usa un Q unico de 1/sqrt(2) */
    if (n_biquads == 2) {
        /* Para orden 4 se requieren 2 biquads en cascada con estos Q especificos */
        Q[0] = 0.54119610;
        Q[1] = 1.30656296;
    }
    
    for (int b = 0; b < n_biquads; b++) {
        double alpha = sinW / (2.0 * Q[b]);
        double a0 = 1.0 + alpha;
        double b0_f, b1_f, b2_f, a1_f, a2_f;
        
        if (type == 1) { /* High-Pass */
            b0_f = ((1.0 + cosW) / 2.0) / a0;
            b1_f = -(1.0 + cosW) / a0;
            b2_f = ((1.0 + cosW) / 2.0) / a0;
        } else { /* Low-Pass */
            b0_f = ((1.0 - cosW) / 2.0) / a0;
            b1_f = (1.0 - cosW) / a0;
            b2_f = ((1.0 - cosW) / 2.0) / a0;
        }
        
        a1_f = (-2.0 * cosW) / a0;
        a2_f = (1.0 - alpha) / a0;
        
        /* Initialize states using the very first sample to avoid impulse response spikes */
        double x1 = (double)data[0], x2 = (double)data[0];
        double y1 = 0.0, y2 = 0.0;
        if (type == 1) { y1 = 0.0; y2 = 0.0; } /* Para HPF, el resultado DC es 0 */
        else { y1 = (double)data[0]; y2 = (double)data[0]; } /* Para LPF, la DC cruza intacta */
        
        for (long i = 0; i < size; i++) {
            /* Hueco (INT_MAX): reiniciar estado del filtro para que cada segmento
               continuo se procese por separado y el hueco quede intacto en la salida. */
            if (data[i] == INT_MAX) {
                x1 = x2 = 0.0; y1 = y2 = 0.0;
                continue;
            }
            double x0 = (double)data[i];
            double y0 = b0_f*x0 + b1_f*x1 + b2_f*x2 - a1_f*y1 - a2_f*y2;
            x2 = x1; x1 = x0;
            y2 = y1; y1 = y0;
            data[i] = (long)y0;
        }
    }
}

/* --------------------------------------------------------------------
 * FindDataEndHypoLocal: Trims trailing zeros caused by requesting future data
 * -------------------------------------------------------------------- */
void FindDataEndHypoLocal(STATION *pSta) {
    long lLastNonZero = -1;
    if (pSta->dEndTime > 0.1 && pSta->lRawCircCtr > 0) {
        for (long i = pSta->lRawCircCtr - 1; i >= 0; i--) {
            if (pSta->plRawCircBuff[i] != 0 && pSta->plRawCircBuff[i] != INT_MAX) {
                lLastNonZero = i;
                break;
            }
        }
        if (lLastNonZero == -1) {
            pSta->lRawCircCtr = 0; 
        } else if (lLastNonZero < pSta->lRawCircCtr - 1) {
            pSta->dEndTime -= ((double)(pSta->lRawCircCtr - 1 - lLastNonZero) / pSta->dSampRate);
            pSta->lRawCircCtr = lLastNonZero + 1;
        }
    }
}

/* --------------------------------------------------------------------
 * FillInternalGaps: Fully interpolates any 0 or INT_MAX gaps
 * -------------------------------------------------------------------- */
void FillInternalGaps(STATION *pSta) {
    if (pSta->lRawCircCtr <= 0) return;

    long last_valid = 0;
    long gap_start = -1;
    int found_first = 0;

    for (long i = 0; i < pSta->lRawCircCtr; i++) {
        if (pSta->plRawCircBuff[i] != 0 && pSta->plRawCircBuff[i] != INT_MAX) {
            last_valid = pSta->plRawCircBuff[i];
            found_first = 1;
            break;
        }
    }
    
    if (!found_first) return;

    for (long i = 0; i < pSta->lRawCircCtr; i++) {
        if (pSta->plRawCircBuff[i] != 0 && pSta->plRawCircBuff[i] != INT_MAX) break;
        pSta->plRawCircBuff[i] = last_valid; 
    }

    for (long i = 0; i < pSta->lRawCircCtr; i++) {
        long val = pSta->plRawCircBuff[i];
        int is_gap = (val == 0 || val == INT_MAX);
        
        if (is_gap) {
            if (gap_start == -1) gap_start = i;
        } else {
            if (gap_start != -1) {
                long gap_len = i - gap_start;
                long next_valid = val;
                
                for (long g = gap_start; g < i; g++) {
                    double frac = (double)(g - gap_start + 1) / (double)(gap_len + 1);
                    pSta->plRawCircBuff[g] = last_valid + (long)(frac * (next_valid - last_valid));
                }
                gap_start = -1;
            }
            last_valid = val;
        }
    }
    
    if (gap_start != -1) {
        for (long g = gap_start; g < pSta->lRawCircCtr; g++) {
            pSta->plRawCircBuff[g] = last_valid;
        }
    }
}

/* --------------------------------------------------------------------
 * MarkZeroDropouts: Limpia los dropouts de ceros grabados en disco (el
 * escritor rellena con 0 los huecos).
 *  - Runs de ceros >= 0.5 s (huecos reales) -> INT_MAX (se dibujan como
 *    espacio vacio; el filtro reinicia su estado).
 *  - Runs cortos de ceros (< 0.5 s, incl. ceros aislados): corresponden a
 *    muestras individuales perdidas del feed. Si estan inmersos en datos
 *    validos se interpolan linealmente entre los vecinos (evita el spike
 *    que un 0 real provocaria en el render y en el filtrado IIR). Si tocan
 *    el inicio/fin del buffer o estan pegados a un hueco (INT_MAX), se
 *    absorben como INT_MAX para no fabricar datos en los bordes de las
 *    ventanas vacias.
 * -------------------------------------------------------------------- */
void MarkZeroDropouts(STATION *pSta) {
    long n = pSta->lRawCircCtr;
    if (n <= 0 || pSta->dSampRate <= 0.0) return;

    long thr = (long)(0.5 * pSta->dSampRate);
    if (thr < 10) thr = 10;

    /* 1a pasada: runs largos de ceros -> INT_MAX (hueco) */
    long run = 0;
    for (long i = 0; i < n; i++) {
        if (pSta->plRawCircBuff[i] == 0) {
            run++;
        } else {
            if (run >= thr) {
                for (long j = i - run; j < i; j++)
                    pSta->plRawCircBuff[j] = INT_MAX;
            }
            run = 0;
        }
    }
    if (run >= thr) {
        for (long j = n - run; j < n; j++)
            pSta->plRawCircBuff[j] = INT_MAX;
    }

    /* 2a pasada: runs cortos de ceros. Interpolar si estan flanqueados por
       datos validos; si no (borde del buffer o contiguos a INT_MAX), hueco. */
    long i = 0;
    while (i < n) {
        if (pSta->plRawCircBuff[i] != 0) { i++; continue; }
        long s = i;
        while (i < n && pSta->plRawCircBuff[i] == 0) i++;
        long e = i;
        long len = e - s;
        if (len >= thr) continue;      /* ya convertido a INT_MAX arriba */

        /* Vecinos validos: los datos son bipolares, asi que se marcan con
           banderas (un centinela -1 chocaria con muestras negativas reales). */
        int has_left  = (s > 0 && pSta->plRawCircBuff[s-1] != INT_MAX);
        int has_right = (e < n && pSta->plRawCircBuff[e]   != INT_MAX);
        long left  = has_left  ? pSta->plRawCircBuff[s-1] : 0;
        long right = has_right ? pSta->plRawCircBuff[e]   : 0;

        if (!has_left || !has_right) {
            for (long j = s; j < e; j++)
                pSta->plRawCircBuff[j] = INT_MAX;
        } else {
            for (long k = s; k < e; k++) {
                double frac = (double)(k - s + 1) / (double)(len + 1);
                pSta->plRawCircBuff[k] = (long)((double)left + frac * (double)(right - left));
            }
        }
    }
}

/* --------------------------------------------------------------------
 * DemeanTrace: Calcula y remueve el offset DC de la señal
 * -------------------------------------------------------------------- */
void DemeanTrace(STATION *pSta) {
    if (pSta->lRawCircCtr <= 0) return;
    
    double mean = 0.0;
    long nvalid = 0;
    for (long k = 0; k < pSta->lRawCircCtr; k++) {
        if (pSta->plRawCircBuff[k] == INT_MAX) continue;
        mean += (double)pSta->plRawCircBuff[k];
        nvalid++;
    }
    if (nvalid > 0) mean /= (double)nvalid;
    
    for (long k = 0; k < pSta->lRawCircCtr; k++) {
        if (pSta->plRawCircBuff[k] == INT_MAX) {
            pSta->plFiltCircBuff[k] = INT_MAX;
        } else {
            pSta->plFiltCircBuff[k] = (long)(pSta->plRawCircBuff[k] - mean);
        }
    }
}
