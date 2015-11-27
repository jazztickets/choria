<?
$DatabaseFile = "sqlite:../../working/database/data.db";
$Database = new PDO($DatabaseFile);
if(!$Database) {
	die("Cannot open database");
}
?>
