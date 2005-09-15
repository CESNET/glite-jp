#ifndef _DB_H
#define _DB_H

#ident "$Header$"

#include <sys/types.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _glite_jp_db_stmt_t *glite_jp_db_stmt_t;


/**
 * Connect to the database.
 *
 * \param[inout] cxt	context to work with
 * \param[in] cs	connect string user/password@host:database
 *
  \return	JP error code
 */
int glite_jp_db_connect(glite_jp_context_t,const char *);


/**
 * Close the connection to database.
 *
 * \param[inout] ctx	context to work with
 */
void glite_jp_db_close(glite_jp_context_t);


/**
 * Parse and execute SQL statement.
 *
 * \param[inout] ctx	context to work with
 * \param[in] txt	SQL statement
 * \param[out] stmt	statement handle, usable for select only
 *
 * \return	number of rows selected, created or affected by update, or -1 on error
 */
int glite_jp_db_execstmt(glite_jp_context_t, const char *, glite_jp_db_stmt_t *);


/** Fetch next row of select statement. 
 * All columns are returned as fresh allocated strings 
 *
 * \param[inout] stmt	statement from glite_jp_db_execstmt()
 * \param[out]	array of fetched values.
 *              As number of columns is fixed and known,
 *              expects allocated array of pointers here.
 *
 * \retval >0	number of fields of the retrieved row
 * \retval 0	no more rows
 * \retval -1	error
 *
 * Errors are stored in context passed to previous glite_jp_db_execstmt()
 */
int glite_jp_db_fetchrow(glite_jp_db_stmt_t, char **);


/**
 * Retrieve column names of a query statement
 *
 * \param[inout] stmt	statement
 * \param[out] cols	result set column names. Expects allocated array.
 *
 * \return	0 if OK, nonzero on error
 */
int glite_jp_db_querycolumns(glite_jp_db_stmt_t, char **);


/**
 * Free the statement structure
 *
 * \param[inout] stmt	statement
 */
void glite_jp_db_freestmt(glite_jp_db_stmt_t *);


/** 
 * Convert time_t into database-specific time string.
 *
 * \param[in] t	the converted time
 * \return	XXX: pointer to static area that is changed by subsequent calls
 */
char *glite_jp_db_timetodb(time_t);


/** 
 * Convert database-specific time string into time_t.
 *
 * \param[in] t	the converted string
 *
 * \return	result time
 */
time_t glite_jp_db_dbtotime(const char *);


/**
 * Check database version.
 *
 * \param[inout] ctx	context to work with
 *
 * \return JP error code
 */
int glite_jp_db_dbcheckversion(glite_jp_context_t);


/**
 * Assign parameters to mysql bind structure.
 *
 * \param[inout] param	mysql bind strusture array
 * \param[in] type	mysql type
 *
 * Variable parameters:
 * 	MYSQL_TYPE_TINY:	char *buffer
 *	MYSQL_TYPE_LONG:	long int *buffer
 *	MYSQL_TYPE_*_BLOB:	void *buffer, unsigned long *length
 *	MYSQL_TYPE_*STRING:	char *buffer, unsigned long *length
 *	MYSQL_TYPE_NULL:	-
 */
void glite_jp_db_assign_param(MYSQL_BIND *param, enum enum_field_types type, ...);


/**
 * Assign result variables to mysql bind structure.
 *
 * \param[inout] result	mysql bind strusture array
 * \param[in] type	mysql type
 * \param[in] is_null	pointer to is_null boolean
 *
 * Variable parameters:
 * 	MYSQL_TYPE_TINY:	char *buffer
 *	MYSQL_TYPE_LONG:	long int *buffer
 *	MYSQL_TYPE_*_BLOB:	void *buffer, unsigned long max_length, unsigned long *length
 *	MYSQL_TYPE_*STRING:	char *buffer, unsigned long max_length, unsigned long *length
 */
void glite_jp_db_assign_result(MYSQL_BIND *result, enum enum_field_types type, my_bool *is_null, ...);

/**
 * Prepare the SQL statement. Use glite_jp_db_freestmt() to free it.
 *
 * \param[inout] ctx	context to work with
 * \param[in] sql	SQL command
 * \param[out] jpstmt	returned JP SQL statement
 * \param[inout] params	mysql static structure with parameters
 * \param[inout] cols	mysql static structure with result buffer
 *
 * \return JP error code
 */
int glite_jpis_db_prepare(glite_jp_context_t ctx, const char *sql, glite_jp_db_stmt_t *jpstmt, MYSQL_BIND *params, MYSQL_BIND *cols);

/**
 * Execute prepared SQL statement.
 *
 * \param[inout] jpstmt	JP SQL statement
 *
 * \return	number of affected rows, -1 on error
 */
int glite_jp_db_execute(glite_jp_db_stmt_t jpstmt);

/**
 *
 * \param[inout] jpstmt	JP SQL statement
 *
 * \return JP error code (ENODATA when no more row are available)
 */
int glite_jp_db_fetch(glite_jp_db_stmt_t jpstmt);

#ifdef __cplusplus
}
#endif

#endif
