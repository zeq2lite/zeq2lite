/* string.h * Definitions for memory and string functions.  */
#ifndef __string_h__
#define	__string_h__
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned int size_t;
#endif
#ifndef NULL
#define NULL 0L
#endif
void *	 memchr(const void *, int, size_t);
int 	 memcmp(const void *, const void *, size_t);
void * 	 memcpy(void *, const void *, size_t);
void *	 memmove(void *, const void *, size_t);
void *	 memset(void *, int, size_t);
char 	*strcat(char *, const char *);
char 	*strchr(const char *, int);
int	 strcmp(const char *, const char *);
int	 strcoll(const char *, const char *);
#if __LCCOPTIMLEVEL > 0
char * _stdcall strcpy(char *,const char *);
#else
char 	* strcpy(char *, const char *);
#endif
size_t	 strcspn(const char *, const char *);
char    *strupr(char *);
char    *strlwr(char *);
char 	*strerror(int);
#if __LCCOPTIMLEVEL > 0 && defined(__STRLEN)
size_t   _stdcall _strlen(char *);
#define strlen _strlen
#else
size_t	 strlen(const char *);
#endif
char 	*strncat(char *, const char *, size_t);
int	 strncmp(const char *, const char *, size_t);
char 	*strncpy(char *, const char *, size_t);
char 	*strpbrk(const char *, const char *);
char 	*strrchr(const char *, int);
size_t	 strspn(const char *, const char *);
char 	*strstr(const char *, const char *);
char    *stristr(char *,char *);
char 	*strtok(char *, const char *);
void *	 memccpy(void *, const void *, int, size_t);
void *   _memccpy(void *,void *,unsigned int);
char 	*strdup(const char *);
char	*strrev(char *);
int	 strnicmp(const char *, const char *, size_t);
void	 swab(const char *, char *, size_t);
int	 stricmp(char *,char *);
char	*_strset( char *,int);
int	 strcmpi(char *,char *);
#define memicmp _memicmp
int memicmp(void *,void *,unsigned int);
int strtrim(char *);
#endif /* __string_h__ */
