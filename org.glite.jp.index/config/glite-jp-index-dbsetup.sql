create table jobs (
	`jobid`		char(32)	binary not null,
	`dg_jobid`        varchar(255)    binary not null,
	`ownerid`         char(32)        binary not null,
	`aclid`           char(32)        binary null,
	`ps`		varchar(255)    not null,

	primary key (jobid),
	unique (dg_jobid),
	index (jobid),
	index (dg_jobid)
) character set utf8 collate utf8_bin engine=innodb;

create table attrs (
	`attrid`		char(32)	binary not null,
	`name`        	varchar(255)	binary not null,
	`indexed`		int		not null,
	`type`		char(32)	binary null,

	primary key (attrid),
	index (attrid),
	index (name)
) character set utf8 collate utf8_bin engine=innodb;

create table feeds (
	`uniqueid`	int		auto_increment not null,
	`feedid`		char(32)	binary unique,
	`state`		int		not null,
	`locked`		int		not null,
	`source`		varchar(255)	not null,
	`expires`		datetime,
	`condition`	mediumblob	null,

        primary key (uniqueid),
        index (uniqueid),
        index (feedid),
	index (state)
) character set utf8 collate utf8_bin engine=innodb;

create table acls (
	`aclid`           char(32)        binary not null,
	`value`           mediumblob      not null,
	`refcnt`          int             not null,

        primary key (aclid)
) character set utf8 collate utf8_bin engine=innodb;

create table users (
	`userid`          char(32)        binary not null,
	`cert_subj`       varchar(255)    binary not null,

        primary key (userid),
        unique (cert_subj)
) character set utf8 collate utf8_bin engine=innodb;

# data tables - created one for each configured and indexed attribute,
# in future values of the non-indexed attributes will be stored in attr_values
#
#create table attr_<attrid> (
#        `jobid`		char(32)	binary not null,
#        `value`		varchar(255) 	binary not null,
#        `full_value`	mediumblob	not null,
#        `origin`		int		not null,
#
#        index (jobid),
#        index (value)
#) character set utf8 collate utf8_bin engine=innodb;


# ---- future schema improvements ----

#create table attr_values (
#	`jobid`           char(32),
#	`attrid`          char(32) binary not null,
#	`value`           varchar(255) binary not null,
#	`full_value`      mediumblob not null,
#	`origin`          int not null,
##	`is_multival`     int,
#
##        primary key (jobid, attrid)
#	index (jobid),
#	index (attrid),
#	index (value)
#) character set utf8 collate utf8_bin engine=innodb;

#create table attr_multivalues (
#	`jobid`           char(32),
#	`attrid`          char(32) binary not null,
#	`value`           varchar(255) binary not null,
#	`full_value`      mediumblob not null,
#	`origin`          int not null,
#
#	index (jobid),
#	index (attrid)
#	index (value)
#) character set utf8 collate utf8_bin engine=innodb;
