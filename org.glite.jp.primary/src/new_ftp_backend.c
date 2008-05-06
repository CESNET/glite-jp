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
#include <zlib.h>

#include "glite/lbu/trio.h"
#include "glite/lbu/escape.h"
#include "glite/jobid/strmd5.h"
#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/known_attr.h"
#include "glite/jp/attr.h"
#include "glite/jp/db.h"

#include "feed.h"
#include "tags.h"
#include "backend_private.h"

#include "jpps_H.h"	/* XXX: SOAP_TYPE___jpsrv__GetJob */

#include "jptype_map.h"

#define FTPBE_DEFAULT_DB_CS	"jpps/@localhost:jpps"

struct ftpbe_config {
	char *internal_path;
	char *external_path;
	char *db_cs;
//	char *gridmap;
	char *logname;
};

static struct ftpbe_config *config = NULL;

struct fhandle_rec {
	gzFile  fd_gz;
	char*   filename;
	int     filemode;
	char*	filedata;
	int 	offset;
	int     eof;
	int     modified;
};
typedef struct fhandle_rec *fhandle;

static struct option ftpbe_opts[] = {
	{ "ftp-internal-path", 1, NULL, 'I' },
	{ "ftp-external-path", 1, NULL,	'E' },
	{ "ftp-db-cs",	       1, NULL,	'D' },
//	{ "ftp-gridmap",       1, NULL,	'G' },
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
//		config->gridmap == NULL ||
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

