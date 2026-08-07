#ifndef idatap_h_included
#define idatap_h_included
#include "ida_platform.h"
#include "util.h"
#include "xfer.h"
#include <ctype.h>

void IdatapInit(void);
char *util_lcase(char *c);
char *util_ucase(char *c);
int util_getline(FILE *fp, char *buffer, int  buflen, char comment, int  *lineno);
int xfer_RecvCnf(int sd, struct xfer_cnf *cnf);
int xfer_DecodeWav(struct xfer_cnf *cnf, struct xfer_wav *wav, char *src);
int xfer_SendReq(int sd, struct xfer_req *req);
int Xfer_RecvWav(int sd, struct xfer_cnf *cnf, struct xfer_wav *wav);
int xfer_SendMsg(int sd, char *message_plus, long msglen);
int xfer_RecvMsg(int sd, char *message, long maxlen, long *len);
struct xfer_time *xfer_time(double dtime);
double xfer_dtime(struct xfer_time *ntime);
char *Xfer_ErrStr(void);
void Xfer_Exit(int sd, int status);
int Xfer_Connect2(char *host, char *service, int port, char *unused1, struct xfer_req *req, struct xfer_cnf *cnf, int retry, int unused2);
long util_lcomp(char *dest, long *src, long nsamp);
long util_ldcmp(long *dest, char *src, long nsamp);
long util_scomp(char *dest, short *src, long nsamp);
short util_sdcmp(short *dest, char *src, long nsamp);
int xfer_Convert(struct xfer_cnf *cnf, struct xfer_wav *wav, struct xfer_packet *packet);
XFER *Xfer_Open2(char *host, int port, char *sc, double beg, double end, int keepup, int retry, int to, int tto);
int Xfer_Read(XFER *xp, struct xfer_packet *packet);
void Xfer_Close(XFER *xp);
int util_lock(int fd, int cmd, int type, off_t offset, int whence, off_t len);
int Xfer_ParseCC(struct xfer_req *req, int i, char *cc, double beg, double end);
int Xfer_ParseSC(struct xfer_req *req, char *sc, double beg, double end);
int Xfer_ParseSCSC(struct xfer_req *req, char *scsc, double beg, double end);
int Xfer_FillReq(struct xfer_req *req, int cnffmt, int wavfmt, char *stachn, double beg, double end, int keepup, int timeout);
int util_sparse(char *input, char **argv, char *delimiters, int  max_tokens);
int util_parse(char *input, char **argv, char *delimiters, int  max_tokens, char quote);
int parse(FILE *fp, char **argv, char *delimiters, int  max_tokens);
int Xfer_WriteCnfGen1(FILE *fp, struct xfer_cnfgen1 *cnf);
int Xfer_ReadCnfGen1(FILE *fp, struct xfer_cnfgen1 *cnf);
int Xfer_WriteWavGen1(FILE *fp, struct xfer_wavgen1 *wav);
int Xfer_ReadWavGen1(FILE *fp, struct xfer_wavgen1 *wav);
int Xfer_WriteCnf(FILE *fp, struct xfer_cnf *cnf);
int Xfer_ReadCnf(FILE *fp, struct xfer_cnf *cnf);
int Xfer_WriteWav(FILE *fp, struct xfer_wav *wav, int strip);
int Xfer_ReadWav(FILE *fp, struct xfer_wav *wav);
char *util_strpad(char *input, int maxlen, int padchar);
char *util_strtrm(char *input);
void util_lswap(long *input, long number);
void util_sswap(short *input, long number);
void util_iftovf(unsigned long *input, long number);
void util_vftoif(unsigned long *input, long number);
unsigned long util_order(void);
char *syserrmsg(int code);
double util_attodt(char *string);
char *util_dttostr(double dtime, int code);
char *util_lttostr(long ltime, int code);
void util_tsplit(double dtime, int *yr, int *da, int *hr, int *mn, int *sc, int *ms);
double util_ydhmsmtod(int yr, int da, int hr, int mn, int sc, int ms);
int util_jdtomd(int year, int day, int *m_no, int *d_no);
int util_ymdtojd(int year, int mo, int da);
long util_today(void);
#ifndef WINNT
void util_incrloglevel(void);
void util_rsetloglevel(void);
void util_logclose(void);
int util_logopen(char *file, int min_level, int max_level, int log_level, char *tfmt, char *fmt, ...);
void util_log(int level, char *fmt, ...);
void Xfer_LogReq(int level, struct xfer_req *req);
void xfer_LogCnfGen1(int level, struct xfer_cnfgen1 *cnf);
void xfer_LogWavGen1(int level, struct xfer_wavgen1 *wav);
void Xfer_LogCnf(int level, struct xfer_cnf *cnf);
void Xfer_LogWav(int level, struct xfer_wav *wav);
Sigfunc *util_signal(int signo, Sigfunc func);
char *util_sigtoa(int signal);
#endif
#endif
