<?
$DatabaseFile = "sqlite:../../working/database/data.s3db";
$Database = new PDO($DatabaseFile);
if(!$Database) {
	die("Cannot open database");
}


?>