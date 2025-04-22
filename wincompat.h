/* C99 (ISO/IEC 9899:1999) - 5hif7y */
/* A little helper to force POSIX compatibility for suckless tools on Windows */

#ifndef WINCOMPAT_H
#define WINCOMPAT_H
#ifdef _WIN32

#pragma warning(disable:4996) /* TODO: reimplement '_fdopen' for MSVC */
//#define _CRT_SECURE_NO_WARNINGS /* TODO: use secure implementation 'strcpy' to 'strcpy_s' */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sys/stat.h>
#include <io.h>
#include <direct.h>
#include <fcntl.h>
#include <stdlib.h>

/* --------- posix datatype --------- */
typedef int mode_t;

/* --------- misc defines --------- */
#define PATH_MAX 260
#ifdef _MSC_VER
  #define access _access
  #define fdopen _fdopen /* TODO: reimplement 'fdopen' for Windows MSVC */
  #define umask _umask
  #define chmod _chmod
#endif
#define warnx(fmt, ...) (fprintf(stderr, fmt "\n", ##__VA_ARGS__))
#define err(exitcode, fmt, ...) do { \
    fprintf(stderr, fmt "\n", ##__VA_ARGS__); \
    exit(exitcode); \
} while (0)
#define errx(exitcode, fmt, ...) err(exitcode, fmt, ##__VA_ARGS__)
/* if your toolchain is using MSVC, use the native implementation '_strdup' */
#if defined(_MSC_VER)
  #define strdup _strdup
/* Use this if your toolchain doesn't have an 'strdup' implementation */
#elif defined(CUSTOM_STRDUP)
  static char *custom_strdup(const char *s){
    if(!s){ return NULL;}
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy) { memcpy(copy, s, len); }
    return copy;
  }
  #define strdup custom_strdup
#endif /* _MSC_VER xor CUSTOM_STRDUP */

/* --------- posix functions reimplementations --------- */
static int mkstemp(char *templte){
  char *XXXXXX = strstr(templte, "XXXXXX");
  if (!XXXXXX || strlen(XXXXXX) != 6) {
    return -1;
  }
  for (int i = 0; i < 100; ++i) {
    for (int j = 0; j < 6; ++j) {
      XXXXXX[j] = 'A' + rand() % 26;
    }
    int fd = _open(templte, _O_RDWR | _O_CREAT | _O_EXCL, _S_IREAD | _S_IWRITE);
    if (fd != -1) {
      return fd;
    }
  }
  return -1;
}

static char *win_basename(const char *path){
  if (!path || !*path){ return NULL; }
 
  const char *last_slash = strrchr(path, '\\');
  const char *last_fslash = strrchr(path, '/');
  const char *base = path;
  
  if (last_slash && last_fslash){
    base = (last_slash > last_fslash) ? last_slash + 1 : last_fslash + 1;
  }
  else if (last_slash) {
    base = last_slash + 1;
  }
  else if (last_fslash){
    base = last_fslash + 1;
  }
  return strdup(base);
}
#define basename(path) win_basename(path)

static char *win_dirname(char *path){
  static char buffer[MAX_PATH];
  char *slash;
  if (!path || !*path){ 
    strcpy(buffer, "."); 
    return buffer;
  }
  strncpy(buffer, path, MAX_PATH);
  buffer[MAX_PATH - 1] = '\0';  // prevenir falta de terminación nula
  slash = strrchr(buffer, '\\');
  if (!slash){
    slash = strrchr(buffer, '/');
  }
  if (slash){
    *slash = '\0';
  } else {
    strcpy(buffer, ".");
  }
  return buffer;
}
#define dirname(path) win_dirname(path)

static char *realpath(const char *path, char *resolved_path){
  if(!resolved_path){ resolved_path = (char*)malloc(MAX_PATH); }
  _fullpath(resolved_path, path, MAX_PATH);
  return resolved_path;
}

static int win_mkdir(const char *path, mode_t mode){
  (void)mode; /* discard unused variable in a way the compiler doesn't complain */
  return _mkdir(path);
}
#define mkdir(path, mode) win_mkdir(path, mode)

/* --------- access --------- */
#ifndef F_OK
#define F_OK 0
#endif

/* --------- permission macros --------- */
#define S_IRWXU 0700
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRWXG 0070
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IXGRP 0010
#define S_IRWXO 0007
#define S_IROTH 0004
#define S_IWOTH 0002
#define S_IXOTH 0001
#define S_ISUID 0004000
#define S_ISGID 0002000
#define S_ISVTX 0001000

/* --------- S_ISXXX macros --------- */
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)  (0)  // doesn't exist in Windows
#define S_ISFIFO(m) (0)
#define S_ISLNK(m)  (0)
#define S_ISSOCK(m) (0)

#endif /* _WIN32 */
#endif /* WINCOMPAT_H */
