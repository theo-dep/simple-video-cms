CREATE DATABASE `video`;
USE `video`;


DROP TABLE IF EXISTS `admins`;
CREATE TABLE `admins` (
  `username` varchar(20) NOT NULL,
  `password` varchar(128) DEFAULT NULL,
  PRIMARY KEY (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;


DROP TABLE IF EXISTS `users`;
CREATE TABLE `users` (
  `username` varchar(20) NOT NULL,
  `password` varchar(128) DEFAULT NULL,
  PRIMARY KEY (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;


DROP TABLE IF EXISTS `sessions`;
CREATE TABLE `sessions` (
  `session_id` varchar(20) NOT NULL,
  `admin_name` varchar(20) DEFAULT NULL,
  `user_name` varchar(20) DEFAULT NULL,
  PRIMARY KEY (`session_id`),
  CONSTRAINT `sessions_ibfk_1` FOREIGN KEY (`admin_name`) REFERENCES `admins` (`username`) ON DELETE CASCADE,
  CONSTRAINT `sessions_ibfk_2` FOREIGN KEY (`user_name`) REFERENCES `users` (`username`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;


DROP TABLE IF EXISTS `videos`;
CREATE TABLE `videos` (
  `video_ID` varchar(50) NOT NULL,
  `video_title` varchar(200) DEFAULT NULL,
  `uploader` varchar(20) DEFAULT NULL,
  `view_count` varchar(10) DEFAULT NULL,
  `upload_date` date DEFAULT NULL,
  PRIMARY KEY (`video_ID`),
  KEY `uploader` (`uploader`),
  CONSTRAINT `videos_ibfk_1` FOREIGN KEY (`uploader`) REFERENCES `users` (`username`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;


DROP TABLE IF EXISTS `favourites`;
CREATE TABLE `favourites` (
  `username` varchar(20) NOT NULL,
  `video_ID` varchar(50) NOT NULL,
  PRIMARY KEY (`username`,`video_ID`),
  KEY `video_ID` (`video_ID`),
  CONSTRAINT `favourites_ibfk_1` FOREIGN KEY (`username`) REFERENCES `users` (`username`) ON DELETE CASCADE,
  CONSTRAINT `favourites_ibfk_2` FOREIGN KEY (`video_ID`) REFERENCES `videos` (`video_ID`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;


DROP TABLE IF EXISTS `flags`;
CREATE TABLE `flags` (
  `video_ID` varchar(50) NOT NULL,
  `username` varchar(20) NOT NULL,
  PRIMARY KEY (`video_ID`,`username`),
  KEY `username` (`username`),
  CONSTRAINT `flags_ibfk_1` FOREIGN KEY (`video_ID`) REFERENCES `videos` (`video_ID`) ON DELETE CASCADE,
  CONSTRAINT `flags_ibfk_2` FOREIGN KEY (`username`) REFERENCES `users` (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;


DROP TABLE IF EXISTS `watched`;
CREATE TABLE `watched` (
  `video_ID` varchar(50) NOT NULL,
  `username` varchar(20) NOT NULL,
  `count` varchar(10) DEFAULT NULL,
  PRIMARY KEY (`video_ID`,`username`),
  KEY `username` (`username`),
  CONSTRAINT `watched_ibfk_1` FOREIGN KEY (`video_ID`) REFERENCES `videos` (`video_ID`) ON DELETE CASCADE,
  CONSTRAINT `watched_ibfk_2` FOREIGN KEY (`username`) REFERENCES `users` (`username`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;


DROP PROCEDURE IF EXISTS `add_to_fav`;
DELIMITER //
CREATE PROCEDURE `add_to_fav` (IN ID VARCHAR(50), IN watcher VARCHAR(20))
BEGIN
  DECLARE count1 INT;

  SELECT COUNT(*) INTO count1
  FROM watched
  WHERE video_ID = ID AND username = watcher
  AND NOT EXISTS (
    SELECT 1
    FROM favourites
    WHERE username = watcher AND video_ID = ID
  );

  IF count1 >= 5 THEN
    INSERT INTO favourites (username, video_ID) VALUES (watcher, ID);
  END IF;

END;
//
DELIMITER ;
