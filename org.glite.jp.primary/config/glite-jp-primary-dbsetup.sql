create table jobs (
	jobid		char(32)	binary not null,
	dg_jobid	varchar(255)	binary not null,
	owner		char(32)	binary not null,

	reg_time	datetime	not null;
	
	primary key (jobid),
	unique (dg_jobid),
	index (owner),
	index (owner,reg_time)
);

create table files (
	jobid		char(32)	binary not null,
	filename	varchar(255)	binary not null,
	int_path	mediumblob	null,
	ext_url		mediumblob	null,

	state		char(32)	binary not null,
	deadline	datetime	null;
	ul_userid	char(32)	binary not null,

	primary key (jobid,filename),
	index (ext_url)
);

create table attrs (
	jobid		char(32)	binary not null,
	name		varchar(255)	binary not null,
	value		mediumblob	null,

	primary key (jobid,name)
);

create table users (
	userid		char(32)	binary not null,
	cert_subj	varchar(255)	binary not null,

	primary key (userid),
	unique (cert_subj)
);

create table backend_info (
	version		char(32)	binary not null
);
