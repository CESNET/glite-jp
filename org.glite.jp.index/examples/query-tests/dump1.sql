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
-- Table structure for table `attr_ac7ea0b2cd17deedbc569733597059ae`
--

DROP TABLE IF EXISTS `attr_ac7ea0b2cd17deedbc569733597059ae`;
CREATE TABLE `attr_ac7ea0b2cd17deedbc569733597059ae` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_ac7ea0b2cd17deedbc569733597059ae`
--


/*!40000 ALTER TABLE `attr_ac7ea0b2cd17deedbc569733597059ae` DISABLE KEYS */;
LOCK TABLES `attr_ac7ea0b2cd17deedbc569733597059ae` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_ac7ea0b2cd17deedbc569733597059ae` ENABLE KEYS */;

--
-- Table structure for table `attr_5de12c1776c3130b9d27a7502a13e11c`
--

DROP TABLE IF EXISTS `attr_5de12c1776c3130b9d27a7502a13e11c`;
CREATE TABLE `attr_5de12c1776c3130b9d27a7502a13e11c` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_5de12c1776c3130b9d27a7502a13e11c`
--


/*!40000 ALTER TABLE `attr_5de12c1776c3130b9d27a7502a13e11c` DISABLE KEYS */;
LOCK TABLES `attr_5de12c1776c3130b9d27a7502a13e11c` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_5de12c1776c3130b9d27a7502a13e11c` ENABLE KEYS */;

--
-- Table structure for table `attr_f496f5d872a2d04ee626045477f340db`
--

DROP TABLE IF EXISTS `attr_f496f5d872a2d04ee626045477f340db`;
CREATE TABLE `attr_f496f5d872a2d04ee626045477f340db` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_f496f5d872a2d04ee626045477f340db`
--


/*!40000 ALTER TABLE `attr_f496f5d872a2d04ee626045477f340db` DISABLE KEYS */;
LOCK TABLES `attr_f496f5d872a2d04ee626045477f340db` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_f496f5d872a2d04ee626045477f340db` ENABLE KEYS */;

--
-- Table structure for table `attr_34d7a9e823c6948d525362d2709bdfcd`
--

DROP TABLE IF EXISTS `attr_34d7a9e823c6948d525362d2709bdfcd`;
CREATE TABLE `attr_34d7a9e823c6948d525362d2709bdfcd` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_34d7a9e823c6948d525362d2709bdfcd`
--


/*!40000 ALTER TABLE `attr_34d7a9e823c6948d525362d2709bdfcd` DISABLE KEYS */;
LOCK TABLES `attr_34d7a9e823c6948d525362d2709bdfcd` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_34d7a9e823c6948d525362d2709bdfcd` ENABLE KEYS */;

--
-- Table structure for table `attr_824794b00ee73be550f893b99ceaa643`
--

DROP TABLE IF EXISTS `attr_824794b00ee73be550f893b99ceaa643`;
CREATE TABLE `attr_824794b00ee73be550f893b99ceaa643` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_824794b00ee73be550f893b99ceaa643`
--


/*!40000 ALTER TABLE `attr_824794b00ee73be550f893b99ceaa643` DISABLE KEYS */;
LOCK TABLES `attr_824794b00ee73be550f893b99ceaa643` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_824794b00ee73be550f893b99ceaa643` ENABLE KEYS */;

--
-- Table structure for table `attr_ac1e0e146f3e1bee11d6e40d07e60abb`
--

DROP TABLE IF EXISTS `attr_ac1e0e146f3e1bee11d6e40d07e60abb`;
CREATE TABLE `attr_ac1e0e146f3e1bee11d6e40d07e60abb` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_ac1e0e146f3e1bee11d6e40d07e60abb`
--


/*!40000 ALTER TABLE `attr_ac1e0e146f3e1bee11d6e40d07e60abb` DISABLE KEYS */;
LOCK TABLES `attr_ac1e0e146f3e1bee11d6e40d07e60abb` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_ac1e0e146f3e1bee11d6e40d07e60abb` ENABLE KEYS */;

