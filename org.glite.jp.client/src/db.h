#ifndef _DB_H
#define _DB_H

#ident "$Header$"

#include <sys/types.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _glite_jp_db_stmt_t *glite_jp_db_stmt_t;

int glite_jp_db_connect(
	glite_jp_context_t,	/* INOUT: */
	char *		/* IN: connect string user/password@host:database */
);

void glite_jp_db_close(glite_jp_context_t);


/* Parse and execute SQL statement. Returns number of rows selected, created 
 * or affected by update, or -1 on error */

int glite_jp_db_execstmt(
	glite_jp_context_t,	/* INOUT: */
	char *,		/* IN: SQL statement */
	glite_jp_db_stmt_t *	/* OUT: statement handle. Usable for
					select only */
);


/* Fetch next row of select statement. 
 * All columns are returned as fresh allocated strings 
 *
 * return values:
 * 	>0 - number of fields of the retrieved row
 * 	 0 - no more rows
 * 	-1 - error
 *
 * Errors are stored in context passed to previous glite_jp_db_execstmt() */

int glite_jp_db_fetchrow(
	glite_jp_db_stmt_t,	/* IN: statement */
	char **		/* OUT: array of fetched values. 
			 *      As number of columns is fixed and known,
			 *      expects allocated array of pointers here */
);

/* Retrieve column names of a query statement */

int glite_jp_db_querycolumns(
	glite_jp_db_stmt_t,	/* IN: statement */
	char **		/* OUT: result set column names. Expects allocated array. */
);

/* Free the statement structure */

void glite_jp_db_freestmt(
	glite_jp_db_stmt_t *    /* INOUT: statement */
);


/* convert time_t into database-specific time string 
 * returns pointer to static area that is changed by subsequent calls */

char *glite_jp_db_timetodb(time_t);
time_t glite_jp_db_dbtotime(char *);


/**
 * Check database version.
 */
int glite_jp_db_dbcheckversion(glite_jp_context_t);


#ifdef __cplusplus
}
#endif

#endif
