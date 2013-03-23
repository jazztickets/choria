<?
	include("topinclude.php");
?>

<table border="0">
	<tr class="header">
		<td>ID</td>
		<td>Monster Count</td>
		<td></td>
	</tr>
<?
	$Query = $Database->query("select ID, MonsterCount from Zones");
	$Result = $Query->fetchAll();

	$Count = count($Result);
	for($i = 0; $i < $Count; $i++) {
		$ZoneID = $Result[$i][0];
?>
	<tr>
		<td><?=$ZoneID?></td>
		<td><?=$Result[$i][1]?></td>
	<?
		$ZoneCountQuery = $Database->query("select count(ZD.ID) from ZoneData ZD where ZD.ZonesID = $ZoneID");
		$ZoneCount = $ZoneCountQuery->fetch();
	?>
		<td><a href="zonedata.php?id=<?=$Result[$i][0]?>">Edit Monsters (<?=$ZoneCount[0]?>)</a></td>
	</tr>
<?
	}
?>
</table>

<?
	include("footer.php");
?>