--
-- Table structure for table `attr_7636d6368c1cf53bc5511241cac9751f`
--

DROP TABLE IF EXISTS `attr_7636d6368c1cf53bc5511241cac9751f`;
CREATE TABLE `attr_7636d6368c1cf53bc5511241cac9751f` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_7636d6368c1cf53bc5511241cac9751f`
--


/*!40000 ALTER TABLE `attr_7636d6368c1cf53bc5511241cac9751f` DISABLE KEYS */;
LOCK TABLES `attr_7636d6368c1cf53bc5511241cac9751f` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_7636d6368c1cf53bc5511241cac9751f` ENABLE KEYS */;

--
-- Table structure for table `attr_64ea5318d74aca823630ba9ca38971e0`
--

DROP TABLE IF EXISTS `attr_64ea5318d74aca823630ba9ca38971e0`;
CREATE TABLE `attr_64ea5318d74aca823630ba9ca38971e0` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_64ea5318d74aca823630ba9ca38971e0`
--


/*!40000 ALTER TABLE `attr_64ea5318d74aca823630ba9ca38971e0` DISABLE KEYS */;
LOCK TABLES `attr_64ea5318d74aca823630ba9ca38971e0` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_64ea5318d74aca823630ba9ca38971e0` ENABLE KEYS */;

--
-- Table structure for table `attr_81a1d6b95da954e977f22f78417b98a8`
--

DROP TABLE IF EXISTS `attr_81a1d6b95da954e977f22f78417b98a8`;
CREATE TABLE `attr_81a1d6b95da954e977f22f78417b98a8` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_81a1d6b95da954e977f22f78417b98a8`
--


/*!40000 ALTER TABLE `attr_81a1d6b95da954e977f22f78417b98a8` DISABLE KEYS */;
LOCK TABLES `attr_81a1d6b95da954e977f22f78417b98a8` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_81a1d6b95da954e977f22f78417b98a8` ENABLE KEYS */;

--
-- Table structure for table `attr_e6c0fb3b99f16296db2623c02f0d5c6f`
--

DROP TABLE IF EXISTS `attr_e6c0fb3b99f16296db2623c02f0d5c6f`;
CREATE TABLE `attr_e6c0fb3b99f16296db2623c02f0d5c6f` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_e6c0fb3b99f16296db2623c02f0d5c6f`
--


/*!40000 ALTER TABLE `attr_e6c0fb3b99f16296db2623c02f0d5c6f` DISABLE KEYS */;
LOCK TABLES `attr_e6c0fb3b99f16296db2623c02f0d5c6f` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_e6c0fb3b99f16296db2623c02f0d5c6f` ENABLE KEYS */;

--
-- Table structure for table `attr_474e4207c49813e09915732c80c0e1cc`
--

DROP TABLE IF EXISTS `attr_474e4207c49813e09915732c80c0e1cc`;
CREATE TABLE `attr_474e4207c49813e09915732c80c0e1cc` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_474e4207c49813e09915732c80c0e1cc`
--


/*!40000 ALTER TABLE `attr_474e4207c49813e09915732c80c0e1cc` DISABLE KEYS */;
LOCK TABLES `attr_474e4207c49813e09915732c80c0e1cc` WRITE;
INSERT INTO `attr_474e4207c49813e09915732c80c0e1cc` VALUES ('593e62a063231f8c623b74406b3e12b0','CertSubj','S:7201:F::CertSubj',3),('9276789a0093ad44457655ef03ade36a','CertSubj','S:7201:S::CertSubj',2);
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_474e4207c49813e09915732c80c0e1cc` ENABLE KEYS */;

--
-- Table structure for table `attr_e2d5742f6e917ea2e949d49b9fa0c1b3`
--

