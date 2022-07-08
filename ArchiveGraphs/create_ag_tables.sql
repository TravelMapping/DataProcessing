DROP TABLE IF EXISTS graphArchives;
DROP TABLE IF EXISTS graphArchiveSets;
CREATE TABLE graphArchiveSets (setName VARCHAR(16), descr VARCHAR(100), dateStamp DATE, hwyDataVers VARCHAR(41), userDataVers VARCHAR(41), dataProcVers VARCHAR(41), PRIMARY KEY(setName));
CREATE TABLE graphArchives (filename VARCHAR(32), descr VARCHAR(100), vertices INTEGER, edges INTEGER, travelers INTEGER, format VARCHAR(10), category VARCHAR(12), setName VARCHAR(16), FOREIGN KEY (category) REFERENCES graphTypes(category), FOREIGN KEY (setName) REFERENCES graphArchiveSets(setName));
