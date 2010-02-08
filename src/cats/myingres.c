
#include "bacula.h"
#include "cats.h"

#ifdef HAVE_INGRES

#include <eqdefc.h>
# include <eqsqlca.h>
extern IISQLCA sqlca;   /* SQL Communications Area */
#include <eqsqlda.h>

#include "myingres.h"
#define INGRES_DEBUG 0
#define DEBB(x) if (INGRES_DEBUG >= x) {
#define DEBE }

/* ---Implementations--- */
int INGcheck()
{
  char errbuf[256];
        if (sqlca.sqlcode < 0)
        {
/* # line 23 "myingres.sc" */   /* inquire_ingres */
  {
    IILQisInqSqlio((short *)0,1,32,255,errbuf,63);
  }
/* # line 24 "myingres.sc" */   /* host code */
                printf("Ingres-DBMS-Fehler: %s\n", errbuf);
                return sqlca.sqlcode;
    }
        else
                return 0;
}
short INGgetCols(const char *stmt)
{
    short number = 1;
    IISQLDA *sqlda;
    sqlda = (IISQLDA *)calloc(1,IISQDA_HEAD_SIZE + (number * IISQDA_VAR_SIZE));
    if (sqlda == (IISQLDA *)0)
        { printf("Failure allocating %d SQLDA elements\n",number); }
    sqlda->sqln = number;
  char stmt_buffer[2000];
    strcpy(stmt_buffer,stmt);
/* # line 46 "myingres.sc" */   /* prepare */
  {
    IIsqInit(&sqlca);
    IIsqPrepare(0,(char *)"s1",(char *)0,0,stmt_buffer);
  }
/* # line 47 "myingres.sc" */   /* describe */
  {
    IIsqInit(&sqlca);
    IIsqDescribe(0,(char *)"s1",sqlda,0);
  }
/* # line 49 "myingres.sc" */   /* host code */
    number = sqlda->sqld;
    free(sqlda);
    return number;
}
IISQLDA *INGgetDescriptor(short numCols, const char *stmt)
{
    IISQLDA *sqlda;
    sqlda = (IISQLDA *)calloc(1,IISQDA_HEAD_SIZE + (numCols * IISQDA_VAR_SIZE));
    if (sqlda == (IISQLDA *)0) 
        { printf("Failure allocating %d SQLDA elements\n",numCols); }
    sqlda->sqln = numCols;
  char stmt_buffer[2000];
    strcpy(stmt_buffer,stmt);
/* # line 69 "myingres.sc" */   /* prepare */
  {
    IIsqInit(&sqlca);
    IIsqPrepare(0,(char *)"s2",sqlda,0,stmt_buffer);
  }
/* # line 71 "myingres.sc" */   /* host code */
    int i;
    for (i=0;i<sqlda->sqld;++i)
    {
        sqlda->sqlvar[i].sqldata =
            (char *)malloc(sqlda->sqlvar[i].sqllen);
        if (sqlda->sqlvar[i].sqldata == (char *)0)
            { printf("Failure allocating %d bytes for SQLVAR data\n",sqlda->sqlvar[i].sqllen); }
        sqlda->sqlvar[i].sqlind = (short *)malloc(sizeof(short));
        if (sqlda->sqlvar[i].sqlind == (short *)0) 
            { printf("Failure allocating sqlind\n"); }
    }
    return sqlda;
}
void INGfreeDescriptor(IISQLDA *sqlda)
{
    int i;
    for ( i = 0 ; i < sqlda->sqld ; ++i )
    {
        free(sqlda->sqlvar[i].sqldata);
        free(sqlda->sqlvar[i].sqlind);
    }
    free(sqlda);
    sqlda = NULL;
}
int INGgetTypeSize(IISQLVAR *ingvar)
{
    int inglength = 0;
        switch (ingvar->sqltype)
    {
                case IISQ_DTE_TYPE:
                inglength = 25;
                    break;
                case IISQ_MNY_TYPE:
                    inglength = 8;
                    break;
                default:
                    inglength = ingvar->sqllen;
    }
        return inglength;
}
INGresult *INGgetINGresult(IISQLDA *sqlda)
{
    INGresult *result = NULL;
    result = (INGresult *)calloc(1, sizeof(INGresult));
    if (result == (INGresult *)0) 
        { printf("Failure allocating INGresult\n"); }
    result->sqlda       = sqlda;
    result->num_fields  = sqlda->sqld;
    result->num_rows    = 0;
    result->first_row   = NULL;
    result->status      = ING_EMPTY_RESULT;
    result->act_row     = NULL;
    strcpy(result->numrowstring,"");
    result->fields = (INGRES_FIELD *)calloc(1, sizeof(INGRES_FIELD) * result->num_fields);
    if (result->fields == (INGRES_FIELD *)0)
        { printf("Failure allocating %d INGRES_FIELD elements\n",result->num_fields); }
    DEBB(2)
        printf("INGgetINGresult, before loop over %d fields\n", result->num_fields);
    DEBE
    int i;
    for (i=0;i<result->num_fields;++i)
    {
        memset(result->fields[i].name,'\0',34);
        strncpy(result->fields[i].name,
            sqlda->sqlvar[i].sqlname.sqlnamec,
            sqlda->sqlvar[i].sqlname.sqlnamel);
        result->fields[i].max_length    = INGgetTypeSize(&sqlda->sqlvar[i]);
        result->fields[i].type          = abs(sqlda->sqlvar[i].sqltype);
        result->fields[i].flags         = (abs(sqlda->sqlvar[i].sqltype)<0) ? 1 : 0;
    }
    return result;
}
void INGfreeINGresult(INGresult *ing_res)
{
    /* TODO: free all rows and fields, then res, not descriptor! */
    if( ing_res != NULL )
    {
        /* use of rows is a nasty workaround til I find the reason,
           why aggregates like max() don't work
         */
        int rows = ing_res->num_rows;
        ING_ROW *rowtemp;
        ing_res->act_row = ing_res->first_row;
        while (ing_res->act_row != NULL && rows > 0)
        {
            rowtemp = ing_res->act_row->next;
            INGfreeRowSpace(ing_res->act_row, ing_res->sqlda);
            ing_res->act_row = rowtemp;
            --rows;
        }
        free(ing_res->fields);
    }
    free(ing_res);
    ing_res = NULL;
}
ING_ROW *INGgetRowSpace(INGresult *ing_res)
{
    IISQLDA *sqlda = ing_res->sqlda;
    ING_ROW *row = NULL;
    IISQLVAR *vars = NULL;
    row = (ING_ROW *)calloc(1,sizeof(ING_ROW));
    if (row == (ING_ROW *)0)
        { printf("Failure allocating ING_ROW\n"); }
    vars = (IISQLVAR *)calloc(1,sizeof(IISQLVAR) * sqlda->sqld);
    if (vars == (IISQLVAR *)0)
        { printf("Failure allocating %d SQLVAR elements\n",sqlda->sqld); }
    row->sqlvar = vars;
    row->next = NULL;
    int i;
    unsigned short len; /* used for VARCHAR type length */
    for (i=0;i<sqlda->sqld;++i)
    {
        /* make strings out of the data, then the space and assign 
            (why string? at least it seems that way, looking into the sources)
         */
        switch (abs(ing_res->fields[i].type))
        {
            case IISQ_VCH_TYPE:
                len = ((ING_VARCHAR *)sqlda->sqlvar[i].sqldata)->len;
                DEBB(2)
                    printf("length of varchar: %d\n", len);
                DEBE
                vars[i].sqldata = (char *)malloc(len+1);
                if (vars[i].sqldata == (char *)0)
                    { printf("Failure allocating %d bytes for SQLVAR data\n",len+1); }
                memcpy(vars[i].sqldata,sqlda->sqlvar[i].sqldata+2,len);
                vars[i].sqldata[len] = '\0';
                break;
            case IISQ_CHA_TYPE:
                vars[i].sqldata = (char *)malloc(ing_res->fields[i].max_length+1);
                if (vars[i].sqldata == (char *)0)
                    { printf("Failure allocating %d bytes for SQLVAR data\n",ing_res->fields[i].max_length); }
                memcpy(vars[i].sqldata,sqlda->sqlvar[i].sqldata,sqlda->sqlvar[i].sqllen);
                vars[i].sqldata[ing_res->fields[i].max_length] = '\0';
                break;
            case IISQ_INT_TYPE:
                vars[i].sqldata = (char *)malloc(20);
                memset(vars[i].sqldata,'\0',20);
                sprintf(vars[i].sqldata,"%d",*(int*)sqlda->sqlvar[i].sqldata);
                break;
        }
        vars[i].sqlind = (short *)malloc(sizeof(short));
        if (sqlda->sqlvar[i].sqlind == (short *)0) 
            { printf("Failure allocating sqlind\n"); }
        memcpy(vars[i].sqlind,sqlda->sqlvar[i].sqlind,sizeof(short));
        DEBB(2)
            printf("INGgetRowSpace, Field %d, type %d, length %d, name %s\n",
            i, sqlda->sqlvar[i].sqltype, sqlda->sqlvar[i].sqllen, ing_res->fields[i].name);
        DEBE
    }
    return row;
}
void INGfreeRowSpace(ING_ROW *row, IISQLDA *sqlda)
{
    int i;
    if (row == NULL || sqlda == NULL)
    {
        printf("INGfreeRowSpace: one argument is NULL!\n");
        return;
    }
    for ( i = 0 ; i < sqlda->sqld ; ++i )
    {
        free(row->sqlvar[i].sqldata);
        free(row->sqlvar[i].sqlind);
    }
    free(row->sqlvar);
    free(row);
}
int INGfetchAll(const char *stmt, INGresult *ing_res)
{
    int linecount = 0;
    ING_ROW *row;
    IISQLDA *desc;
  char stmt_buffer[2000];
    strcpy(stmt_buffer,stmt);
    desc = ing_res->sqlda;
/* # line 275 "myingres.sc" */  /* host code */
    INGcheck();
/* # line 277 "myingres.sc" */  /* open */
  {
    IIsqInit(&sqlca);
    IIcsOpen((char *)"c2",19215,16475);
    IIwritio(0,(short *)0,1,32,0,(char *)"s2");
    IIcsQuery((char *)"c2",19215,16475);
  }
/* # line 278 "myingres.sc" */  /* host code */
    INGcheck();
    /* for (linecount=0;sqlca.sqlcode==0;++linecount) */
    while(sqlca.sqlcode==0)
    {
/* # line 283 "myingres.sc" */  /* fetch */
  {
    IIsqInit(&sqlca);
    if (IIcsRetScroll((char *)"c2",19215,16475,-1,-1) != 0) {
      IIcsDaGet(0,desc);
      IIcsERetrieve();
    } /* IIcsRetrieve */
  }
/* # line 284 "myingres.sc" */  /* host code */
        INGcheck();
        if (sqlca.sqlcode == 0)
        {
            row = INGgetRowSpace(ing_res); /* alloc space for fetched row */
            /* initialize list when encountered first time */
            if (ing_res->first_row == 0) 
            {
                ing_res->first_row = row; /* head of the list */
                ing_res->first_row->next = NULL;
                ing_res->act_row = ing_res->first_row;
            }   
            ing_res->act_row->next = row; /* append row to old act_row */
            ing_res->act_row = row; /* set row as act_row */
            row->row_number = linecount;
            ++linecount;
            DEBB(2)
            int i;
            printf("Row %d ", linecount);
            for (i=0;i<ing_res->num_fields;++i)
                { printf("F%d:%s ",i,row->sqlvar[i].sqldata); }
            printf("\n");
            DEBE
        }
    }
/* # line 313 "myingres.sc" */  /* close */
  {
    IIsqInit(&sqlca);
    IIcsClose((char *)"c2",19215,16475);
  }
/* # line 315 "myingres.sc" */  /* host code */
    ing_res->status = ING_COMMAND_OK;
    ing_res->num_rows = linecount;
    return linecount;
}
ING_STATUS INGresultStatus(INGresult *res)
{
    if (res == NULL) {return ING_NO_RESULT;}
    return res->status;
}
void INGrowSeek(INGresult *res, int row_number)
{
    if (res->act_row->row_number == row_number) { return; }
    /* TODO: real error handling */
    if (row_number<0 || row_number>res->num_rows) { return;}
    ING_ROW *trow = res->first_row;
    while ( trow->row_number != row_number )
    { trow = trow->next; }
    res->act_row = trow;
    /* note - can be null - if row_number not found, right? */
}
char *INGgetvalue(INGresult *res, int row_number, int column_number)
{
    if (row_number != res->act_row->row_number)
        { INGrowSeek(res, row_number); }
    return res->act_row->sqlvar[column_number].sqldata;
}
int INGgetisnull(INGresult *res, int row_number, int column_number)
{
    if (row_number != res->act_row->row_number)
        { INGrowSeek(res, row_number); }
    return (short)*res->act_row->sqlvar[column_number].sqlind;
}
int INGntuples(const INGresult *res)
{
    return res->num_rows;
}
int INGnfields(const INGresult *res)
{
    return res->num_fields;
}
char *INGfname(const INGresult *res, int column_number)
{
    if ( (column_number > res->num_fields) || (column_number < 0) )
        { return NULL; }
    else
        { return res->fields[column_number].name; }
}
short INGftype(const INGresult *res, int column_number)
{
    return res->fields[column_number].type;
}
INGresult *INGexec(INGconn *conn, const char *query)
{
    /* TODO: error handling -> res->status? */
    IISQLDA *desc = NULL;
    INGresult *res = NULL;
    int cols = -1;
  char stmt[2000];
    strncpy(stmt,query,strlen(query));
    stmt[strlen(query)]='\0';
    DEBB(1)
        printf("INGexec: query is >>%s<<\n",stmt);
    DEBE
    if ((cols = INGgetCols(query)) == 0)
    {
        DEBB(1)
            printf("INGexec: non-select\n");
        DEBE
        /* non-select statement - TODO: EXECUTE IMMEDIATE */
/* # line 400 "myingres.sc" */  /* execute */
  {
    IIsqInit(&sqlca);
    IIsqExImmed(stmt);
    IIsyncup((char *)0,0);
  }
/* # line 401 "myingres.sc" */  /* host code */
    }
    else
    {
        DEBB(1)
            printf("INGexec: select\n");
        DEBE
        /* select statement */
        desc = INGgetDescriptor(cols, query);
        res = INGgetINGresult(desc);
        INGfetchAll(query, res);
    }
    return res;
}
void INGclear(INGresult *res)
{
    if (res == NULL) { return; }
    IISQLDA *desc = res->sqlda;
    INGfreeINGresult(res);
    INGfreeDescriptor(desc);
}
INGconn *INGconnectDB(char *dbname, char *user, char *passwd)
{
    if (dbname == NULL || strlen(dbname) == 0)
        { return NULL; }
    INGconn *dbconn = (INGconn *)calloc(1,sizeof(INGconn));
    if (dbconn == (INGconn *)0)
        { printf("Failure allocating INGconn\n"); }
  char ingdbname[24];
  char ingdbuser[32];
  char ingdbpasw[32];
  char conn_name[32];
  int sess_id;
    strcpy(ingdbname,dbname);
    if ( user != NULL)
    {
        DEBB(1)
            printf("Connection: with user/passwd\n");
        DEBE
        strcpy(ingdbuser,user);
        if ( passwd != NULL)
            { strcpy(ingdbpasw,passwd); }
        else
            { strcpy(ingdbpasw, ""); }
/* # line 452 "myingres.sc" */  /* connect */
  {
    IIsqInit(&sqlca);
    IIsqUser(ingdbuser);
    IIsqConnect(0,ingdbname,(char *)"-dbms_password",ingdbpasw,(char *)0, 
    (char *)0, (char *)0, (char *)0, (char *)0, (char *)0, (char *)0, 
    (char *)0, (char *)0, (char *)0, (char *)0);
  }
/* # line 456 "myingres.sc" */  /* host code */
    }
    else
    {
        DEBB(1)
            printf("Connection: w/ user/passwd\n");
        DEBE
/* # line 462 "myingres.sc" */  /* connect */
  {
    IIsqInit(&sqlca);
    IIsqConnect(0,ingdbname,(char *)0, (char *)0, (char *)0, (char *)0, 
    (char *)0, (char *)0, (char *)0, (char *)0, (char *)0, (char *)0, 
    (char *)0, (char *)0, (char *)0);
  }
/* # line 463 "myingres.sc" */  /* host code */
    }   
/* # line 465 "myingres.sc" */  /* inquire_sql */
  {
    IILQisInqSqlio((short *)0,1,32,31,conn_name,13);
  }
/* # line 466 "myingres.sc" */  /* inquire_sql */
  {
    IILQisInqSqlio((short *)0,1,30,sizeof(sess_id),&sess_id,11);
  }
/* # line 468 "myingres.sc" */  /* host code */
    strcpy(dbconn->dbname,ingdbname);
    strcpy(dbconn->user,ingdbuser);
    strcpy(dbconn->password,ingdbpasw);
    strcpy(dbconn->connection_name,conn_name);
    dbconn->session_id = sess_id;
    DEBB(1)
        printf("Connected to '%s' with user/passwd %s/%s, sessID/name %i/%s\n",
            dbconn->dbname,
            dbconn->user,
            dbconn->password,
            dbconn->session_id,
            dbconn->connection_name
        );
    DEBE
    return dbconn;
}
void INGdisconnectDB(INGconn *dbconn)
{
    /* TODO: use of dbconn */
/* # line 490 "myingres.sc" */  /* disconnect */
  {
    IIsqInit(&sqlca);
    IIsqDisconnect();
  }
/* # line 491 "myingres.sc" */  /* host code */
    free(dbconn);
}
char *INGerrorMessage(const INGconn *conn)
{
    return NULL;
}
char    *INGcmdTuples(INGresult *res)
{
    return res->numrowstring;
}
/*      TODO?
char *INGerrorMessage(const INGconn *conn);
int INGputCopyEnd(INGconn *conn, const char *errormsg);
int INGputCopyData(INGconn *conn, const char *buffer, int nbytes);
*/

#endif
