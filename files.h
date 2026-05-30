#ifndef FILES_H
#define FILES_H

#define MAX_FILES        16
#define MAX_FILENAME     32
#define MAX_FILE_CONTENT 512
#define MAX_OPEN_FILES   8     /* por proceso */

typedef struct {
    int  file_id;
    char name[MAX_FILENAME];
    char content[MAX_FILE_CONTENT];
    int  size;
    int  exists;
    int  locked_by_pid;   /* -1 = no bloqueado */
} FileEntry;

typedef struct {
    int file_id;           /* -1 = entrada libre */
    int pid;
    int mode;              /* 0=lectura, 1=escritura */
} OpenFileEntry;

void  files_init(void);
int   file_create(const char *name);
int   file_open(const char *name, int pid, int mode);
int   file_read(int fd, int pid);
int   file_write(int fd, int pid, const char *data);
int   file_close(int fd, int pid);
int   file_delete(const char *name, int pid);
void  files_list(void);
void  files_open_table_show(int pid);

#endif
