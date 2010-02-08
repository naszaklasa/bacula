# include "/opt/Ingres/IngresII/ingres/files/eqdefc.h"
#ifndef _MYINGRES_SH
#define _MYINGRES_SH
# include "/opt/Ingres/IngresII/ingres/files/eqsqlda.h"
/* ---typedefs--- */
typedef struct ing_field {
   char          name[34];
   int           max_length;
   unsigned int  type;
   unsigned int  flags;       // 1 == not null
} INGRES_FIELD;
typedef struct ing_row {
    IISQLVAR *sqlvar;		/* ptr to sqlvar[sqld] for one row */
    struct ing_row *next;
    int row_number;
} ING_ROW;
typedef enum ing_status {
    ING_COMMAND_OK,
    ING_TUPLES_OK,
    ING_NO_RESULT,
    ING_NO_ROWS_PROCESSED,
    ING_EMPTY_RESULT,
    ING_ERROR
} ING_STATUS;
typedef struct ing_varchar {
    short len;
    char* value;
} ING_VARCHAR;
/* It seems, Bacula needs the complete query result stored in one data structure */
typedef struct ing_result {
    IISQLDA *sqlda;		/* descriptor */
    INGRES_FIELD *fields;
    int num_rows;
    int num_fields;
    ING_STATUS status;
    ING_ROW *first_row;
    ING_ROW *act_row;		/* just for iterating */
    char numrowstring[10];
} INGresult;
typedef struct ing_conn {
    char dbname[24];
    char user[32];
    char password[32];
    char connection_name[32];
    int session_id;    
} INGconn;
/* ---Prototypes--- */
int	INGcheck();
ING_STATUS	INGresultStatus(INGresult *res);
short	INGgetCols(const char *stmt);
IISQLDA *INGgetDescriptor(short numCols, const char *stmt);
void	INGfreeDescriptor(IISQLDA *sqlda);
int	INGgetTypeSize(IISQLVAR *ingvar);
INGresult	*INGgetINGresult(IISQLDA *sqlda);
void	INGfreeINGresult(INGresult *ing_res);
ING_ROW *INGgetRowSpace(INGresult *ing_res);
void	INGfreeRowSpace(ING_ROW *row, IISQLDA *sqlda);
int	INGfetchAll(const char *stmt, INGresult *ing_res);
void	INGrowSeek(INGresult *res, int row_number);
char 	*INGgetvalue(INGresult *res, int row_number, int column_number);
int 	INGgetisnull(INGresult *res, int row_number, int column_number);
int 	INGntuples(const INGresult *res);
int 	INGnfields(const INGresult *res);
char 	*INGfname(const INGresult *res, int column_number);
short 	INGftype(const INGresult *res, int column_number);
INGresult	*INGexec(INGconn *db, const char *query);
void 	INGclear(INGresult *res);
INGconn *INGconnectDB(char *dbname, char *user, char *passwd);
void 	INGdisconnectDB(INGconn *dbconn);
char	*INGerrorMessage(const INGconn *conn);
char	*INGcmdTuples(INGresult *res);
#endif /* _MYINGRES_SH */
