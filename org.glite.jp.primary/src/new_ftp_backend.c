#ident "$Header$"

#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/strmd5.h"

#include "tags.h"
#include "backend.h"
#include "db.h"

#include "jpps_H.h"	/* XXX: SOAP_TYPE___jpsrv__GetJob */

#include "jptype_map.h"

#define FTPBE_DEFAULT_DB_CS	"jpps/@localhost:jpps"

struct ftpbe_config {
	char *internal_path;
	char *external_path;
	char *db_cs;
	char *gridmap;
	char *logname;
};

static struct ftpbe_config *config = NULL;

struct fhandle_rec {
	int fd;
	int fd_append;
};
typedef struct fhandle_rec *fhandle;

static struct option ftpbe_opts[] = {
	{ "ftp-internal-path", 1, NULL, 'I' },
	{ "ftp-external-path", 1, NULL,	'E' },
	{ "ftp-db-cs",	       1, NULL,	'D' },
	{ "ftp-gridmap",       1, NULL,	'G' },
	{ NULL,                0, NULL,  0  }
};

/*******************************************************************************
	Internal helpers
*******************************************************************************/


static int config_check(
	glite_jp_context_t ctx,
	struct ftpbe_config *config)
{
	return config == NULL ||
		config->internal_path == NULL ||
		config->external_path == NULL ||
		config->db_cs == NULL ||
		config->gridmap == NULL ||
		config->logname == NULL;

	/* XXX check reality */
}

static int jobid_unique_pathname(glite_jp_context_t ctx, const char *job, 
			  char **unique, char **ju_path, int get_path)
{
	char *p;
	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	p = strrchr(job, '/');
	if (!p) {
		err.code = EINVAL;
		err.desc = "Malformed jobid";
		return glite_jp_stack_error(ctx,&err);
	}
	/* XXX thorough checks */
	if (!(*unique = strdup(p+1))) {
		err.code = ENOMEM;
		return glite_jp_stack_error(ctx,&err);
	}
	if (get_path) {
		if (!(*ju_path = strdup(p+1))) {
			free(*unique);
			err.code = ENOMEM;
			return glite_jp_stack_error(ctx,&err);
		}
		*(*ju_path + 10) = '\0';
	}
	return 0;
}

static int mkdirpath(const char* path, int prefixlen)
{
	char *wpath, *p;
	int goout, ret;

	wpath = strdup(path);
	if (!wpath) {
		errno = ENOMEM;
		return -1;
	}

	p = wpath + prefixlen;
	goout = 0;
	while (!goout) {
		while (*p == '/') p++;
		while (*p != '/' && *p != '\0') p++;
		goout = (*p == '\0');
		*p = '\0';
		ret = mkdir(wpath, S_IRUSR | S_IWUSR | S_IXUSR);
		if (ret < 0 && errno != EEXIST) break;
		*p = '/';
	}
	free(wpath);
	return goout ? 0 : ret;
}

static int store_user(glite_jp_context_t ctx, const char *userid, const char *subj)
{
	glite_jp_error_t err;
	char *stmt;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;
	
	assert(userid != NULL);
	assert(subj != NULL);

	trio_asprintf(&stmt,"insert into users(userid,cert_subj) "
		"values ('%|Ss','%|Ss')",userid,subj);
	if (!stmt) {
		err.code = ENOMEM;
		return glite_jp_stack_error(ctx,&err);
	}

	if (glite_jp_db_execstmt(ctx, stmt, NULL) < 0) {
		if (ctx->error->code == EEXIST) 
			glite_jp_clear_error(ctx);
		else {
			free(stmt);
			err.code = EIO;
			err.desc = "DB access failed";
			return glite_jp_stack_error(ctx,&err);
		}
	}
	free(stmt);

	return 0;
}

static long regtime_trunc(long tv_sec)
{
	return tv_sec / (86400*7);
}

static long regtime_ceil(long tv_sec)
{
	return (tv_sec % (86400*7)) ? tv_sec/(86400*7)+1 : tv_sec/(86400*7) ;
}

/********************************************************************************
	Backend calls
********************************************************************************/
int glite_jppsbe_init(
	glite_jp_context_t ctx,
	int argc,
	char *argv[]
)
{
	glite_jp_error_t err;
	int opt;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	config = (struct ftpbe_config *) calloc(1, sizeof *config);
	if (!config) {
		err.code = ENOMEM;
		return glite_jp_stack_error(ctx,&err);
	}

	config->logname = getlogin();

	while ((opt = getopt_long(argc, argv, "I:E:G:", ftpbe_opts, NULL)) != EOF) {
		switch (opt) {
			case 'I': config->internal_path = optarg; break;
			case 'E': config->external_path = optarg; break;
			case 'D': config->db_cs = optarg; break;
			case 'G': config->gridmap = optarg; break;
			default: break;
		}
	}

	/* Defaults */
	if (!config->db_cs) config->db_cs = strdup(FTPBE_DEFAULT_DB_CS);

	if (config_check(ctx, config)) {
		err.code = EINVAL;
		err.desc = "Invalid FTP backend configuration";
		return glite_jp_stack_error(ctx,&err);
	}

	if (glite_jp_db_connect(ctx, config->db_cs)) {
		err.code = EIO;
		err.desc = "Cannot access backend's database (during init)";
		return glite_jp_stack_error(ctx,&err);
	} else {
		glite_jp_db_close(ctx); /* slaves open their own connections */
	}

	return 0;
}

int glite_jppsbe_init_slave(
	glite_jp_context_t ctx
)
{
	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;
	
	if (glite_jp_db_connect(ctx, config->db_cs)) {
		err.code = EIO;
		err.desc = "Cannot access backend's database";
		return glite_jp_stack_error(ctx,&err);
	} 

	return 0;
}

