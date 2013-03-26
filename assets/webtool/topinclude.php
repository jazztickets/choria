<?
	include("connection.php");
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
	<head>
		<link rel="stylesheet" type="text/css" href="style.css" />
		<title>Choria Design Tool</title>
	</head>
	<body>
		<div>
			<a class="navbar" href="monsters.php">Monsters Drops</a> |
			<a class="navbar" href="zones.php">Zones</a> |
			<a class="navbar" href="vendors.php">Vendors</a> |
			<a class="navbar" href="traders.php">Traders</a>
		</div>
		<?
		$query = $Database->query("select name from sqlite_master where type = 'table'");
		if(!$query) {
			print_r($Database->errorInfo());
		}
		$results = $query->fetchAll(PDO::FETCH_ASSOC);
		$array_keys = array_keys($results[0]);
		$key_count = count($array_keys);
		?>
		<div style="font-size: 85%; margin-top: 5px">
			<? foreach($results as $result) { ?>
			<a class="navbar" href="data.php?table=<?=$result['name']?>"><?=$result['name']?></a> |
			<? } ?>
		</div>
		<div class="main">
