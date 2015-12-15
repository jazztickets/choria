<?
$DatabaseFile = "sqlite:../../working/stats/stats.db";
$Database = new PDO($DatabaseFile);
if(!$Database) {
	die("Cannot open database");
}
$Database->query("PRAGMA foreign_keys=ON");
?>
