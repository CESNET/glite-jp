-- MySQL dump 10.8
--
-- Host: localhost    Database: jpis1test
-- ------------------------------------------------------
-- Server version	4.1.7-max-log

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE="NO_AUTO_VALUE_ON_ZERO" */;

--
-- Current Database: `jpis1test`
--

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `jpis1test`;

USE `jpis1test`;

--
-- Table structure for table `acls`
--

DROP TABLE IF EXISTS `acls`;
CREATE TABLE `acls` (
  `aclid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` mediumblob NOT NULL,
  `refcnt` int(11) NOT NULL default '0',
  PRIMARY KEY  (`aclid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `acls`
--


/*!40000 ALTER TABLE `acls` DISABLE KEYS */;
LOCK TABLES `acls` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `acls` ENABLE KEYS */;

--
-- Table structure for table `attr_1005bc536e987ca1e027a5b8f84c9a67`
--

DROP TABLE IF EXISTS `attr_1005bc536e987ca1e027a5b8f84c9a67`;
CREATE TABLE `attr_1005bc536e987ca1e027a5b8f84c9a67` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_1005bc536e987ca1e027a5b8f84c9a67`
--


/*!40000 ALTER TABLE `attr_1005bc536e987ca1e027a5b8f84c9a67` DISABLE KEYS */;
LOCK TABLES `attr_1005bc536e987ca1e027a5b8f84c9a67` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_1005bc536e987ca1e027a5b8f84c9a67` ENABLE KEYS */;

--
-- Table structure for table `attr_12aaad5454b6e3e44cb4f4a432336af4`
--

DROP TABLE IF EXISTS `attr_12aaad5454b6e3e44cb4f4a432336af4`;
CREATE TABLE `attr_12aaad5454b6e3e44cb4f4a432336af4` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_12aaad5454b6e3e44cb4f4a432336af4`
--


/*!40000 ALTER TABLE `attr_12aaad5454b6e3e44cb4f4a432336af4` DISABLE KEYS */;
LOCK TABLES `attr_12aaad5454b6e3e44cb4f4a432336af4` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_12aaad5454b6e3e44cb4f4a432336af4` ENABLE KEYS */;

--
-- Table structure for table `attr_29d3ef83b39b8cf690e8113df316835a`
--

DROP TABLE IF EXISTS `attr_29d3ef83b39b8cf690e8113df316835a`;
CREATE TABLE `attr_29d3ef83b39b8cf690e8113df316835a` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_29d3ef83b39b8cf690e8113df316835a`
--


/*!40000 ALTER TABLE `attr_29d3ef83b39b8cf690e8113df316835a` DISABLE KEYS */;
LOCK TABLES `attr_29d3ef83b39b8cf690e8113df316835a` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_29d3ef83b39b8cf690e8113df316835a` ENABLE KEYS */;

--
-- Table structure for table `attr_5023d8d4cc249460dd947a878153027f`
--

DROP TABLE IF EXISTS `attr_5023d8d4cc249460dd947a878153027f`;
CREATE TABLE `attr_5023d8d4cc249460dd947a878153027f` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_5023d8d4cc249460dd947a878153027f`
--


/*!40000 ALTER TABLE `attr_5023d8d4cc249460dd947a878153027f` DISABLE KEYS */;
LOCK TABLES `attr_5023d8d4cc249460dd947a878153027f` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_5023d8d4cc249460dd947a878153027f` ENABLE KEYS */;

--
-- Table structure for table `attr_52942b8c70bab8491ab5d3b9713d79f5`
--

DROP TABLE IF EXISTS `attr_52942b8c70bab8491ab5d3b9713d79f5`;
CREATE TABLE `attr_52942b8c70bab8491ab5d3b9713d79f5` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_52942b8c70bab8491ab5d3b9713d79f5`
--


/*!40000 ALTER TABLE `attr_52942b8c70bab8491ab5d3b9713d79f5` DISABLE KEYS */;
LOCK TABLES `attr_52942b8c70bab8491ab5d3b9713d79f5` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_52942b8c70bab8491ab5d3b9713d79f5` ENABLE KEYS */;

--
-- Table structure for table `attr_6bc44144bf813a2ad7d67cb2adbdaf42`
--

DROP TABLE IF EXISTS `attr_6bc44144bf813a2ad7d67cb2adbdaf42`;
CREATE TABLE `attr_6bc44144bf813a2ad7d67cb2adbdaf42` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_6bc44144bf813a2ad7d67cb2adbdaf42`
--


/*!40000 ALTER TABLE `attr_6bc44144bf813a2ad7d67cb2adbdaf42` DISABLE KEYS */;
LOCK TABLES `attr_6bc44144bf813a2ad7d67cb2adbdaf42` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_6bc44144bf813a2ad7d67cb2adbdaf42` ENABLE KEYS */;

--
-- Table structure for table `attr_760a84e0ff89fa3f4e96ec82adfd92f1`
--

DROP TABLE IF EXISTS `attr_760a84e0ff89fa3f4e96ec82adfd92f1`;
CREATE TABLE `attr_760a84e0ff89fa3f4e96ec82adfd92f1` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_760a84e0ff89fa3f4e96ec82adfd92f1`
--


/*!40000 ALTER TABLE `attr_760a84e0ff89fa3f4e96ec82adfd92f1` DISABLE KEYS */;
LOCK TABLES `attr_760a84e0ff89fa3f4e96ec82adfd92f1` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_760a84e0ff89fa3f4e96ec82adfd92f1` ENABLE KEYS */;

--
-- Table structure for table `attr_7c3be9defcbcf9f0e7890600d9c204ac`
--

DROP TABLE IF EXISTS `attr_7c3be9defcbcf9f0e7890600d9c204ac`;
CREATE TABLE `attr_7c3be9defcbcf9f0e7890600d9c204ac` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_7c3be9defcbcf9f0e7890600d9c204ac`
--


/*!40000 ALTER TABLE `attr_7c3be9defcbcf9f0e7890600d9c204ac` DISABLE KEYS */;
LOCK TABLES `attr_7c3be9defcbcf9f0e7890600d9c204ac` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_7c3be9defcbcf9f0e7890600d9c204ac` ENABLE KEYS */;

--
-- Table structure for table `attr_862e3dd7c5da90c9a659a32a41f63af8`
--

DROP TABLE IF EXISTS `attr_862e3dd7c5da90c9a659a32a41f63af8`;
CREATE TABLE `attr_862e3dd7c5da90c9a659a32a41f63af8` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_862e3dd7c5da90c9a659a32a41f63af8`
--


/*!40000 ALTER TABLE `attr_862e3dd7c5da90c9a659a32a41f63af8` DISABLE KEYS */;
LOCK TABLES `attr_862e3dd7c5da90c9a659a32a41f63af8` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_862e3dd7c5da90c9a659a32a41f63af8` ENABLE KEYS */;

--
-- Table structure for table `attr_982d06bdc65d3a4240b36a060a09886e`
--

DROP TABLE IF EXISTS `attr_982d06bdc65d3a4240b36a060a09886e`;
CREATE TABLE `attr_982d06bdc65d3a4240b36a060a09886e` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_982d06bdc65d3a4240b36a060a09886e`
--


/*!40000 ALTER TABLE `attr_982d06bdc65d3a4240b36a060a09886e` DISABLE KEYS */;
LOCK TABLES `attr_982d06bdc65d3a4240b36a060a09886e` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_982d06bdc65d3a4240b36a060a09886e` ENABLE KEYS */;

--
-- Table structure for table `attr_9892f81a8175c09bd00afcb152f510ad`
--

DROP TABLE IF EXISTS `attr_9892f81a8175c09bd00afcb152f510ad`;
CREATE TABLE `attr_9892f81a8175c09bd00afcb152f510ad` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_9892f81a8175c09bd00afcb152f510ad`
--


/*!40000 ALTER TABLE `attr_9892f81a8175c09bd00afcb152f510ad` DISABLE KEYS */;
LOCK TABLES `attr_9892f81a8175c09bd00afcb152f510ad` WRITE;
INSERT INTO `attr_9892f81a8175c09bd00afcb152f510ad` VALUES ('593e62a063231f8c623b74406b3e12b0','CertSubj','S:7201:F::CertSubj',3),('9276789a0093ad44457655ef03ade36a','CertSubj','S:7201:S::CertSubj',2);
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_9892f81a8175c09bd00afcb152f510ad` ENABLE KEYS */;

--
-- Table structure for table `attr_9a812abe1262a90858b7be792f198596`
--

DROP TABLE IF EXISTS `attr_9a812abe1262a90858b7be792f198596`;
CREATE TABLE `attr_9a812abe1262a90858b7be792f198596` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_9a812abe1262a90858b7be792f198596`
--


/*!40000 ALTER TABLE `attr_9a812abe1262a90858b7be792f198596` DISABLE KEYS */;
LOCK TABLES `attr_9a812abe1262a90858b7be792f198596` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_9a812abe1262a90858b7be792f198596` ENABLE KEYS */;

--
-- Table structure for table `attr_a1e9e0a1b7943cc041fefb5da65868f9`
--

DROP TABLE IF EXISTS `attr_a1e9e0a1b7943cc041fefb5da65868f9`;
CREATE TABLE `attr_a1e9e0a1b7943cc041fefb5da65868f9` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_a1e9e0a1b7943cc041fefb5da65868f9`
--


/*!40000 ALTER TABLE `attr_a1e9e0a1b7943cc041fefb5da65868f9` DISABLE KEYS */;
LOCK TABLES `attr_a1e9e0a1b7943cc041fefb5da65868f9` WRITE;
INSERT INTO `attr_a1e9e0a1b7943cc041fefb5da65868f9` VALUES ('593e62a063231f8c623b74406b3e12b0','Done','S:7201:F::Done',3),('9276789a0093ad44457655ef03ade36a','Ready','S:7201:S::Ready',1);
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_a1e9e0a1b7943cc041fefb5da65868f9` ENABLE KEYS */;

--
-- Table structure for table `attr_a9c522a79597e1bfd2bd687d42d557b7`
--

DROP TABLE IF EXISTS `attr_a9c522a79597e1bfd2bd687d42d557b7`;
CREATE TABLE `attr_a9c522a79597e1bfd2bd687d42d557b7` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_a9c522a79597e1bfd2bd687d42d557b7`
--


/*!40000 ALTER TABLE `attr_a9c522a79597e1bfd2bd687d42d557b7` DISABLE KEYS */;
LOCK TABLES `attr_a9c522a79597e1bfd2bd687d42d557b7` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_a9c522a79597e1bfd2bd687d42d557b7` ENABLE KEYS */;

--
-- Table structure for table `attr_c47f78255056386d2b3da6d506d1f244`
--

DROP TABLE IF EXISTS `attr_c47f78255056386d2b3da6d506d1f244`;
CREATE TABLE `attr_c47f78255056386d2b3da6d506d1f244` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_c47f78255056386d2b3da6d506d1f244`
--


/*!40000 ALTER TABLE `attr_c47f78255056386d2b3da6d506d1f244` DISABLE KEYS */;
LOCK TABLES `attr_c47f78255056386d2b3da6d506d1f244` WRITE;
INSERT INTO `attr_c47f78255056386d2b3da6d506d1f244` VALUES ('593e62a063231f8c623b74406b3e12b0','VOCE','S:7201:F::VOCE',3);
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_c47f78255056386d2b3da6d506d1f244` ENABLE KEYS */;

--
-- Table structure for table `attr_d193237d94c17244ebba4ce049759371`
--

DROP TABLE IF EXISTS `attr_d193237d94c17244ebba4ce049759371`;
CREATE TABLE `attr_d193237d94c17244ebba4ce049759371` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_d193237d94c17244ebba4ce049759371`
--


/*!40000 ALTER TABLE `attr_d193237d94c17244ebba4ce049759371` DISABLE KEYS */;
LOCK TABLES `attr_d193237d94c17244ebba4ce049759371` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_d193237d94c17244ebba4ce049759371` ENABLE KEYS */;

--
-- Table structure for table `attr_e019a506c890326966714893ac3e8cf5`
--

DROP TABLE IF EXISTS `attr_e019a506c890326966714893ac3e8cf5`;
CREATE TABLE `attr_e019a506c890326966714893ac3e8cf5` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_e019a506c890326966714893ac3e8cf5`
--


/*!40000 ALTER TABLE `attr_e019a506c890326966714893ac3e8cf5` DISABLE KEYS */;
LOCK TABLES `attr_e019a506c890326966714893ac3e8cf5` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_e019a506c890326966714893ac3e8cf5` ENABLE KEYS */;

--
-- Table structure for table `attr_e26a86a86bfc6799461d999860e57d81`
--

DROP TABLE IF EXISTS `attr_e26a86a86bfc6799461d999860e57d81`;
CREATE TABLE `attr_e26a86a86bfc6799461d999860e57d81` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_e26a86a86bfc6799461d999860e57d81`
--


/*!40000 ALTER TABLE `attr_e26a86a86bfc6799461d999860e57d81` DISABLE KEYS */;
LOCK TABLES `attr_e26a86a86bfc6799461d999860e57d81` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_e26a86a86bfc6799461d999860e57d81` ENABLE KEYS */;

--
-- Table structure for table `attrs`
--

DROP TABLE IF EXISTS `attrs`;
CREATE TABLE `attrs` (
  `attrid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `name` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `indexed` int(11) NOT NULL default '0',
  `type` varchar(32) character set latin1 collate latin1_bin default NULL,
  PRIMARY KEY  (`attrid`),
  KEY `attrid` (`attrid`),
  KEY `name` (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attrs`
--


/*!40000 ALTER TABLE `attrs` DISABLE KEYS */;
LOCK TABLES `attrs` WRITE;
INSERT INTO `attrs` VALUES ('52942b8c70bab8491ab5d3b9713d79f5','http://egee.cesnet.cz/en/Schema/JP/System:owner',1,'mediumblob'),('6bc44144bf813a2ad7d67cb2adbdaf42','http://egee.cesnet.cz/en/Schema/JP/System:jobId',1,'mediumblob'),('862e3dd7c5da90c9a659a32a41f63af8','http://egee.cesnet.cz/en/Schema/JP/System:regtime',0,'mediumblob'),('9892f81a8175c09bd00afcb152f510ad','http://egee.cesnet.cz/en/Schema/LB/Attributes:user',1,'mediumblob'),('e019a506c890326966714893ac3e8cf5','http://egee.cesnet.cz/en/Schema/LB/Attributes:aTag',0,'mediumblob'),('d193237d94c17244ebba4ce049759371','http://egee.cesnet.cz/en/Schema/LB/Attributes:eNodes',0,'mediumblob'),('5023d8d4cc249460dd947a878153027f','http://egee.cesnet.cz/en/Schema/LB/Attributes:RB',1,'mediumblob'),('c47f78255056386d2b3da6d506d1f244','http://egee.cesnet.cz/en/Schema/LB/Attributes:CE',1,'mediumblob'),('29d3ef83b39b8cf690e8113df316835a','http://egee.cesnet.cz/en/Schema/LB/Attributes:UIHost',1,'mediumblob'),('a9c522a79597e1bfd2bd687d42d557b7','http://egee.cesnet.cz/en/Schema/LB/Attributes:CPUTime',0,'mediumblob'),('12aaad5454b6e3e44cb4f4a432336af4','http://egee.cesnet.cz/en/Schema/LB/Attributes:NProc',0,'mediumblob'),('a1e9e0a1b7943cc041fefb5da65868f9','http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus',1,'mediumblob'),('760a84e0ff89fa3f4e96ec82adfd92f1','http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatusDate',0,'mediumblob'),('9a812abe1262a90858b7be792f198596','http://egee.cesnet.cz/en/Schema/LB/Attributes:retryCount',0,'mediumblob'),('e26a86a86bfc6799461d999860e57d81','http://egee.cesnet.cz/en/Schema/LB/Attributes:jobType',0,'mediumblob'),('1005bc536e987ca1e027a5b8f84c9a67','http://egee.cesnet.cz/en/Schema/LB/Attributes:nsubjobs',0,'mediumblob'),('7c3be9defcbcf9f0e7890600d9c204ac','http://egee.cesnet.cz/en/Schema/LB/Attributes:lastStatusHistory',0,'mediumblob'),('982d06bdc65d3a4240b36a060a09886e','http://egee.cesnet.cz/en/Schema/LB/Attributes:fullStatusHistory',0,'mediumblob');
UNLOCK TABLES;
/*!40000 ALTER TABLE `attrs` ENABLE KEYS */;

--
-- Table structure for table `feeds`
--

DROP TABLE IF EXISTS `feeds`;
CREATE TABLE `feeds` (
  `uniqueid` int(11) NOT NULL auto_increment,
  `feedid` varchar(32) character set latin1 collate latin1_bin default NULL,
  `state` int(11) NOT NULL default '0',
  `locked` int(11) NOT NULL default '0',
  `source` varchar(255) NOT NULL default '',
  `expires` datetime default NULL,
  `conditions` mediumblob,
  PRIMARY KEY  (`uniqueid`),
  UNIQUE KEY `feedid` (`feedid`),
  KEY `uniqueid` (`uniqueid`),
  KEY `feedid_2` (`feedid`),
  KEY `state` (`state`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `feeds`
--


/*!40000 ALTER TABLE `feeds` DISABLE KEYS */;
LOCK TABLES `feeds` WRITE;
INSERT INTO `feeds` VALUES (93,'12345',8,0,'http://localhost:8901','2005-10-14 10:48:27','COND2');
UNLOCK TABLES;
/*!40000 ALTER TABLE `feeds` ENABLE KEYS */;

--
-- Table structure for table `jobs`
--

DROP TABLE IF EXISTS `jobs`;
CREATE TABLE `jobs` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `dg_jobid` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `ownerid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `aclid` varchar(32) character set latin1 collate latin1_bin default NULL,
  `ps` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`jobid`),
  UNIQUE KEY `dg_jobid` (`dg_jobid`),
  KEY `jobid` (`jobid`),
  KEY `dg_jobid_2` (`dg_jobid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `jobs`
--


/*!40000 ALTER TABLE `jobs` DISABLE KEYS */;
LOCK TABLES `jobs` WRITE;
INSERT INTO `jobs` VALUES ('593e62a063231f8c623b74406b3e12b0','https://localhost:7846/pokus1','5864429d57da18e4ecf9ea366c6b2c9c',NULL,'http://localhost:8901'),('9276789a0093ad44457655ef03ade36a','https://localhost:7846/pokus2','9996d295b9e10ce182983b258b280779',NULL,'http://localhost:8901');
UNLOCK TABLES;
/*!40000 ALTER TABLE `jobs` ENABLE KEYS */;

--
-- Table structure for table `users`
--

DROP TABLE IF EXISTS `users`;
CREATE TABLE `users` (
  `userid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `cert_subj` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  PRIMARY KEY  (`userid`),
  UNIQUE KEY `cert_subj` (`cert_subj`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `users`
--


/*!40000 ALTER TABLE `users` DISABLE KEYS */;
LOCK TABLES `users` WRITE;
INSERT INTO `users` VALUES ('5864429d57da18e4ecf9ea366c6b2c9c','/O=CESNET/O=Masaryk University/CN=Milos Mulac'),('9996d295b9e10ce182983b258b280779','OwnerName');
UNLOCK TABLES;
/*!40000 ALTER TABLE `users` ENABLE KEYS */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;