int glite_jppsbe_register_job(	
	glite_jp_context_t ctx,
	const char *job,
	const char *owner
)
{
	glite_jp_error_t err;
	char *data_dir = NULL;
	char *ju = NULL;
	char *ju_path = NULL;
	char *ownerhash = NULL;
	struct timeval reg_tv;
	char *stmt = NULL;
	char *dbtime = NULL;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	assert(job != NULL);
	assert(owner != NULL);

	gettimeofday(&reg_tv, NULL);

	if (jobid_unique_pathname(ctx, job, &ju, &ju_path, 1) != 0) {
		err.code = ctx->error->code;
		err.desc = "Cannot obtain jobid unique path/name";
		return glite_jp_stack_error(ctx,&err);
	}

	ownerhash = str2md5(owner); /* static buffer */
	if (store_user(ctx, ownerhash, owner)) {
		err.code = EIO;
		err.desc = "Cannot store user entry";
		goto error_out;
	}

	dbtime = glite_jp_db_timetodb(reg_tv.tv_sec);
	if (!dbtime) {
		err.code = ENOMEM;
		goto error_out;
	}

	trio_asprintf(&stmt,"insert into jobs(jobid,dg_jobid,owner,reg_time) "
		"values ('%|Ss','%|Ss','%|Ss', %s)",
		ju, job, ownerhash, dbtime);
	if (!stmt) {
		err.code = ENOMEM;
		goto error_out;
	}
	
	if (glite_jp_db_execstmt(ctx, stmt, NULL) < 0) {
		if (ctx->error->code == EEXIST) {
			err.code = EEXIST;
			err.desc = "Job already registered";
		}
		else {
			err.code = EIO;
			err.desc = "DB access failed";
		}
		goto error_out;
	}

	if (asprintf(&data_dir, "%s/data/%s/%d/%s",
			config->internal_path, ownerhash, regtime_trunc(reg_tv.tv_sec), ju) == -1) {
		err.code = ENOMEM;
		goto error_out;
	}
	if (mkdirpath(data_dir, strlen(config->internal_path)) < 0 &&
			errno != EEXIST) {
		err.code = errno;
		err.desc = "Cannot mkdir jobs's data directory";
		goto error_out;
	}

error_out:
	free(data_dir);
	free(stmt); free(dbtime);
	free(ju); free(ju_path);

	if (err.code) {
		return glite_jp_stack_error(ctx,&err);
	} else {
		return 0;
	}
}

static int add_to_gridmap(glite_jp_context_t ctx, const char *dn)
{
	FILE *gridmap = NULL;
	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	gridmap = fopen(config->gridmap, "a");
	if (!gridmap) {
		err.code = errno;
		err.desc = "Cannot open gridmap file";
		return glite_jp_stack_error(ctx,&err);
	}
	if (fprintf(gridmap, "\"%s\" %s\n", dn, config->logname) < 6 ||
		ferror(gridmap)) {
		err.code = EIO;
		err.desc = "Cannot write to gridmap file";
		fclose(gridmap);
		return glite_jp_stack_error(ctx,&err);
	}
	fclose(gridmap);
	return 0;
}

static int remove_from_gridmap(glite_jp_context_t ctx, const char *dn)
{
	FILE *gridmap = NULL;
	char *temp_name = NULL;
	FILE *temp_file = NULL;
	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;
	
	/* XXX */
	return 0;
}

int glite_jppsbe_start_upload(
	glite_jp_context_t ctx,
	const char *job,
	const char *class,
	const char *name, 	
	const char *content_type,
	char **destination_out,
	time_t *commit_before_inout
)
{
	char *data_basename = NULL;
	char *data_fname = NULL;
	char *ju = NULL;
	char *ju_path = NULL;
	char *peername = NULL;
	char *peerhash = NULL;

	char *stmt = NULL;
	glite_jp_db_stmt_t db_res;
	int db_retn;
	char *db_row[2] = { NULL, NULL };

	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	assert(job!=NULL);
	assert(destination_out!=NULL);

	assert(class!=NULL);

	if (jobid_unique_pathname(ctx, job, &ju, &ju_path, 1) != 0) {
		err.code = ctx->error->code;
		err.desc = "Cannot obtain jobid unique path/name";
		return glite_jp_stack_error(ctx,&err);
	}

	peername = glite_jp_peer_name(ctx);
	if (peername == NULL) {
		err.code = EINVAL;
		err.desc = "Cannot obtain client certificate info";
		goto error_out;
	}

	trio_asprintf(&stmt, "select owner, reg_time from jobs"
		" where jobid='%|Ss'", ju);

	if (!stmt) {
		err.code = ENOMEM;
		goto error_out;
	}

	if ((db_retn = glite_jp_db_execstmt(ctx, stmt, &db_res)) <= 0) {
		if (db_retn == 0) {
			err.code = ENOENT;
			err.desc = "No such job registered";
		} else {
			err.code = EIO;
			err.desc = "DB access failed";
		}
		goto error_out;
	}
	
	db_retn = glite_jp_db_fetchrow(db_res, db_row);
	if (db_retn != 2) {
		glite_jp_db_freestmt(&db_res);
		err.code = EIO;
		err.desc = "DB access failed";
		goto error_out;
	}

	glite_jp_db_freestmt(&db_res);
	
	/* XXX authorization done in soap_ops.c */

	/* XXX name length */
	if (asprintf(&data_basename, "%s%s%s", class,
		(name != NULL) ? "." : "",
		(name != NULL) ? name : "") == -1) {
		err.code = ENOMEM;
		goto error_out;
	}

	if (asprintf(&data_fname, "%s/data/%s/%d/%s/%s",
			config->internal_path, db_row[0],
			regtime_trunc(glite_jp_db_dbtotime(db_row[1])),
			ju, data_basename) == -1) {
		err.code = ENOMEM;
		goto error_out;
	}
	if (asprintf(destination_out, "%s/data/%s/%d/%s/%s",
			config->external_path, db_row[0],
			regtime_trunc(glite_jp_db_dbtotime(db_row[1])),
			ju, data_basename) == -1) {
		err.code = ENOMEM;
		goto error_out;
	}

	if (commit_before_inout != NULL)
	/* XXX no timeout enforced */
		/* XXX: gsoap does not like so much, one year should be enough
		*commit_before_inout = (time_t) LONG_MAX;
		*/
		*commit_before_inout = time(NULL) + 365*24*60*60;
	
	/* 
	if (add_to_gridmap(ctx, peername)) {
		err.code = EIO;
		err.desc = "Cannot add peer DN to ftp server authorization file";
		goto error_out;
	}
	*/

	peerhash = str2md5(peername); /* static buffer */
	if (store_user(ctx, peerhash, peername)) {
		err.code = EIO;
		err.desc = "Cannot store upload user entry";
		goto error_out;
	}

	free(stmt); stmt = NULL;
	trio_asprintf(&stmt,"insert into files"
		"(jobid,filename,int_path,ext_url,state,deadline,ul_userid) "
		"values ('%|Ss','%|Ss','%|Ss','%|Ss','%|Ss', '%|Ss', '%|Ss')",
		ju, data_basename, data_fname, *destination_out, "uploading", 
		glite_jp_db_timetodb(*commit_before_inout), peerhash);
	if (!stmt) {
		err.code = ENOMEM;
		goto error_out;
	}

	if (glite_jp_db_execstmt(ctx, stmt, NULL) < 0) {
		if (ctx->error->code == EEXIST) {
			err.code = EEXIST;
			err.desc = "File already stored or upload in progress";
		} else {
			err.code = EIO;
			err.desc = "DB access failed";
		}
		goto error_out;
	}
	
error_out:
	free(db_row[0]); free(db_row[1]);
	free(stmt);
	free(data_basename); 
	free(data_fname); 
	free(ju); free(ju_path);
	if (err.code) {
		return glite_jp_stack_error(ctx,&err);
	} else {
		return 0;
	}
}