	if (glite_jp_db_ExecSQL(ctx, stmt, NULL) < 0) {
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

	while ((opt = getopt_long(argc, argv, "I:E:D:" /* G: */, ftpbe_opts, NULL)) != EOF) {
		switch (opt) {
			case 'I': config->internal_path = optarg; break;
			case 'E': config->external_path = optarg; break;
			case 'D': config->db_cs = optarg; break;
//			case 'G': config->gridmap = optarg; break;
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

	if (glite_lbu_InitDBContext(((glite_lbu_DBContext *)&ctx->dbhandle)) != 0) {
		err.code = EINVAL;
		err.desc = "Cannot init backend's database";
		return glite_jp_stack_error(ctx,&err);
	}
	if (glite_lbu_DBConnect(ctx->dbhandle, config->db_cs)) {
		err.code = EIO;
		err.desc = "Cannot access backend's database (during init)";
		return glite_jp_stack_error(ctx,&err);
	} else {
		/* slaves open their own connections */
		glite_lbu_DBClose(ctx->dbhandle);
		glite_lbu_FreeDBContext(ctx->dbhandle);
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
	
	if (glite_lbu_InitDBContext(((glite_lbu_DBContext *)&ctx->dbhandle)) != 0) {
		err.code = EINVAL;
		err.desc = "Cannot init backend's database";
		return glite_jp_stack_error(ctx,&err);
	}
	if (glite_lbu_DBConnect(ctx->dbhandle, config->db_cs)) {
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

	glite_lbu_TimeToDB(reg_tv.tv_sec, &dbtime);
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
	
	if (glite_jp_db_ExecSQL(ctx, stmt, NULL) < 0) {
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

#if 0
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
#endif

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
	char *commit_before_inout_str;
	time_t	now;
	char *now_str = NULL;

	char *stmt = NULL;
	glite_lbu_Statement db_res;
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

	if ((db_retn = glite_jp_db_ExecSQL(ctx, stmt, &db_res)) <= 0) {
		if (db_retn == 0) {
			err.code = ENOENT;
			err.desc = "No such job registered";
		} else {
			err.code = EIO;
			err.desc = "DB access failed";
		}
		goto error_out;
	}
	
	db_retn = glite_jp_db_FetchRow(ctx, db_res, sizeof(db_row)/sizeof(db_row[0]), NULL, db_row);
	if (db_retn != 2) {
		glite_jp_db_FreeStmt(&db_res);
		err.code = EIO;
		err.desc = "DB access failed";
		goto error_out;
	}

	glite_jp_db_FreeStmt(&db_res);
	
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
			regtime_trunc(glite_lbu_DBToTime(db_row[1])),
			ju, data_basename) == -1) {
		err.code = ENOMEM;
		goto error_out;
	}
	if (asprintf(destination_out, "%s/data/%s/%d/%s/%s",
			config->external_path, db_row[0],
			regtime_trunc(glite_lbu_DBToTime(db_row[1])),
			ju, data_basename) == -1) {
		err.code = ENOMEM;
		goto error_out;
	}

	if (commit_before_inout != NULL)
	/* XXX no timeout enforced */
		/* XXX: gsoap does not like so much, one year should be enough
		*commit_before_inout = (time_t) LONG_MAX;
		*/
		*commit_before_inout = time(NULL) + 5*60;//365*24*60*60;
	
	/* 
	if (add_to_gridmap(ctx, peername)) {
		err.code = EIO;
		err.desc = "Cannot add peer DN to ftp server authorization file";
		goto error_out;
	}
	*/
	else if (*commit_before_inout > time(NULL) + 5*60)
		*commit_before_inout = time(NULL) + 5*60;

	peerhash = str2md5(peername); /* static buffer */
	if (store_user(ctx, peerhash, peername)) {
		err.code = EIO;
		err.desc = "Cannot store upload user entry";
		goto error_out;
	}

	free(stmt); stmt = NULL;

	time(&now);
	glite_lbu_TimeToDB(now,&now_str);

        trio_asprintf(&stmt,"delete from files where jobid = '%|Ss' and state = 'uploading' and deadline < %s", ju, now_str);
	free(now_str);

        if (!stmt) {
                err.code = ENOMEM;
                goto error_out;
        }
        if (glite_jp_db_ExecSQL(ctx, stmt, NULL) < 0) {
                err.code = EIO;
                err.desc = "DB access failed";
                goto error_out;
        }

	free(stmt); stmt = NULL;

	glite_lbu_TimeToDB(*commit_before_inout, &commit_before_inout_str);
	trio_asprintf(&stmt,"insert into files"
		"(jobid,filename,int_path,ext_url,state,deadline,ul_userid) "
		"values ('%|Ss','%|Ss','%|Ss','%|Ss','%|Ss', %s, '%|Ss')",
		ju, data_basename, data_fname, *destination_out, "uploading", 
		commit_before_inout_str, peerhash);
	free(commit_before_inout_str);
	if (!stmt) {
		err.code = ENOMEM;
		goto error_out;
	}

	if (glite_jp_db_ExecSQL(ctx, stmt, NULL) < 0) {
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
	glite_lbu_Statement db_res;
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
	
	if ((db_retn = glite_jp_db_ExecSQL(ctx, stmt, &db_res)) <= 0) {
		if (db_retn == 0) {
			err.code = ENOENT;
			err.desc = "No such upload in progress";
		} else {
			err.code = EIO;
			err.desc = "DB access failed";
		}
		goto error_out;
	}

	db_retn = glite_jp_db_FetchRow(ctx, db_res, sizeof(db_row)/sizeof(db_row[0]), NULL, db_row);
	if (db_retn != 7) {
		glite_jp_db_FreeStmt(&db_res);
		err.code = EIO;
		err.desc = "DB access failed";
		goto error_out;
	}
	glite_jp_db_FreeStmt(&db_res);

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

	if (glite_jp_db_ExecSQL(ctx, stmt, NULL) < 0) {
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
	glite_lbu_Statement db_res;
	int db_retn;
	char *db_row[2] = { NULL, NULL};
	int i;
	char *cp = NULL;
	
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
	
	if ((db_retn = glite_jp_db_ExecSQL(ctx, stmt, &db_res)) <= 0) {
		if (db_retn == 0) {
			err.code = ENOENT;
			err.desc = "Invalid destination string";
		} else {
			err.code = EIO;
			err.desc = "DB access failed";
		}
		goto error_out;
	}

	db_retn = glite_jp_db_FetchRow(ctx, db_res, sizeof(db_row)/sizeof(db_row[0]), NULL, db_row);
	if (db_retn != 2) {
		glite_jp_db_FreeStmt(&db_res);
		err.code = EIO;
		err.desc = "DB access failed";
		goto error_out;
	}
	glite_jp_db_FreeStmt(&db_res);

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
	char *ju = NULL;
	char *ju_path = NULL;

	char *stmt = NULL;
	glite_lbu_Statement db_res;
	int db_retn;
	char *db_row[3] = { NULL, NULL, NULL };

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

	if ((db_retn = glite_jp_db_ExecSQL(ctx, stmt, &db_res)) <= 0) {
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
	
	db_retn = glite_jp_db_FetchRow(ctx, db_res, sizeof(db_row)/sizeof(db_row[0]), NULL, db_row);
	if (db_retn != 3) {
		glite_jp_db_FreeStmt(&db_res);
		err.code = EIO;
		err.desc = "DB access failed";
		goto error_out;
	}

	glite_jp_db_FreeStmt(&db_res);

	if (glite_jpps_authz(ctx,SOAP_TYPE___jpsrv__GetJobFiles,job,db_row[2])) {
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
			regtime_trunc(glite_lbu_DBToTime(db_row[1])),
			ju, data_basename) == -1) {
		err.code = ENOMEM;
		goto error_out;
	}

// FIXME: relict?
#if 0
	trio_asprintf(&stmt,"select 'x' from files where jobid='%|Ss' "
				"and ext_url = '%|Ss' "
				"and state='committed' ",ju,*url_out);

	if ((db_retn = glite_jp_db_ExecSQL(ctx,stmt,&db_res)) <= 0) {
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
#endif

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
	glite_lbu_Statement db_res;
	int db_retn;
	char *db_row[2] = { NULL, NULL };

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

	if ((db_retn = glite_jp_db_ExecSQL(ctx, stmt, &db_res)) <= 0) {
		if (db_retn == 0) {
			err.code = ENOENT;
			err.desc = "No such job registered";
		} else {
			err.code = EIO;
			err.desc = "DB access failed";
		}
		goto error_out;
	}
	
	db_retn = glite_jp_db_FetchRow(ctx, db_res, sizeof(db_row)/sizeof(db_row[0]), NULL, db_row);
	if (db_retn != 2) {
		glite_jp_db_FreeStmt(&db_res);
		err.code = EIO;
		err.desc = "DB access failed";
		goto error_out;
	}

	glite_jp_db_FreeStmt(&db_res);
	
	/* XXX name length */
	if (asprintf(&data_basename, "%s%s%s", class,
		(name != NULL) ? "." : "", (name != NULL) ? name : "") == -1) {
		err.code = ENOMEM;
		goto error_out;
	}

	if (asprintf(fname_out, "%s/data/%s/%d/%s/%s",
			config->internal_path, db_row[0],
			regtime_trunc(glite_lbu_DBToTime(db_row[1])),
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

	int error = 0;
	int created = 0;
	if (mode % 4 == O_RDONLY)
		handle->fd_gz = gzopen(fname, "r");
	else if (mode % 4 == O_WRONLY)
		handle->fd_gz = gzopen(fname, "r+");
	else if (mode % 4 == O_RDWR){
		handle->fd_gz = gzopen(fname, "r+");
		if ((handle->fd_gz == NULL) && (mode & O_CREAT)){
			handle->fd_gz = gzopen(fname, "w+");
			created = 1; // when the file is created, gzread returns -2 
		}
	}
	if (handle->fd_gz == NULL){
	        gzerror(((fhandle)handle)->fd_gz, &(err.code));
                err.desc = "Cannot open requested file";
                free(handle); handle = NULL;
                error = 1;
                goto error_out;
        }

	handle->offset = 0;
	handle->eof = 0;

	if (! created){
		const int READ_STEP = 8192;
		//handle->filedata = malloc(sizeof(*handle->filedata)*READ_STEP);
		int diff = 0;
		char buf[READ_STEP];
		do{
			diff = gzread(handle->fd_gz, buf/*handle->filedata + handle->eof*/, READ_STEP);
			if (diff < 0){
				gzerror(((fhandle)handle)->fd_gz, &(err.code));
		                err.desc = "Error reading file";
        		        free(handle->filedata);
				error = 1;
                		goto error_out;
			}
			handle->eof += diff;
			handle->filedata = realloc(handle->filedata, sizeof(*handle->filedata)*handle->eof);
			memcpy(handle->filedata + handle->eof - diff, buf, sizeof(*buf)*diff);
		} while(diff == READ_STEP);
	}
	
	/*if (gzclose(fd_gz)){
		gzerror(((fhandle)handle)->fd_gz, &(err.code));
                err.desc = "Error closing file descriptor";
		free(handle->filedata);
                goto error_out;
	}*/

	handle->filename = strdup(fname);
	handle->filemode = mode;
        handle->modified = 0;

	*handle_out = (void*) handle;

error_out:
	free(fname);
	if (error) { 
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

	if (gzclose(((fhandle)handle)->fd_gz)){
		gzerror(((fhandle)handle)->fd_gz, &(err.code));
                err.desc = "Error closing file descriptor";
                goto error_out;
	}

	if (((fhandle)handle)->modified){
		if ((((fhandle)handle)->fd_gz = gzopen(((fhandle)handle)->filename, "w")) == NULL){
			gzerror(((fhandle)handle)->fd_gz, &(err.code));
                	err.desc = "Error opening file for write changes";
	                goto error_out;
		}
		if (gzwrite(((fhandle)handle)->fd_gz, ((fhandle)handle)->filedata, ((fhandle)handle)->eof) < 0){
			gzerror(((fhandle)handle)->fd_gz, &(err.code));
                        err.desc = "Error writing changes";
                        goto error_out;
		}
		if (gzclose(((fhandle)handle)->fd_gz)){
			gzerror(((fhandle)handle)->fd_gz, &(err.code));
	                err.desc = "Error closing file descriptor";
        	        goto error_out;
		}
	}

error_out:
	free(((fhandle)handle)->filedata);
	free(handle); handle=NULL;
	if (err.code) {
		return glite_jp_stack_error(ctx,&err);
	} else { 
		return 0;
	}
}


int glite_jppsbe_file_attrs(glite_jp_context_t ctx, void *handle, struct stat *buf){
	glite_jp_error_t err;

        assert(handle != NULL);

        glite_jp_clear_error(ctx);
        memset(&err,0,sizeof err);
        err.source = __FUNCTION__;

	if (! stat(((fhandle)handle)->filename, buf)) {
		err.code = errno;
		err.desc = "Error calling fstat";
		return glite_jp_stack_error(ctx,&err);
	}

	return 0;
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
	glite_jp_error_t err;

        assert(handle != NULL);
        assert(buf != NULL);

	glite_jp_clear_error(ctx);
        memset(&err,0,sizeof err);
        err.source = __FUNCTION__;

	if (((fhandle)handle)->filename == NULL){
		err.code = 0;
		err.desc = "Cannot read, file not open";
                return glite_jp_stack_error(ctx,&err);
	}

	int to_read;
	if (offset + nbytes > ((fhandle)handle)->eof)
		to_read = ((fhandle)handle)->eof - offset;
	else
		to_read = nbytes;
	memcpy(buf, ((fhandle)handle)->filedata + offset, to_read);
	((fhandle)handle)->offset = offset + to_read;

	*nbytes_ret = to_read;

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

	if (((fhandle)handle)->filename == NULL){
                err.code = 0;
                err.desc = "Cannot write, file not open";
                return glite_jp_stack_error(ctx,&err);
        }

	if (((fhandle)handle)->filemode % 4 == 0){
		err.desc = "Cannot write to readonly file";
		return glite_jp_stack_error(ctx,&err);
	}
	
	if (offset + nbytes > ((fhandle)handle)->eof){
		((fhandle)handle)->filedata = realloc(((fhandle)handle)->filedata, offset + nbytes);
		((fhandle)handle)->eof = offset + nbytes;
	}

	memcpy(((fhandle)handle)->filedata + offset, buf, nbytes);

	((fhandle)handle)->modified = 1;

	return 0;
}

int glite_jppsbe_compress_and_remove_file(
	glite_jp_context_t ctx,
        const char *job,
        const char *class,
        const char *name
){
	glite_jp_error_t err;
        glite_jp_clear_error(ctx);
        memset(&err,0,sizeof err);
        err.source = __FUNCTION__;

	char *src, *dest;
	get_job_fname(ctx, job, class, name, &src);

	dest = malloc(sizeof(*dest)*(strlen(src)+strlen(".gz")+1));
	sprintf(dest, "%s.gz", src);

	char buf[8192];
	FILE* s = fopen(src, "r");
	gzFile d = gzopen(dest, "w");
	size_t l;
	while ((l = fread(buf, sizeof(*buf), 8192, s)) > 0)
		gzwrite(d, buf, sizeof(*buf)*l);
	gzclose(d);
	fclose(s);

	char *ju, *ju_path;
	if (jobid_unique_pathname(ctx, job, &ju, &ju_path, 1) != 0) {
                err.code = ctx->error->code;
                err.desc = "Cannot obtain jobid unique path/name";
		goto error_out;
        }

	rename(dest, src);

error_out:
        if (err.code) {
                return glite_jp_stack_error(ctx,&err);
        } else {
                return 0;
        }

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

        ((fhandle)handle)->filedata = realloc(((fhandle)handle)->filedata, ((fhandle)handle)->eof + nbytes);
	memcpy(((fhandle)handle)->filedata + ((fhandle)handle)->eof, buf, nbytes);
        
	((fhandle)handle)->eof += nbytes;
	((fhandle)handle)->modified = 1;

	return 0;
}

static int get_job_info(
	glite_jp_context_t ctx,
	const char *job,
	char **owner,
	struct timeval *tv_reg
)
{
	char	*qry,*col[2];
	int	rows;
	glite_jp_error_t	err;
	glite_lbu_Statement	s = NULL;

	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;
	glite_jp_clear_error(ctx);

	trio_asprintf(&qry,"select u.cert_subj,j.reg_time "
		"from jobs j, users u "
		"where j.owner = u.userid "
		"and j.dg_jobid = '%|Ss'",job);

	if ((rows = glite_jp_db_ExecSQL(ctx,qry,&s)) <= 0) {
		if (rows == 0) {
			err.code = ENOENT;
			err.desc = "No records for this job";
		}
		else {
			err.code = EIO;
			err.desc = "DB call fail retrieving job files";
		}
		glite_jp_stack_error(ctx,&err);
		goto cleanup;
	}

	if (glite_jp_db_FetchRow(ctx,s,sizeof(col)/sizeof(col[0]), NULL, col) < 0) {
		err.code = EIO;
		err.desc = "DB call fail retrieving job files";
		glite_jp_stack_error(ctx,&err);
		goto cleanup;
	}

	*owner = col[0];
	tv_reg->tv_sec = glite_lbu_DBToTime(col[1]);
	tv_reg->tv_usec = 0;
	free(col[1]);

cleanup:
	free(qry);
	if (s) glite_jp_db_FreeStmt(&s);
	return err.code;
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
	int i,j;
	glite_jp_error_t err;

	assert(job != NULL);
	assert(attrs_inout != NULL);

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	for (i = 0; attrs_inout[i].name; i++) {
/* must be implemented via filetype plugin
		case GLITE_JP_ATTR_TIME:
		case GLITE_JP_ATTR_TAG:
*/
		if (!strcmp(attrs_inout[i].name,GLITE_JP_ATTR_OWNER)
			|| !strcmp(attrs_inout[i].name,GLITE_JP_ATTR_REGTIME)) {
			if (!got_info) {
				if (get_job_info(ctx, job, &owner, &tv_reg)) {
					err.code = ctx->error->code;
					err.desc = "Cannot retrieve job info";
					goto error_out;
				}
				got_info = 1;
			}
		}
		else {
			err.code = EINVAL;
			err.desc = "Invalid attribute type";
			goto error_out;
			break;
		}

		if (!strcmp(attrs_inout[i].name,GLITE_JP_ATTR_OWNER)) {
			attrs_inout[i].value = strdup(owner);
			if (!attrs_inout[i].value) {
				err.code = ENOMEM;
				err.desc = "Cannot copy owner string";
				goto error_out;
			}	
			attrs_inout[i].origin = GLITE_JP_ATTR_ORIG_SYSTEM;
			attrs_inout[i].origin_detail = NULL;

			/* XXX */
			attrs_inout[i].timestamp = tv_reg.tv_sec;
		}

		if (!strcmp(attrs_inout[i].name,GLITE_JP_ATTR_REGTIME)) {
			trio_asprintf(&attrs_inout[i].value,"%ld.%06ld",tv_reg.tv_sec,tv_reg.tv_usec);
			attrs_inout[i].origin = GLITE_JP_ATTR_ORIG_SYSTEM;
			attrs_inout[i].origin_detail = NULL;
			attrs_inout[i].timestamp = tv_reg.tv_sec;
		}
	
/* must be implemented via filetype plugin
		case GLITE_JP_ATTR_TIME:
		case GLITE_JP_ATTR_TAG:
*/
	}

error_out:
	free(owner);
	if (err.code) {
		while (i > 0) {
			i--;
			glite_jp_attrval_free(attrs_inout+i,0);
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


int glite_jppsbe_query(
	glite_jp_context_t ctx,
	const glite_jp_query_rec_t query[],
	char * attrs[],
	void *arg,
	int (*callback)(
		glite_jp_context_t ctx,
		const char *job,
		const glite_jp_attrval_t metadata[],
		void *arg
	)
)
{
	glite_jp_error_t	err;
	int	i,ret;
	int	quser = 0;
	char	*where = NULL,*stmt = NULL,*aux = NULL, *cols = NULL;
	char	*qres[3] = { NULL, NULL, NULL };
	int	cmask = 0, owner_idx = -1, reg_idx = -1;
	glite_lbu_Statement	q = NULL;
	glite_jp_attrval_t	metadata[3];

	memset(&err,0,sizeof err);
	glite_jp_clear_error(ctx);
	err.source = __FUNCTION__;

	/* XXX: assuming not more than 2 */
	memset(metadata,0, sizeof metadata);

	/* XXX: const discarding is OK */
	for (i=0;attrs[i]; i++) {
		assert(i<2);
		metadata[i].name = (char *) attrs[i];
	}

	for (i=0; query[i].attr; i++) {
		char	*qitem;

		/* XXX: don't assert() */
		assert(!query[i].binary);

		if (!strcmp(query[i].attr,GLITE_JP_ATTR_OWNER)) {
			switch (query[i].op) {
				case GLITE_JP_QUERYOP_EQUAL:
					quser = 1;
					trio_asprintf(&qitem,"u.cert_subj = '%|Ss'",query[i].value);
					break;
				default:
					err.code = EINVAL;
					err.desc = "only = allowed for owner queries";
					glite_jp_stack_error(ctx,&err);
					goto cleanup;
			}
		}
		else if (!strcmp(query[i].attr,GLITE_JP_ATTR_REGTIME)) {
			time_t 	t = glite_jp_attr2time(query[i].value);
			char	*t1,*t2 = NULL;

			glite_lbu_TimeToDB(t, &t1);
			switch (query[i].op) {
				case GLITE_JP_QUERYOP_EQUAL:
					trio_asprintf(&qitem,"j.reg_time = %s",t1);
					break;
				case GLITE_JP_QUERYOP_UNEQUAL:
					trio_asprintf(&qitem,"j.reg_time != %s",t1);
					break;
				case GLITE_JP_QUERYOP_LESS:
					trio_asprintf(&qitem,"j.reg_time < %s",t1);
					break;
				case GLITE_JP_QUERYOP_GREATER:
					trio_asprintf(&qitem,"j.reg_time > %s",t1);
					break;
				case GLITE_JP_QUERYOP_WITHIN:
					free(t2);
					glite_lbu_TimeToDB(glite_jp_attr2time(query[i].value2)+1, &t2);
					trio_asprintf(&qitem,"j.reg_time >= %s and j.reg_time <= %s",t1,t2);
					break;
				default:
					err.code = EINVAL;
					err.desc = "invalid query op";
					glite_jp_stack_error(ctx,&err);
					goto cleanup;
			}
			free(t1);
			free(t2);
		}
		trio_asprintf(&aux,"%s%s%s",where ? where : "",where ? " and " : "", qitem);
		free(where);
		free(qitem);
		where = aux;
		aux = NULL;
	}

	for (i=0; metadata[i].name; i++) {
		assert (i<2);	/* XXX: should never happen */

		if (!strcmp(metadata[i].name,GLITE_JP_ATTR_OWNER)) {
			quser = 1;
			cmask |= 1;
			owner_idx = i;
		}
		else if (!strcmp(metadata[i].name,GLITE_JP_ATTR_REGTIME)) {
			cmask |= 2;
			reg_idx = i;
		}
		else {
			err.code = EINVAL;
			err.desc = "invalid query column";
			glite_jp_stack_error(ctx,&err);
			goto cleanup;
		}
	}
	switch (cmask) {
		case 1: cols = "j.dg_jobid,u.cert_subj"; break;
		case 2: cols = "j.dg_jobid,j.reg_time"; break;
		case 3:	cols = "j.dg_jobid,u.cert_subj,j.reg_time"; break;
	}
	
	trio_asprintf(&stmt,"select %s from jobs j%s where %s %s",
			cols,
			quser ? ",users u" : "",
			where,
			cmask & 1 ? "and u.userid = j.owner" : "");

	if ((ret = glite_jp_db_ExecSQL(ctx,stmt,&q)) < 0) {
		err.code = EIO;
		err.desc = "DB call fail";
		glite_jp_stack_error(ctx,&err);
		goto cleanup;
	}
	else if (ret == 0) {
		err.code = ENOENT;
		err.desc = "no matching jobs";
		glite_jp_stack_error(ctx,&err);
		goto cleanup;
	}

	while ((ret = glite_jp_db_FetchRow(ctx,q,sizeof(qres)/sizeof(qres[0]), NULL, qres)) > 0) {
		if (cmask & 1) {
			/* XXX: owner always first */
			metadata[owner_idx].value = qres[1];
			metadata[owner_idx].origin = GLITE_JP_ATTR_ORIG_SYSTEM;
			qres[1] = NULL;
		}
		if (cmask & 2) {
			int	qi = cmask == 2 ? 1 : 2;
			time_t	t = glite_lbu_DBToTime(qres[qi]);
			metadata[reg_idx].value = glite_jp_time2attr(t);
			metadata[reg_idx].origin = GLITE_JP_ATTR_ORIG_SYSTEM;
			free(qres[qi]);
			qres[qi] = NULL;
		}
		if (callback(ctx,qres[0],metadata,arg)) {
			err.code = EIO;
			err.desc = qres[0];
			glite_jp_stack_error(ctx,&err);
			goto cleanup;
		}

		free(qres[0]);
		free(metadata[0].value);
		free(metadata[1].value);
		qres[0] = metadata[0].value = metadata[1].value = NULL;
	}


	if (ret < 0) {
		err.code = EIO;
		err.desc = "DB call fail";
		glite_jp_stack_error(ctx,&err);
		goto cleanup;
	}

cleanup:
	free(where);
	free(aux);
	free(stmt);
	free(qres[0]); free(qres[1]); free(qres[2]);
	free(metadata[0].value); free(metadata[1].value);
	if (q) glite_jp_db_FreeStmt(&q);

	return err.code;
}


int glite_jppsbe_is_metadata(glite_jp_context_t ctx,const char *attr)
{
	/* XXX: should be more */
	if (!strcmp(attr,GLITE_JP_ATTR_OWNER)) return 1;
	if (!strcmp(attr,GLITE_JP_ATTR_REGTIME)) return 1;

	return 0;
}


int glite_jppsbe_get_names(
	glite_jp_context_t ctx,
	const char *job,
	const char *class,
	char	***names_out
)
{
	char	*qry = NULL,*file = NULL,*dot;
	char	**out = NULL;
	glite_lbu_Statement	s = NULL;
	int rows,nout = 0;
	glite_jp_error_t	err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	trio_asprintf(&qry,"select filename from files f,jobs j "
			"where j.dg_jobid = '%|Ss' and j.jobid = f.jobid and f.state = 'committed'",job);

	if ((rows = glite_jp_db_ExecSQL(ctx,qry,&s)) <= 0) {
		if (rows == 0) {
			err.code = ENOENT;
			err.desc = "No files for this job";
		}
		else {
			err.code = EIO;
			err.desc = "DB call fail retrieving job files";
		}
		glite_jp_stack_error(ctx,&err);
		goto cleanup;
	}

	while ((rows = glite_jp_db_FetchRow(ctx,s,1,NULL,&file))) {
		if (rows < 0) {
			err.code = EIO;
			err.desc = "DB call fail retrieving job files";
			goto cleanup;
		}

		dot = strchr(file,'.'); /* XXX: can class contain dot? */

		if (dot) *dot = 0;
		out = realloc(out,(nout+1) * sizeof *out);
		if (!strcmp(file,class)) out[nout++] = dot ? dot+1 : NULL;

		free(file);
		file = NULL;
	}

cleanup:
	if (s) glite_jp_db_FreeStmt(&s);
	free(qry);
	free(file);

	if (ctx->error) {
		int	i;
		for (i=0; out && out[i]; i++) free(out[i]);
		free(out);
		return -ctx->error->code;
	}

	if (nout) *names_out = out;
	return nout;
}


/** mark the job as sent to this feed */
int glite_jppsbe_set_fed(
	glite_jp_context_t ctx,
	const char *feed,
	const char *job
)
{
	char	*stmt = NULL,*u = NULL;
	int	rows,ret;
	glite_jp_error_t	err;
	memset(&err,0,sizeof err);

	if ((ret = jobid_unique_pathname(ctx,job,&u,NULL,0))) return ret;

	trio_asprintf(&stmt,"insert into fed_jobs(feedid,jobid) "
		"values ('%|Ss','%|Ss')", feed,u);
	free(u);

	if ((rows = glite_jp_db_ExecSQL(ctx,stmt,NULL)) < 0) {
		err.source = __FUNCTION__;
		err.code = EIO;
		err.desc = "insert into fed_jobs";
		glite_jp_stack_error(ctx,&err);
		goto cleanup;
	}

	if (rows != 1) {
		err.source = __FUNCTION__;
		err.code = EIO;
		err.desc = "inserted rows != 1";
		glite_jp_stack_error(ctx,&err);
	}

cleanup:
	free(stmt);
	return err.code;
}


/** check whether the job has been already sent to this feed */
int glite_jppsbe_check_fed(
	glite_jp_context_t ctx,
	const char *feed,
	const char *job,
	int *result
)
{
	char	*stmt = NULL,*u = NULL;
	int	rows,ret;
	glite_jp_error_t	err;
	memset(&err,0,sizeof err);

	if ((ret = jobid_unique_pathname(ctx,job,&u,NULL,0))) return ret;

	trio_asprintf(&stmt,"select 'x' from fed_jobs "
			"where jobid = '%|Ss' and feedid = '%|Ss'",
			u,feed);

	free(u);

	if ((rows = glite_jp_db_ExecSQL(ctx,stmt,NULL)) < 0) {
		err.source = __FUNCTION__;
		err.code = EIO;
		err.desc = "select from fed_jobs";
		glite_jp_stack_error(ctx,&err);
		goto cleanup;
	}

	*result = rows;

cleanup:
	free(stmt);
	return err.code;
}


/** store the feed to database */
int glite_jppsbe_store_feed(
	glite_jp_context_t ctx,
	struct jpfeed *feed
)
{
	char	*stmt,*aux,*alist,*qlist,*e;
	int	i,rows;
	glite_jp_error_t	err;

	memset(&err,0,sizeof err);

	qlist = alist = stmt = aux = e = NULL;

	for (i=0; feed->attrs[i]; i++) {
		char	*e;
		trio_asprintf(&aux,"%s%s%s",
				alist ? alist : "",
				alist ? "\n" : "",
				e = glite_lbu_EscapeULM(feed->attrs[i]));
		free(e);
		free(alist);
		alist = aux;
		aux = NULL;
	}

	for (i=0; feed->qry[i].attr; i++) {
		char	op,*e1,*e2 = NULL;

		/* XXX */
		assert(!feed->qry[i].binary);

		switch (feed->qry[i].op) {
			case GLITE_JP_QUERYOP_EQUAL: op = '='; break;
			case GLITE_JP_QUERYOP_UNEQUAL: op = '!'; break;
			case GLITE_JP_QUERYOP_LESS: op = '<'; break;
			case GLITE_JP_QUERYOP_GREATER: op = '>'; break;
			case GLITE_JP_QUERYOP_EXISTS: op = 'E'; break;
			default: abort(); /* XXX */
		}

		trio_asprintf(&aux,"%s%s%s\n%c\n%s",
				qlist ? qlist : "",
				qlist ? "\n" : "",
				e1 = glite_lbu_EscapeULM(feed->qry[i].attr),
				op,
				op != 'E' ? e2 = glite_lbu_EscapeULM(feed->qry[i].value) : "E");
		free(e1); free(e2);

		free(qlist);
		qlist = aux;
		aux = NULL;
	}

	glite_lbu_TimeToDB(feed->expires, &e);
	trio_asprintf(&stmt,"insert into feeds(feedid,destination,expires,cols,query) "
			"values ('%|Ss','%|Ss',%s,'%|Ss','%|Ss')",
			feed->id,feed->destination,
			e,
			alist,qlist);

	free(alist); free(qlist); free(e);

	if ((rows = glite_jp_db_ExecSQL(ctx,stmt,NULL)) < 0) {
		err.source = __FUNCTION__;
		err.code = EIO;
		err.desc = "insert into fed_jobs";
		glite_jp_stack_error(ctx,&err);
		goto cleanup;
	}

	if (rows != 1) {
		err.source = __FUNCTION__;
		err.code = EIO;
		err.desc = "inserted rows != 1";
		glite_jp_stack_error(ctx,&err);
	}

cleanup:
	free(stmt);
	return err.code;

}

int glite_jppsbe_refresh_feed(
        glite_jp_context_t ctx,
        char *feed_id,
	time_t expires
)
{
	glite_jp_error_t        err;
        memset(&err,0,sizeof err);

	char *stmt = NULL;
	char *e = NULL;
	glite_lbu_TimeToDB(expires, &e);
	trio_asprintf(&stmt, "update feeds set expires=%s where feedid=%s",
		e, feed_id);
	if (!stmt) {
                err.code = ENOMEM;
                goto error_out;
        }

        if (glite_jp_db_ExecSQL(ctx, stmt, NULL) < 0) {
                err.code = EIO;
                err.desc = "DB access failed";
                goto error_out;
        }

error_out:
        free(stmt);
	free(e);
        if (err.code)
                return glite_jp_stack_error(ctx,&err);
        else
                return 0;
}

/** purge expired feeds */
int glite_jppsbe_purge_feeds(
	glite_jp_context_t ctx
)
{
	char	*stmt = NULL,*feed = NULL;
	char	*expires;
	glite_jp_error_t	err;
	glite_lbu_Statement	q = NULL;
	int	rows;

	glite_lbu_TimeToDB(time(NULL), &expires);
	memset(&err,0,sizeof err);

	trio_asprintf(&stmt,"select feedid from feeds where expires < %s",expires);

	if ((rows = glite_jp_db_ExecSQL(ctx, stmt, &q)) < 0) {
		err.code = EIO;
		err.desc = "select from feeds";
		glite_jp_stack_error(ctx,&err);
		goto cleanup;
	}

	while ((rows = glite_jp_db_FetchRow(ctx,q,1,NULL,&feed)) > 0) {
		printf("feed %s has expired.\n", feed);
		free(stmt);
		trio_asprintf(&stmt,"delete from fed_jobs where feedid = '%|Ss'",feed);
		if ((rows = glite_jp_db_ExecSQL(ctx, stmt, NULL)) < 0) {
			err.code = EIO;
			err.desc = "delete from fed_jobs";
			glite_jp_stack_error(ctx,&err);
			goto cleanup;
		}
	}

	free(stmt);
	trio_asprintf(&stmt,"delete from feeds where expires < %s",expires);
	if ((rows = glite_jp_db_ExecSQL(ctx, stmt, NULL)) < 0) {
		err.code = EIO;
		err.desc = "select from feeds";
		glite_jp_stack_error(ctx,&err);
		goto cleanup;
	}

cleanup:
	glite_jp_db_FreeStmt(&q);
	free(feed);
	free(stmt);
	free(expires);
	return err.code;
}


/** read stored feed into context */
int glite_jppsbe_read_feeds(
	glite_jp_context_t ctx
)
{
	char	*stmt,*res[5],*expires;
	glite_jp_error_t	err;
	glite_lbu_Statement	q = NULL;
	int	rows;

	stmt = expires = NULL;
	memset(&err,0,sizeof err);
	memset(&res,0,sizeof res);
	err.source = __FUNCTION__;

	glite_lbu_TimeToDB(time(NULL), &expires);
	trio_asprintf(&stmt,"select feedid,destination,expires,cols,query "
			"from feeds "
			"where expires > %s",expires);
	free(expires); expires = NULL;

	if ((rows = glite_jp_db_ExecSQL(ctx, stmt, &q)) < 0) {
		err.code = EIO;
		err.desc = "select from feeds";
		glite_jp_stack_error(ctx,&err);
		goto cleanup;
	}
	free(stmt);

	while ((rows = glite_jp_db_FetchRow(ctx,q,sizeof(res)/sizeof(res[0]),NULL, res)) > 0) {
		struct jpfeed *f = calloc(1,sizeof *f);
		int	n;
		char	*p;

		f->id = res[0]; res[0] = NULL;
		f->destination = res[1]; res[1] = NULL;
		f->expires = glite_lbu_DBToTime(res[2]); free(res[2]); res[2] = NULL;

		n = 0;
		for (p = strtok(res[3],"\n"); p; p = strtok(NULL,"\n")) {
			f->attrs = realloc(f->attrs,(n+2) * sizeof *f->attrs);
			f->attrs[n] = glite_lbu_UnescapeULM(p);
			f->attrs[++n] = NULL;
		}

		n = 0;
		for (p = strtok(res[4],"\n"); p; p = strtok(NULL,"\n")) {
			f->qry = realloc(f->qry,(n+2) * sizeof *f->qry);
			memset(&f->qry[n],0,sizeof *f->qry);
			f->qry[n].attr = glite_lbu_EscapeULM(p);
			p = strtok(NULL,"\n");
			switch (*p) {
				case '=': f->qry[n].op = GLITE_JP_QUERYOP_EQUAL; break;
				case '<': f->qry[n].op = GLITE_JP_QUERYOP_LESS; break;
				case '>': f->qry[n].op = GLITE_JP_QUERYOP_GREATER; break;
				case '!': f->qry[n].op = GLITE_JP_QUERYOP_UNEQUAL; break;
				case 'E': f->qry[n].op = GLITE_JP_QUERYOP_EXISTS; break;
				default: abort(); /* XXX: internal inconsistency */
			}
			p = strtok(NULL,"\n");
			if (f->qry[n].op != GLITE_JP_QUERYOP_EXISTS) 
				f->qry[n].value = glite_lbu_EscapeULM(p);

			memset(&f->qry[++n],0,sizeof *f->qry);
		}
		f->next = ctx->feeds;
		ctx->feeds = f;
	}

	if (rows < 0) {
		err.code = EIO;
		err.desc = "fetch from feeds";
		glite_jp_stack_error(ctx,&err);
		goto cleanup;
	}

cleanup:
	glite_jp_db_FreeStmt(&q);
	free(res[0]); free(res[1]); free(res[2]); free(res[3]); free(res[4]);
	return err.code;
}

int glite_jppsbe_append_tag(
	void *fpctx,
	char *jobid,
	glite_jp_attrval_t *attr
)
{
	void 			*file_be;
	glite_jp_error_t        err;
        glite_jp_context_t      ctx = fpctx;
	memset(&err,0,sizeof err);
        err.source = __FUNCTION__;

	if (glite_jppsbe_open_file(ctx,jobid,"tags",NULL,
                                                O_RDWR|O_CREAT,&file_be)
                        // XXX: tags need reading to check magic number
        ) {
		err.code = EIO;
		err.desc = "cannot open tags file";
		return glite_jp_stack_error(ctx,&err);
        }

	if (tag_append(ctx,file_be,attr))
        {
		err.code = EIO;
                err.desc = "cannot append tag";
                return glite_jp_stack_error(ctx,&err);
        }

	if (glite_jppsbe_close_file(ctx,file_be))
	{
		err.code = EIO;
                err.desc = "cannot close tags file";
		return glite_jp_stack_error(ctx,&err);
        }
	
	return 0;
}


int glite_jppsbe_read_tag(
	void *fpctx,
	const char *jobid,
	const char *attr,
	glite_jp_attrval_t **attrval
)
{
        glite_jp_error_t        err;
        glite_jp_context_t      ctx = fpctx;
	struct tags_handle     	*h;

	memset(&err,0,sizeof err);
        err.source = __FUNCTION__;
	h = malloc(sizeof (*h));
	h->tags = NULL;
	h->n = 0;

	if (glite_jppsbe_open_file(ctx,jobid,"tags",NULL,
                                                O_RDONLY,&(h->bhandle))
                        // XXX: tags need reading to check magic number
        ) {
                err.code = EIO;
                err.desc = "cannot open tags file";
                return glite_jp_stack_error(ctx,&err);
        }

	if (tag_attr(ctx,h,attr,attrval)){
		glite_jp_error_t	*e;
		err.code = EIO;
                err.desc = "cannot read tag";
		glite_jp_stack_error(ctx,&err);		
		e = ctx->error;
		ctx->error = NULL;
		glite_jppsbe_close_file(ctx,h->bhandle);
		ctx->error = e;
		return err.code;
	}

	int i;
	for (i=0; i < h->n; i++){
		free(h->tags[i].name);
		free(h->tags[i].value);
	}
	free(h->tags);

	if (glite_jppsbe_close_file(ctx,h->bhandle))
        {
                err.code = EIO;
                err.desc = "cannot close tags file";
		free(h);
                return glite_jp_stack_error(ctx,&err);
        }

	free(h);
	
	return 0;
}



/* XXX:
- no primary authorization yet
- no concurrency control yet
- partial success in pwrite,append
- "unique" part of jobid is assumed to be unique across bookkeeping servers
- repository versioning not fully implemented yet
*/
