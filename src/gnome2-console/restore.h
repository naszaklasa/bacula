
typedef struct {
   GtkCList  *list;
   POOLMEM *buf;                      /* scratch buffer */
   POOLMEM *fname;                    /* full path and file name */
   POOLMEM *path;                     /* path, no file */
   POOLMEM *file;                     /* file, no path */
   int fnl;                           /* file length */
   int pnl;                           /* path length */
} Window;
