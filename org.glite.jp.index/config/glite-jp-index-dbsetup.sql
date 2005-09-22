create table jobs (
        jobid		char(32)	binary not null,
        dg_jobid        varchar(255)    binary not null,
        ownerid         char(32)        binary not null,
        aclid           char(32)        binary null,

        primary key (jobid),
        unique (dg_jobid),
        index (jobid),
	index (dg_jobid)
);

create table attrs (
       	attrid		char(32)	binary not null,
        name        	varchar(255)	binary not null,
	indexed		int		not null,
	type		char(32)	binary null,

        primary key (attrid),
        index (attrid),
	index (name)
);

create table feeds (
        feedid		char(32)	binary not null,
	state		int		not null,
	source		varchar(255)	not null,
	expires		datetime	not null,
	attrs		mediumblob	null,
	condition	mediumblob	null,

        primary key (feedid),
        index (feedid),
	index (state)
);

create table acls (
        aclid           char(32)        binary not null,
        value           mediumblob      not null,
        refcnt          int             not null,

        primary key (aclid)
);

create table users (
        userid          char(32)        binary not null,
        cert_subj       varchar(255)    binary not null,

        primary key (userid),
        unique (cert_subj)
);


# data tables - created one for each configured attribute, index on
# value is created only for attributes configured to be indexed
#
#create table attr_<attrid> (
#        jobid		char(32)	binary not null,
#        value		varchar(255) 	binary not null,
#	 full_value	mediumblob	not null,
#
#        index (jobid),
#        index (value)
#);

