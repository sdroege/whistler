#ifndef __FFT_H__
#define __FFT_H__

/* cdft: Complex Discrete Fourier Transform */
void cdft(int n, int isgn, double *a, int *ip, double *w);
/* rdft: Real Discrete Fourier Transform */
void rdft(int n, int isgn, double *a, int *ip, double *w);
/* ddct: Discrete Cosine Transform */
void ddct(int n, int isgn, double *a, int *ip, double *w);
/* ddst: Discrete Sine Transform */
void ddst(int n, int isgn, double *a, int *ip, double *w);
/* dfct: Cosine Transform of RDFT (Real Symmetric DFT) */
void dfct(int n, double *a, double *t, int *ip, double *w);
/* dfst: Sine Transform of RDFT (Real Anti-symmetric DFT) */
void dfst(int n, double *a, double *t, int *ip, double *w);

#endif /* __FFT_H__ */
