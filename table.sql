CREATE TABLE IF NOT EXISTS `member` (
  `uid` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT 'ID',
  `username` varchar(20) NOT NULL,
  `password` varchar(33) NOT NULL DEFAULT '',
  `email` varchar(30) NOT NULL,
  `ip` int(11) unsigned NOT NULL DEFAULT '0',
  `sex` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `reg_time` int(11) unsigned NOT NULL DEFAULT '0',
  `c_status` tinyint(1) unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`uid`),
  KEY `username` (`username`)
) ENGINE=MyISAM