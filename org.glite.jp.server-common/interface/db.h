#ifndef _DB_H
#define _DB_H

#ident "$Header$"

#include <sys/types.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <glite/jp/types.h>
#include <glite/jp/context.h>

typedef struct _glite_jp_db_stmt_t *glite_jp_db_stmt_t;

typedef enum {
	GLITE_JP_DB_TYPE_NULL = 0,
	GLITE_JP_DB_TYPE_TINYINT = 1,
	GLITE_JP_DB_TYPE_INT = 2,
	GLITE_JP_DB_TYPE_TINYBLOB = 3,
	GLITE_JP_DB_TYPE_TINYTEXT = 4,
	GLITE_JP_DB_TYPE_BLOB = 5,
	GLITE_JP_DB_TYPE_TEXT = 6,
	GLITE_JP_DB_TYPE_MEDIUMBLOB = 7,
	GLITE_JP_DB_TYPE_MEDIUMTEXT = 8,
	GLITE_JP_DB_TYPE_LONGBLOB = 9,
	GLITE_JP_DB_TYPE_LONGTEXT = 10,
	GLITE_JP_DB_TYPE_VARCHAR = 11,
	GLITE_JP_DB_TYPE_CHAR = 12,
	GLITE_JP_DB_TYPE_DATE = 13,
	GLITE_JP_DB_TYPE_TIME = 14,
	GLITE_JP_DB_TYPE_DATETIME = 15,
	GLITE_JP_DB_TYPE_TIMESTAMP = 16,
	GLITE_JP_DB_TYPE_LAST = 17
} glite_jp_db_type_t;

/**
 * Connect to the database.
 *
 * \param[inout] cxt	context to work with
 * \param[in] cs	connect string user/password@host:database
 *
  \return	JP error code
 */
int glite_jp_db_connect(glite_jp_context_t, const char *);


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
 * Free the statement structure and destroy its parameters.
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
 * Create and assign parameters for mysql prepared commands.
 *
 * \param[out] params	internal structure array
 *
 * Variable parameters:
 * always:
 * 	glite_jp_db_type_t type		DB item type
 * then one of them:
 * 	GLITE_JP_DB_TYPE_TINYINT:	char *buffer
 *	GLITE_JP_DB_TYPE_INT:	int *buffer
 *	GLITE_JP_DB_TYPE_*BLOB/TEXT:	void *buffer, unsigned long *length
 *	GLITE_JP_DB_TYPE_[VAR]CHAR:	char *buffer, unsigned long *length
 *	GLITE_JP_DB_TYPE_DATE:	void **buffer
 *	GLITE_JP_DB_TYPE_TIME:	void **buffer
 *	GLITE_JP_DB_TYPE_DATETIME:	void **buffer
 *	GLITE_JP_DB_TYPE_TIMESTAMP:	void **buffer
 *	GLITE_JP_DB_TYPE_NULL:	-
 */
void glite_jp_db_create_params(void **params, int n, ...);

/**
 * Create and assign result variables for mysql prepared commands.
 *
 * \param[inout] result	mysql bind strusture array
 *
 * Variable parameters:
 * always:
 * \param[in] glite_jp_db_type_t type	DB item type
 * \param[in] int *is_null	pointer to is_null boolean or NULL
 * then one of them:
 * 	GLITE_JP_DB_TYPE_TINYINT:	char *buffer
 *	GLITE_JP_DB_TYPE_INT:		long int *buffer
 *	GLITE_JP_DB_TYPE_*BLOB/TEXT:	void *buffer, unsigned long max_length, unsigned long *length
 *	GLITE_JP_DB_TYPE_[VAR]CHAR:	char *buffer, unsigned long max_length, unsigned long *length
 *	GLITE_JP_DB_TYPE_DATE:		void **buffer
 *	GLITE_JP_DB_TYPE_TIME:		void **buffer
 *	GLITE_JP_DB_TYPE_DATETIME:	void **buffer
 *	GLITE_JP_DB_TYPE_TIMESTAMP:	void **buffer
 */
void glite_jp_db_create_results(void **results, int n, ...);

/**
 * Destroy prepared parameters.
 */
void glite_jp_db_destroy_params(void *params);

/**
 * Destroy prepared results.
 */
void glite_jp_db_destroy_results(void *results);

#if 0
void glite_jp_db_assign_param(MYSQL_BIND *param, enum enum_field_types type, ...);
void glite_jp_db_assign_result(MYSQL_BIND *result, enum enum_field_types type, my_bool *is_null, ...);
#endif

/**
 * Assign time_t to buffer.
 */
void glite_jp_db_set_time(void *buffer, const time_t time);

/**
 * Get the time from buffer.
 */
time_t glite_jp_db_get_time(const void *buffer);

/**
 * Rebind the parameters and/or results.
 *
 * \param[inout] jpstmt	JP SQL statement to work with
 * \param[inout] params	mysql static structure with parameters or NULL
 * \param[inout] cols	mysql static structure with result buffer or NULL
 *
 * \return JP error code
 */
int glite_jp_db_rebind(glite_jp_db_stmt_t jpstmt, void *params, void *cols);

/**
 * Prepare the SQL statement. Use glite_jp_db_freestmt() to free it.
 *
 * \param[inout] ctx	context to work with
 * \param[in] sql	SQL command
 * \param[out] jpstmt	returned JP SQL statement
 * \param[inout] params	mysql static structure with parameters or NULL
 * \param[inout] cols	mysql static structure with result buffer or NULL
 *
 * \return JP error code
 */
int glite_jp_db_prepare(glite_jp_context_t ctx, const char *sql, glite_jp_db_stmt_t *jpstmt, void *params, void *cols);

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
