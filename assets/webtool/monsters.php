<?
	include("topinclude.php");
	@$changed = intval($_GET['changed']);
?>

<? if($changed) echo "<div class=\"changed\">Changed</div>"; ?>
<table border="0">
	<tr class="header">
		<td>ID</td>
		<td>Name</td>
		<td></td>
	</tr>
<?
	$Query = $Database->query("select ID, Name from monster");
	$Result = $Query->fetchAll();

	$Count = count($Result);
	for($i = 0; $i < $Count; $i++) {
		$MonsterID = $Result[$i][0];
?>
	<tr>
		<td><a href="data_ae.php?id=<?=$Result[$i][0]?>&table=monsters&mode=edit"><?=$MonsterID?></a></td>
		<td><?=$Result[$i][1]?></td>
	<?
		$DropsCountQuery = $Database->query("select count(MD.ID) from monsterdrop MD left join item I on MD.item_id = I.ID where MD.monster_id = $MonsterID");
		$DropCount = $DropsCountQuery->fetch();
	?>
		<td><a href="monsterdrops.php?id=<?=$Result[$i][0]?>">Edit Drops (<?=$DropCount[0]?>)</a></td>
	</tr>
<?
	}
?>
</table>

<?
	include("footer.php");
?>
