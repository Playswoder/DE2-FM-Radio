#ifdef __cplusplus
extern "C" {
#endif

typedef struct Si4703 Si4703;

Si4703* Si4703_create(int rst, int sdio, int sclk, int intp);
void    Si4703_powerUp(Si4703* obj);
void    Si4703_setChannel(Si4703* obj, int freq);
int     Si4703_getChannel(Si4703* obj);
void    Si4703_destroy(Si4703* obj);
void    Si4703_setChannel(Si4703* obj, int freq);


#ifdef __cplusplus
}
#endif