DROP TABLE IF EXISTS `attr_e2d5742f6e917ea2e949d49b9fa0c1b3`;
CREATE TABLE `attr_e2d5742f6e917ea2e949d49b9fa0c1b3` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_e2d5742f6e917ea2e949d49b9fa0c1b3`
--


/*!40000 ALTER TABLE `attr_e2d5742f6e917ea2e949d49b9fa0c1b3` DISABLE KEYS */;
LOCK TABLES `attr_e2d5742f6e917ea2e949d49b9fa0c1b3` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_e2d5742f6e917ea2e949d49b9fa0c1b3` ENABLE KEYS */;

--
-- Table structure for table `attr_941ae4f469950ed63ad19822dbcf5427`
--

DROP TABLE IF EXISTS `attr_941ae4f469950ed63ad19822dbcf5427`;
CREATE TABLE `attr_941ae4f469950ed63ad19822dbcf5427` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_941ae4f469950ed63ad19822dbcf5427`
--


/*!40000 ALTER TABLE `attr_941ae4f469950ed63ad19822dbcf5427` DISABLE KEYS */;
LOCK TABLES `attr_941ae4f469950ed63ad19822dbcf5427` WRITE;
INSERT INTO `attr_941ae4f469950ed63ad19822dbcf5427` VALUES ('593e62a063231f8c623b74406b3e12b0','Done','S:7201:F::Done',3),('9276789a0093ad44457655ef03ade36a','Ready','S:7201:S::Ready',1);
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_941ae4f469950ed63ad19822dbcf5427` ENABLE KEYS */;

--
-- Table structure for table `attr_97b3c128ab54e621c806b9ffe5c45185`
--

DROP TABLE IF EXISTS `attr_97b3c128ab54e621c806b9ffe5c45185`;
CREATE TABLE `attr_97b3c128ab54e621c806b9ffe5c45185` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_97b3c128ab54e621c806b9ffe5c45185`
--


/*!40000 ALTER TABLE `attr_97b3c128ab54e621c806b9ffe5c45185` DISABLE KEYS */;
LOCK TABLES `attr_97b3c128ab54e621c806b9ffe5c45185` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_97b3c128ab54e621c806b9ffe5c45185` ENABLE KEYS */;

--
-- Table structure for table `attr_04ffb63c6978549209734fc02e8d688d`
--

DROP TABLE IF EXISTS `attr_04ffb63c6978549209734fc02e8d688d`;
CREATE TABLE `attr_04ffb63c6978549209734fc02e8d688d` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_04ffb63c6978549209734fc02e8d688d`
--


/*!40000 ALTER TABLE `attr_04ffb63c6978549209734fc02e8d688d` DISABLE KEYS */;
LOCK TABLES `attr_04ffb63c6978549209734fc02e8d688d` WRITE;
INSERT INTO `attr_04ffb63c6978549209734fc02e8d688d` VALUES ('593e62a063231f8c623b74406b3e12b0','VOCE','S:7201:F::VOCE',3);
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_04ffb63c6978549209734fc02e8d688d` ENABLE KEYS */;

--
-- Table structure for table `attr_48f1a123884d6e24fe205c0f1c60c686`
--

DROP TABLE IF EXISTS `attr_48f1a123884d6e24fe205c0f1c60c686`;
CREATE TABLE `attr_48f1a123884d6e24fe205c0f1c60c686` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_48f1a123884d6e24fe205c0f1c60c686`
--


/*!40000 ALTER TABLE `attr_48f1a123884d6e24fe205c0f1c60c686` DISABLE KEYS */;
LOCK TABLES `attr_48f1a123884d6e24fe205c0f1c60c686` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_48f1a123884d6e24fe205c0f1c60c686` ENABLE KEYS */;

--
-- Table structure for table `attr_52df8110aad9f80fd33a96073bfe58e7`
--

DROP TABLE IF EXISTS `attr_52df8110aad9f80fd33a96073bfe58e7`;
CREATE TABLE `attr_52df8110aad9f80fd33a96073bfe58e7` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_52df8110aad9f80fd33a96073bfe58e7`
--


/*!40000 ALTER TABLE `attr_52df8110aad9f80fd33a96073bfe58e7` DISABLE KEYS */;
LOCK TABLES `attr_52df8110aad9f80fd33a96073bfe58e7` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_52df8110aad9f80fd33a96073bfe58e7` ENABLE KEYS */;