int glite_jppsbe_commit_upload(
	glite_jp_context_t ctx,
	const char *destination
)
{
	char *peername = NULL;
	char *peerhash = NULL;

	char *stmt = NULL;
	glite_jp_db_stmt_t db_res;
	int db_retn;
	char *db_row[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	int i;
	
	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	assert(destination != NULL);

	trio_asprintf(&stmt, "select * from files where "
			     "ext_url='%|Ss' and state='uploading'", destination);
	if (!stmt) {
		err.code = ENOMEM;
		goto error_out;
	}
	
	if ((db_retn = glite_jp_db_execstmt(ctx, stmt, &db_res)) <= 0) {
		if (db_retn == 0) {
			err.code = ENOENT;
			err.desc = "No such upload in progress";
		} else {
			err.code = EIO;
			err.desc = "DB access failed";
		}
		goto error_out;
	}

	db_retn = glite_jp_db_fetchrow(db_res, db_row);
	if (db_retn != 7) {
		glite_jp_db_freestmt(&db_res);
		err.code = EIO;
		err.desc = "DB access failed";
		goto error_out;
	}
	glite_jp_db_freestmt(&db_res);

	peername = glite_jp_peer_name(ctx);
	if (peername == NULL) {
		err.code = EINVAL;
		err.desc = "Cannot obtain client certificate info";
		goto error_out;
	}

	peerhash = str2md5(peername); /* static buffer */
	if (strcmp(peerhash, db_row[6])) {
		err.code = EPERM;
		err.desc = "Upload started by client with different identity";
		goto error_out;
	}

	free(stmt);
	trio_asprintf(&stmt,"update files set state='committed', deadline=NULL "
		"where jobid='%|Ss' and filename='%|Ss'", db_row[0], db_row[1]);
	
	if (!stmt) {
		err.code = ENOMEM;
		goto error_out;
	}

	if (glite_jp_db_execstmt(ctx, stmt, NULL) < 0) {
		err.code = EIO;
		err.desc = "DB access failed";
		goto error_out;
	}
error_out:
	for (i=0; i<7; i++) free(db_row[i]);
	free(peername);
	free(stmt);
	if (err.code) {
		return glite_jp_stack_error(ctx,&err);
	} else {
		return 0;
	}
}

int glite_jppsbe_destination_info(
	glite_jp_context_t ctx,
	const char *destination,
	char **job,
	char **class,
	char **name
)
{
	char *stmt = NULL;
	glite_jp_db_stmt_t db_res;
	int db_retn;
	char *db_row[2] = { NULL, NULL};
	int i;
	char *cp = NULL;
	
	char *classname = NULL;
	glite_jp_error_t err;

	assert(destination != NULL);
	assert(job != NULL);
	assert(class != NULL);
	assert(name != NULL);

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	
	trio_asprintf(&stmt, "select j.dg_jobid,f.filename from jobs j,files f where "
			     "f.ext_url='%|Ss' and j.jobid=f.jobid", destination);
	if (!stmt) {
		err.code = ENOMEM;
		goto error_out;
	}
	
	if ((db_retn = glite_jp_db_execstmt(ctx, stmt, &db_res)) <= 0) {
		if (db_retn == 0) {
			err.code = ENOENT;
			err.desc = "Invalid destination string";
		} else {
			err.code = EIO;
			err.desc = "DB access failed";
		}
		goto error_out;
	}

	db_retn = glite_jp_db_fetchrow(db_res, db_row);
	if (db_retn != 2) {
		glite_jp_db_freestmt(&db_res);
		err.code = EIO;
		err.desc = "DB access failed";
		goto error_out;
	}
	glite_jp_db_freestmt(&db_res);

	*job = strdup(db_row[0]);

	cp = strchr(db_row[1],'.');
	if (!cp) { 
		*name = NULL;
	} else {
		*cp++ = '\0';
		*name = strdup(cp);
	}
	*class = strdup(db_row[1]);

	if (!*job || !*class)  {
		err.code = ENOMEM;
		goto error_out;
	}

error_out:
	for (i=0; i<2; i++) free(db_row[i]);
	free(stmt);
	if (err.code) {
		return glite_jp_stack_error(ctx,&err);
	} else {
		return 0;
	}
}


int glite_jppsbe_get_job_url(
	glite_jp_context_t ctx,
	const char *job,
	const char *class,
	const char *name, 	
	char **url_out
)
{
	char *data_basename = NULL;
	char *data_fname = NULL;
	char *ju = NULL;
	char *ju_path = NULL;

	char *stmt = NULL;
	glite_jp_db_stmt_t db_res;
	int db_retn;
	char *db_row[3] = { NULL, NULL, NULL };

	long reg_time;
	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	assert(job!=NULL);
	assert(url_out != NULL);

	assert(class!=NULL);

	if (jobid_unique_pathname(ctx, job, &ju, &ju_path, 1) != 0) {
		err.code = ctx->error->code;
		err.desc = "Cannot obtain jobid unique path/ : ""name";
		return glite_jp_stack_error(ctx,&err);
	}

	trio_asprintf(&stmt, "select j.owner,reg_time,u.cert_subj from jobs j, users u "
		"where j.jobid='%|Ss' and j.owner = u.userid", ju);

	if (!stmt) {
		err.code = ENOMEM;
		goto error_out;
	}

	if ((db_retn = glite_jp_db_execstmt(ctx, stmt, &db_res)) <= 0) {
		if (db_retn == 0) {
			err.code = ENOENT;
			err.desc = "No such job registered";
		} else {
			err.code = EIO;
			err.desc = "DB access failed";
		}
		goto error_out;
	}

	free(stmt); stmt = NULL;
	
	db_retn = glite_jp_db_fetchrow(db_res, db_row);
	if (db_retn != 3) {
		glite_jp_db_freestmt(&db_res);
		err.code = EIO;
		err.desc = "DB access failed";
		goto error_out;
	}

	glite_jp_db_freestmt(&db_res);

	if (glite_jpps_authz(ctx,SOAP_TYPE___jpsrv__GetJob,job,db_row[2])) {
		err.code = EPERM;
		goto error_out;
	}
	
	/* XXX name length */
	if (asprintf(&data_basename, "%s%s%s", class,
		(name != NULL) ? "." : "",
		(name != NULL) ? name : "") == -1) {
		err.code = ENOMEM;
		goto error_out;
	}

	if (asprintf(url_out, "%s/data/%s/%d/%s/%s",
			config->external_path, db_row[0],
			regtime_trunc(glite_jp_db_dbtotime(db_row[1])),
			ju, data_basename) == -1) {
		err.code = ENOMEM;
		goto error_out;
	}

	trio_asprintf(&stmt,"select 'x' from files where jobid='%|Ss' "
				"and ext_url = '%|Ss' "
				"and state='committed' ",ju,*url_out);

	if ((db_retn = glite_jp_db_execstmt(ctx,stmt,&db_res)) <= 0) {
		if (db_retn == 0) {
			err.code = ENOENT;
			err.desc = "not uploaded yet";
		}
		else {
			err.code = EIO;
			err.desc = "DB access failed";
		}
		/* goto error_out; */
	}

error_out:
	free(db_row[0]); free(db_row[1]);
	free(stmt);
	free(data_basename);
	free(ju); free(ju_path);
	if (err.code) {
		return glite_jp_stack_error(ctx,&err);
	} else {
		return 0;
	}
}

static int get_job_fname(
	glite_jp_context_t ctx,
	const char *job,
	const char *class,
	const char *name, 	
	char **fname_out
)
{
	char *data_basename = NULL;
	char *ju = NULL;
	char *ju_path = NULL;

	char *stmt = NULL;
	glite_jp_db_stmt_t db_res;
	int db_retn;
	char *db_row[2] = { NULL, NULL };

	long reg_time;
	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	assert(job!=NULL);
	assert(fname_out != NULL);

	assert(class!=NULL);

	if (jobid_unique_pathname(ctx, job, &ju, &ju_path, 1) != 0) {
		err.code = ctx->error->code;
		err.desc = "Cannot obtain jobid unique path/name";
		return glite_jp_stack_error(ctx,&err);
	}

	trio_asprintf(&stmt, "select owner, reg_time from jobs "
		"where jobid='%|Ss'", ju);

	if (!stmt) {
		err.code = ENOMEM;
		goto error_out;
	}

	if ((db_retn = glite_jp_db_execstmt(ctx, stmt, &db_res)) <= 0) {
		if (db_retn == 0) {
			err.code = ENOENT;
			err.desc = "No such job registered";
		} else {
			err.code = EIO;
			err.desc = "DB access failed";
		}
		goto error_out;
	}
	
	db_retn = glite_jp_db_fetchrow(db_res, db_row);
	if (db_retn != 2) {
		glite_jp_db_freestmt(&db_res);
		err.code = EIO;
		err.desc = "DB access failed";
		goto error_out;
	}

	glite_jp_db_freestmt(&db_res);
	
	/* XXX name length */
	if (asprintf(&data_basename, "%s%s%s", class,
		(name != NULL) ? "." : "", (name != NULL) ? name : "") == -1) {
		err.code = ENOMEM;
		goto error_out;
	}

	if (asprintf(fname_out, "%s/data/%s/%d/%s/%s",
			config->internal_path, db_row[0],
			regtime_trunc(glite_jp_db_dbtotime(db_row[1])),
			ju, data_basename) == -1) {
		err.code = ENOMEM;
		goto error_out;
	}

error_out:
	free(db_row[0]); free(db_row[1]);
	free(stmt);
	free(data_basename);
	free(ju); free(ju_path);
	if (err.code) {
		return glite_jp_stack_error(ctx,&err);
	} else {
		return 0;
	}
}


int glite_jppsbe_open_file(
	glite_jp_context_t ctx,
	const char *job,
	const char *class,
	const char *name,
	int mode,
	void **handle_out
)
{
	fhandle	handle = NULL;
	char* fname = NULL;
	glite_jp_error_t err;

	assert(handle_out != NULL);

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	if (get_job_fname(ctx, job, class, name, &fname)) {
		err.code = ctx->error->code;
		err.desc = "Cannot construct internal filename";
		return glite_jp_stack_error(ctx,&err);
	}

	handle = (fhandle) calloc(1,sizeof(*handle));
	if (handle == NULL) {
		err.code = ENOMEM;
		goto error_out;
	}

	handle->fd = open(fname, mode, S_IRUSR | S_IWUSR);
	if (handle->fd < 0) {
		err.code = errno;
		err.desc = "Cannot open requested file";
		free(handle);
		goto error_out;
	}
	handle->fd_append = open(fname, mode | O_APPEND, S_IRUSR | S_IWUSR);
	if (handle->fd_append < 0) {
		err.code = errno;
		err.desc = "Cannot open requested file for append";
		close(handle->fd);
		free(handle);
		goto error_out;
	}
	*handle_out = (void*) handle;

error_out:
	free(fname);
	if (err.code) { 
		return glite_jp_stack_error(ctx,&err);
	} else { 
		return 0;
	}
}

int glite_jppsbe_close_file(
	glite_jp_context_t ctx,
	void *handle
)
{
	glite_jp_error_t err;

	assert(handle != NULL);

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	if (close(((fhandle)handle)->fd_append) < 0) {
		err.code = errno;
		err.desc = "Error closing file descriptor (fd_append)";
		goto error_out;
	}
	if (close(((fhandle)handle)->fd) < 0) {
		err.code = errno;
		err.desc = "Error closing file descriptor";
		goto error_out;
	}

error_out:
	free(handle);
	if (err.code) {
		return glite_jp_stack_error(ctx,&err);
	} else { 
		return 0;
	}
}

int glite_jppsbe_pread(
	glite_jp_context_t ctx,
	void *handle,
	void *buf,
	size_t nbytes,
	off_t offset,
	ssize_t *nbytes_ret
)
{
	ssize_t ret;
	glite_jp_error_t err;

	assert(handle != NULL);
	assert(buf != NULL);

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	if ((ret = pread(((fhandle)handle)->fd, buf, nbytes, offset)) < 0) {
		err.code = errno;
		err.desc = "Error in pread()";
		return glite_jp_stack_error(ctx,&err);
	}
	*nbytes_ret = ret;

	return 0;
}

int glite_jppsbe_pwrite(
	glite_jp_context_t ctx,
	void *handle,
	void *buf,
	size_t nbytes,
	off_t offset
)
{
	glite_jp_error_t err;

	assert(handle != NULL);
	assert(buf != NULL);

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	if (pwrite(((fhandle)handle)->fd, buf, nbytes, offset) < 0) {
		err.code = errno;
		err.desc = "Error in pwrite()";
		return glite_jp_stack_error(ctx,&err);
	}

	return 0;
}

int glite_jppsbe_append(
	glite_jp_context_t ctx,
	void *handle,
	void *buf,
	size_t nbytes
)
{
	glite_jp_error_t err;

	assert(handle != NULL);
	assert(buf != NULL);

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	if (write(((fhandle)handle)->fd_append, buf, nbytes) < 0) {
		err.code = errno;
		err.desc = "Error in write()";
		return glite_jp_stack_error(ctx,&err);
	}

	return 0;
}

static int get_job_info(
	glite_jp_context_t ctx,
	const char *job,
	char **owner,
	struct timeval *tv_reg
)
{
	char *ju = NULL;
	char *ju_path = NULL;
	FILE *regfile = NULL;
	long reg_time_sec;
	long reg_time_usec;
	int ownerlen = 0;
	int info_version;
	char *int_fname = NULL;
	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	if (jobid_unique_pathname(ctx, job, &ju, &ju_path, 1) != 0) {
		err.code = ctx->error->code;
		err.desc = "Cannot obtain jobid unique path/name";
		return glite_jp_stack_error(ctx,&err);
	}

	if (asprintf(&int_fname, "%s/regs/%s/%s.info",
			config->internal_path, ju_path, ju) == -1) {
		err.code = ENOMEM;
		goto error_out;
	}
	regfile = fopen(int_fname, "r");
	if (regfile == NULL) {
		err.code = errno;
		if (errno == ENOENT) 
			err.desc = "Job not registered";
		else
			err.desc = "Cannot open jobs's reg info file";
		goto error_out;
	}
	if (fscanf(regfile, "%d %ld.%ld %*s %*s %d ", &info_version,
		&reg_time_sec, &reg_time_usec, &ownerlen) < 4 || ferror(regfile)) {
		fclose(regfile);
		err.code = errno;
		err.desc = "Cannot read jobs's reg info file";
		goto error_out;
	}
	if (ownerlen) {
		*owner = (char *) calloc(1, ownerlen+1);
		if (!*owner) {
			err.code = ENOMEM;
			goto error_out;
		}
		if (fgets(*owner, ownerlen+1, regfile) == NULL) {
			fclose(regfile);
			free(*owner);
			err.code = errno;
			err.desc = "Cannot read jobs's reg info file";
			goto error_out;
		}
	}
	fclose(regfile);

	tv_reg->tv_sec = reg_time_sec;
	tv_reg->tv_usec = reg_time_usec;

error_out:
	free(int_fname);
	free(ju);
	free(ju_path);
	if (err.code) {
		return glite_jp_stack_error(ctx,&err);
	} else { 
		return 0;
	}
}

static int get_job_info_int(
	glite_jp_context_t ctx,
	const char *int_fname,
	char **jobid,
	char **owner,
	struct timeval *tv_reg
)
{
	FILE *regfile = NULL;
	long reg_time_sec;
	long reg_time_usec;
	int ownerlen = 0;
	int info_version;
	char jobid_buf[256];
	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	regfile = fopen(int_fname, "r");
	if (regfile == NULL) {
		err.code = errno;
		err.desc = "Cannot open jobs's reg info file";
		goto error_out;
	}
	if (fscanf(regfile, "%d %ld.%ld %s %*s %d ", &info_version,
		&reg_time_sec, &reg_time_usec, jobid_buf, &ownerlen) < 5 || ferror(regfile)) {
		fclose(regfile);
		err.code = errno;
		err.desc = "Cannot read jobs's reg info file";
		goto error_out;
	}
	*jobid = strdup(jobid_buf);
	if (ownerlen) {
		*owner = (char *) calloc(1, ownerlen+1);
		if (!*owner) {
			err.code = ENOMEM;
			goto error_out;
		}
		if (fgets(*owner, ownerlen+1, regfile) == NULL) {
			fclose(regfile);
			free(*owner);
			err.code = errno;
			err.desc = "Cannot read jobs's reg info file";
			goto error_out;
		}
	}
	fclose(regfile);

	tv_reg->tv_sec = reg_time_sec;
	tv_reg->tv_usec = reg_time_usec;

error_out:
	if (err.code) {
		return glite_jp_stack_error(ctx,&err);
	} else { 
		return 0;
	}
}

int glite_jppsbe_get_job_metadata(
	glite_jp_context_t ctx,
	const char *job,
	glite_jp_attrval_t attrs_inout[]
)
{
	int got_info = 0;
	struct timeval tv_reg;
	char *owner = NULL;
	int got_tags = 0;
	void *tags_handle = NULL;
	glite_jp_tagval_t* tags = NULL;
	int i,j;
	glite_jp_error_t err;

	assert(job != NULL);
	assert(attrs_inout != NULL);

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	for (i = 0; attrs_inout[i].attr.type != GLITE_JP_ATTR_UNDEF; i++) {
		switch (attrs_inout[i].attr.type) {
		case GLITE_JP_ATTR_OWNER:

/* must be implemented via filetype plugin
		case GLITE_JP_ATTR_TIME:
*/
			if (!got_info) {
				if (get_job_info(ctx, job, &owner, &tv_reg)) {
					err.code = ctx->error->code;
					err.desc = "Cannot retrieve job info";
					goto error_out;
				}
				got_info = 1;
			}
			break;

/* must be implemented via filetype plugin
		case GLITE_JP_ATTR_TAG:
			if (!got_tags) {
				if (glite_jppsbe_open_file(ctx, job, GLITE_JP_FILECLASS_TAGS,
					O_RDONLY, &tags_handle)) {
					err.code = ctx->error->code;
					err.desc = "Cannot open tag file";
					goto error_out;
				}
				if (glite_jpps_tag_readall(ctx, tags_handle, &tags)) {
					err.code = ctx->error->code;
					err.desc = "Cannot read tags";
					glite_jppsbe_close_file(ctx, tags_handle);
					goto error_out;
				}
				glite_jppsbe_close_file(ctx, tags_handle);
				got_tags = 1;
			}
			break;
*/
		default:
			err.code = EINVAL;
			err.desc = "Invalid attribute type";
			goto error_out;
			break;
		}

		switch (attrs_inout[i].attr.type) {
		case GLITE_JP_ATTR_OWNER:
			attrs_inout[i].value.s = strdup(owner);
			if (!attrs_inout[i].value.s) {
				err.code = ENOMEM;
				err.desc = "Cannot copy owner string";
				goto error_out;
			}	
			break;
		case GLITE_JP_ATTR_TIME:
			attrs_inout[i].value.time = tv_reg;
			break;

/* must be implemented via filetype plugin
		case GLITE_JP_ATTR_TAG:
			for (j = 0; tags[j].name != NULL; j++) {
				if (!strcmp(tags[j].name, attrs_inout[i].attr.name)) {
					if (glite_jpps_tagval_copy(ctx, &tags[j],
						&attrs_inout[i].value.tag)) {
						err.code = ENOMEM;
						err.desc = "Cannot copy tag value";
						goto error_out;
					}
					break;
				}
			}
			if (!tags[j].name) attrs_inout[i].value.tag.name = NULL;
			break;
*/
		default:
			break;
		}
	}

error_out:
	free(owner);
	if (tags) for (j = 0; tags[j].name != NULL; j++) {
		free(tags[j].name);
		free(tags[j].value);
	}
	free(tags);
	
	if (err.code) {
		while (i > 0) {
			i--;
			switch (attrs_inout[i].attr.type) {
			case GLITE_JP_ATTR_OWNER:
				free(attrs_inout[i].value.s);
				break;
			case GLITE_JP_ATTR_TAG:
				free(attrs_inout[i].value.tag.name);
				free(attrs_inout[i].value.tag.value);
			default:
				break;
			}
		}
		return glite_jp_stack_error(ctx,&err);
	} else {
		return 0;
	}
}
static int compare_timeval(struct timeval a, struct timeval b)
{
	if (a.tv_sec < b.tv_sec) return -1;
	if (a.tv_sec > b.tv_sec) return 1;
	if (a.tv_usec < b.tv_usec) return -1;
	if (a.tv_usec > b.tv_usec) return 1;
	return 0;
}


/* FIXME: disabled -- clarification wrt. filetype plugin needed */

#if 0

static int query_phase2(
	glite_jp_context_t ctx,
	const char *ownerhash,
	long regtime_tr,
	int q_tags,
	int md_tags,
	const glite_jp_query_rec_t query[],
	glite_jp_attrval_t metadata[],
	int (*callback)(
		glite_jp_context_t ctx,
		const char *job,
		const glite_jp_attrval_t metadata[]
	)
);

static int query_phase2(
	glite_jp_context_t ctx,
	const char *ownerhash,
	long regtime_tr,
	int q_tags,
	int md_tags,
	const glite_jp_query_rec_t query[],
	glite_jp_attrval_t metadata[],
	int (*callback)(
		glite_jp_context_t ctx,
		const char *job,
		const glite_jp_attrval_t metadata[]
	)
)
{
	char *time_dirname = NULL;
	DIR *time_dirp = NULL;
	struct dirent *jobent;
	char *info_fname = NULL;
	char *jobid = NULL;
	char *owner = NULL;
	struct timeval tv_reg;
	void *tags_handle = NULL;
	int matching;
	int i, j;
	glite_jp_tagval_t* tags = NULL;
	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	if (asprintf(&time_dirname, "%s/data/%s/%d", config->internal_path,
			ownerhash, regtime_tr) == -1) {
		err.code = ENOMEM;
		return glite_jp_stack_error(ctx,&err);
	}
	time_dirp = opendir(time_dirname);
	if (!time_dirp) {
		free(time_dirname);
		return 0; /* found nothing */
	}
	while ((jobent = readdir(time_dirp)) != NULL) {
		if (!strcmp(jobent->d_name, ".")) continue;
		if (!strcmp(jobent->d_name, "..")) continue;
		if (asprintf(&info_fname, "%s/%s/_info", time_dirname,
				jobent->d_name) == -1) {
			err.code = ENOMEM;
			goto error_out;
		}
		if (get_job_info_int(ctx, info_fname, &jobid, &owner,  &tv_reg)) {
			err.code = EIO;
			err.desc = "Cannot retrieve job info";
			goto error_out;
		}
		if (q_tags || md_tags) {
			if (glite_jppsbe_open_file(ctx, jobid, GLITE_JP_FILECLASS_TAGS,
				O_RDONLY, &tags_handle)) {
				err.code = ctx->error->code;
				err.desc = "Cannot open tag file";
				goto error_out;
			}
			if (glite_jpps_tag_readall(ctx, tags_handle, &tags)) {
				err.code = ctx->error->code;
				err.desc = "Cannot read tags";
				glite_jppsbe_close_file(ctx, tags_handle);
				goto error_out;
			}
			glite_jppsbe_close_file(ctx, tags_handle);
			tags_handle = NULL;
		}

		matching = 1;
		for (i = 0; matching && query[i].attr.type != GLITE_JP_ATTR_UNDEF; i++) {
			switch (query[i].attr.type) {
			case GLITE_JP_ATTR_OWNER:
				if (query[i].value.s == NULL || 
					strcmp(query[i].value.s, owner)) matching = 0;
				break;
			case GLITE_JP_ATTR_TIME:
				switch (query[i].op) {
					case GLITE_JP_QUERYOP_EQUAL:
						matching = !compare_timeval(tv_reg, query[i].value.time);
						break;
					case GLITE_JP_QUERYOP_UNEQUAL:
						matching = compare_timeval(tv_reg, query[i].value.time);
						break;
					case GLITE_JP_QUERYOP_LESS:
						matching = compare_timeval(tv_reg, query[i].value.time) < 0;
						break;
					case GLITE_JP_QUERYOP_GREATER:
						matching = compare_timeval(tv_reg, query[i].value.time) > 0;
						break;
					case GLITE_JP_QUERYOP_WITHIN:
						matching = compare_timeval(tv_reg, query[i].value.time) >= 0
							&& compare_timeval(tv_reg, query[i].value2.time) <= 0;
						break;
				}
				break;
			case GLITE_JP_ATTR_TAG:
				if (!tags) {
					matching = 0;
					break;
				}
				for (j = 0; tags[j].name != NULL; j++) {
					if (!strcmp(tags[j].name, query[i].attr.name)) {
						switch (query[i].op) {
						case GLITE_JP_QUERYOP_EQUAL:
							matching = !strcmp(tags[j].value, query[i].value.s);
							break;
						case GLITE_JP_QUERYOP_UNEQUAL:
							matching = strcmp(tags[j].value, query[i].value.s);
							break;
						case GLITE_JP_QUERYOP_LESS:
							matching = strcmp(tags[j].value, query[i].value.s) < 0;
							break;
						case GLITE_JP_QUERYOP_GREATER:
							matching = strcmp(tags[j].value, query[i].value.s) > 0;
							break;
						case GLITE_JP_QUERYOP_WITHIN:
							matching = strcmp(tags[j].value, query[i].value.s) >= 0 \
								&& strcmp(tags[j].value, query[i].value2.s) <= 0 ;
							break;
						default:
							break;
						}
					}
				}
				break;
			default:
				break;
			}
		}
		if (!matching) {
			free(info_fname); info_fname = NULL;
			free(jobid); jobid = NULL;
			if (tags) for (j = 0; tags[j].name != NULL; j++) {
				free(tags[j].name);
				free(tags[j].value);
			}
			free(tags); tags = NULL;
			continue;
		}

		for (i = 0; metadata[i].attr.type != GLITE_JP_ATTR_UNDEF; i++) {
			switch (metadata[i].attr.type) {
			case GLITE_JP_ATTR_OWNER:
				metadata[i].value.s = owner;
				break;
			case GLITE_JP_ATTR_TIME:
				metadata[i].value.time = tv_reg;
				break;
			case GLITE_JP_ATTR_TAG:
				for (j = 0; tags[j].name != NULL; j++) {
					if (!strcmp(tags[j].name, metadata[i].attr.name)) {
						if (glite_jpps_tagval_copy(ctx, &tags[j],
							&metadata[i].value.tag)) {
							err.code = ENOMEM;
							err.desc = "Cannot copy tag value";
							goto error_out;
						}
						break;
					}
				}
				if (!tags[j].name) {
					metadata[i].value.tag.name = NULL;
					metadata[i].value.tag.value = NULL;
				}
				break;
			default:
				break;
			}
		}
		(*callback)(ctx, jobid, metadata);
		free(jobid); jobid = NULL;
		while (i > 0) {
			i--;
			switch (metadata[i].attr.type) {
			case GLITE_JP_ATTR_TAG:
				free(metadata[i].value.tag.name);
				free(metadata[i].value.tag.value);
			default:
				break;
			}
		}
	}

error_out:
	if (tags) for (j = 0; tags[j].name != NULL; j++) {
		free(tags[j].name);
		free(tags[j].value);
	}
	if (tags_handle) glite_jppsbe_close_file(ctx, tags_handle);
	free(info_fname);
	free(owner);
	free(jobid);
	closedir(time_dirp);
	free(time_dirname);
	if (err.code)  {
		while (i > 0) {
			i--;
			switch (metadata[i].attr.type) {
			case GLITE_JP_ATTR_TAG:
				free(metadata[i].value.tag.name);
				free(metadata[i].value.tag.value);
			default:
				break;
			}
		}
		return glite_jp_stack_error(ctx,&err);
	} else
		return 0;
}

int glite_jppsbe_query(
	glite_jp_context_t ctx,
	const glite_jp_query_rec_t query[],
	const glite_jp_attrval_t metadata[],
	int (*callback)(
		glite_jp_context_t ctx,
		const char *job,
		const glite_jp_attrval_t metadata[]
	)
)
{
	/* XXX clone metadata */
	int i;
	char *q_exact_owner = NULL;
	char *ownerhash = NULL;
	long q_min_time = 0;
	long q_max_time = LONG_MAX;
	long q_min_time_tr;
	long q_max_time_tr;
	int q_with_tags = 0;
	int md_info = 0;
	int md_tags = 0;
	char *owner_dirname = NULL;
	DIR *owner_dirp = NULL;
	struct dirent *ttimeent;
	char *data_dirname = NULL;
	DIR *data_dirp = NULL;
	struct dirent *ownerent;
	long ttime = 0;
	glite_jp_attrval_t *metadata_templ = NULL;
	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	for (i = 0; query[i].attr.type != GLITE_JP_ATTR_UNDEF; i++) {
		if (query[i].attr.type == GLITE_JP_ATTR_OWNER && query[i].op == GLITE_JP_QUERYOP_EQUAL) {
			q_exact_owner = query[i].value.s;
		}
		if (query[i].attr.type == GLITE_JP_ATTR_TIME) {
			switch (query[i].op) {
			case GLITE_JP_QUERYOP_EQUAL:
				q_min_time = query[i].value.time.tv_sec;
				q_max_time = query[i].value.time.tv_sec + 1;
				break;
			case GLITE_JP_QUERYOP_LESS:
				if (q_max_time > query[i].value.time.tv_sec + 1)
					q_max_time = query[i].value.time.tv_sec + 1;
				break;
			case GLITE_JP_QUERYOP_WITHIN:
				if (q_max_time > query[i].value2.time.tv_sec + 1)
					q_max_time = query[i].value2.time.tv_sec + 1;
				/* fallthrough */
			case GLITE_JP_QUERYOP_GREATER:
				if (q_min_time < query[i].value.time.tv_sec)
					q_min_time = query[i].value.time.tv_sec;
				break;
			default:
				err.code = EINVAL;
				err.desc = "Invalid query op";
				return glite_jp_stack_error(ctx,&err);
				break;
			}
		}
		if (query[i].attr.type == GLITE_JP_ATTR_TAG) 
			q_with_tags = 1;

	}

	for (i = 0; metadata[i].attr.type != GLITE_JP_ATTR_UNDEF; i++) {
		switch (metadata[i].attr.type) {
		case GLITE_JP_ATTR_OWNER:
		case GLITE_JP_ATTR_TIME:
			md_info = 1;
			break;
		case GLITE_JP_ATTR_TAG:
			md_tags = 1;
			break;
		default:
			err.code = EINVAL;
			err.desc = "Invalid attribute type in metadata parameter";
			return glite_jp_stack_error(ctx,&err);
			break;
		}
	}
	metadata_templ = (glite_jp_attrval_t *) calloc(i + 1, sizeof(glite_jp_attrval_t));
	if (!metadata_templ) {
		err.code = ENOMEM;
		return glite_jp_stack_error(ctx,&err);
	}
	memcpy(metadata_templ, metadata, (i + 1) * sizeof(glite_jp_attrval_t));
	
	q_min_time_tr = regtime_trunc(q_min_time);
	q_max_time_tr = regtime_ceil(q_max_time);

	if (q_exact_owner) {
		ownerhash = str2md5(q_exact_owner); /* static buffer */
		if (asprintf(&owner_dirname, "%s/data/%s", config->internal_path, ownerhash) == -1) {
			err.code = ENOMEM;
			return glite_jp_stack_error(ctx,&err);
		}
		owner_dirp = opendir(owner_dirname);
		free(owner_dirname);
		if (!owner_dirp) {
			free(metadata_templ);
			return 0; /* found nothing */
		}
		while ((ttimeent = readdir(owner_dirp)) != NULL) {
			if (!strcmp(ttimeent->d_name, ".")) continue;
			if (!strcmp(ttimeent->d_name, "..")) continue;
			ttime = atol(ttimeent->d_name);
			if (ttime >= q_min_time_tr && ttime < q_max_time_tr) {
				if (query_phase2(ctx, ownerhash, ttime, q_with_tags, md_tags,
						query, metadata_templ, callback)) {
					err.code = EIO;
					err.desc = "query_phase2() error";
					goto error_out;
				}
			}
		}
	} else { /* !q_exact_owner */
		if (asprintf(&data_dirname, "%s/data", config->internal_path) == -1) {
			err.code = ENOMEM;
			goto error_out;
		}
		data_dirp = opendir(data_dirname);
		if (!data_dirp) {
			err.code = EIO;
			err.desc = "Cannot open data directory";
			goto error_out;
		}
		while ((ownerent = readdir(data_dirp)) != NULL) {
			if (!strcmp(ownerent->d_name, ".")) continue;
			if (!strcmp(ownerent->d_name, "..")) continue;
			if (asprintf(&owner_dirname, "%s/data/%s", config->internal_path,
					ownerent->d_name) == -1) {
				err.code = ENOMEM;
				goto error_out;
			}
			owner_dirp = opendir(owner_dirname);
			free(owner_dirname);
			if (!owner_dirp) {
				err.code = EIO;
				err.desc = "Cannot open owner data directory";
				goto error_out;
			}
			while ((ttimeent = readdir(owner_dirp)) != NULL) {
				if (!strcmp(ttimeent->d_name, ".")) continue;
				if (!strcmp(ttimeent->d_name, "..")) continue;
				ttime = atol(ttimeent->d_name);
				if (ttime >= q_min_time_tr && ttime < q_max_time_tr) {
					if (query_phase2(ctx, ownerent->d_name, ttime, q_with_tags, md_tags,
							query, metadata_templ, callback)) {
						err.code = EIO;
						err.desc = "query_phase2() error";
						goto error_out;
					}
				}
			}
			closedir(owner_dirp); owner_dirp = NULL;
		}
		closedir(data_dirp); data_dirp = NULL;
	}
	return 0;

error_out:
	if (owner_dirp) closedir(owner_dirp);
	if (data_dirp) closedir(data_dirp);
	free(data_dirname);
	free(metadata_templ);
	return glite_jp_stack_error(ctx,&err);
}

#else 

/* placeholder instead */
int glite_jppsbe_query(
	glite_jp_context_t ctx,
	const glite_jp_query_rec_t query[],
	const glite_jp_attrval_t metadata[],
	int (*callback)(
		glite_jp_context_t ctx,
		const char *job,
		const glite_jp_attrval_t metadata[]
	)
)
{
	glite_jp_error_t	err;
	err.code = ENOSYS;
	err.desc = "not implemented";
	return glite_jp_stack_error(ctx,&err);
}

#endif

/* XXX:
- no primary authorization yet
- no concurrency control yet
- partial success in pwrite,append
- "unique" part of jobid is assumed to be unique across bookkeeping servers
- repository versioning not fully implemented yet
*/