--
-- Table structure for table `attr_47a0c544b03cd51e37f3ad7f9e0a0a62`
--

DROP TABLE IF EXISTS `attr_47a0c544b03cd51e37f3ad7f9e0a0a62`;
CREATE TABLE `attr_47a0c544b03cd51e37f3ad7f9e0a0a62` (
  `jobid` varchar(32) character set latin1 collate latin1_bin NOT NULL default '',
  `value` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `full_value` mediumblob NOT NULL,
  `origin` int(11) NOT NULL default '0',
  KEY `jobid` (`jobid`),
  KEY `value` (`value`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `attr_47a0c544b03cd51e37f3ad7f9e0a0a62`
--


/*!40000 ALTER TABLE `attr_47a0c544b03cd51e37f3ad7f9e0a0a62` DISABLE KEYS */;
LOCK TABLES `attr_47a0c544b03cd51e37f3ad7f9e0a0a62` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `attr_47a0c544b03cd51e37f3ad7f9e0a0a62` ENABLE KEYS */;

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
INSERT INTO `attrs` VALUES ('824794b00ee73be550f893b99ceaa643','http://egee.cesnet.cz/en/Schema/JP/System:owner',1,'mediumblob'),('ac1e0e146f3e1bee11d6e40d07e60abb','http://egee.cesnet.cz/en/Schema/JP/System:jobId',1,'mediumblob'),('81a1d6b95da954e977f22f78417b98a8','http://egee.cesnet.cz/en/Schema/JP/System:regtime',0,'mediumblob'),('474e4207c49813e09915732c80c0e1cc','http://egee.cesnet.cz/en/Schema/LB/Attributes:user',1,'mediumblob'),('52df8110aad9f80fd33a96073bfe58e7','http://egee.cesnet.cz/en/Schema/LB/Attributes:aTag',0,'mediumblob'),('48f1a123884d6e24fe205c0f1c60c686','http://egee.cesnet.cz/en/Schema/LB/Attributes:eNodes',0,'mediumblob'),('34d7a9e823c6948d525362d2709bdfcd','http://egee.cesnet.cz/en/Schema/LB/Attributes:RB',1,'mediumblob'),('04ffb63c6978549209734fc02e8d688d','http://egee.cesnet.cz/en/Schema/LB/Attributes:CE',1,'mediumblob'),('f496f5d872a2d04ee626045477f340db','http://egee.cesnet.cz/en/Schema/LB/Attributes:UIHost',1,'mediumblob'),('97b3c128ab54e621c806b9ffe5c45185','http://egee.cesnet.cz/en/Schema/LB/Attributes:CPUTime',0,'mediumblob'),('5de12c1776c3130b9d27a7502a13e11c','http://egee.cesnet.cz/en/Schema/LB/Attributes:NProc',0,'mediumblob'),('941ae4f469950ed63ad19822dbcf5427','http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus',1,'mediumblob'),('7636d6368c1cf53bc5511241cac9751f','http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatusDate',0,'mediumblob'),('e2d5742f6e917ea2e949d49b9fa0c1b3','http://egee.cesnet.cz/en/Schema/LB/Attributes:retryCount',0,'mediumblob'),('47a0c544b03cd51e37f3ad7f9e0a0a62','http://egee.cesnet.cz/en/Schema/LB/Attributes:jobType',0,'mediumblob'),('ac7ea0b2cd17deedbc569733597059ae','http://egee.cesnet.cz/en/Schema/LB/Attributes:nsubjobs',0,'mediumblob'),('64ea5318d74aca823630ba9ca38971e0','http://egee.cesnet.cz/en/Schema/LB/Attributes:lastStatusHistory',0,'mediumblob'),('e6c0fb3b99f16296db2623c02f0d5c6f','http://egee.cesnet.cz/en/Schema/LB/Attributes:fullStatusHistory',0,'mediumblob');
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
  `condition` mediumblob,
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

