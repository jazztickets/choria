<?
	include("topinclude.php");
?>

<table border="0">
	<tr class="header">
		<td>ID</td>
		<td>Name</td>
		<td>Reward Item</td>
		<td>Reward Count</td>
		<td></td>
	</tr>
<?
	$Query = $Database->query("select T.ID, T.Name, I.Name, T.Count from trader T inner join item I on T.item_id = I.ID");
	$Result = $Query->fetchAll();

	$Count = count($Result);
	for($i = 0; $i < $Count; $i++) {
		$trader_id = $Result[$i][0];
?>
	<tr>
		<td><?=$trader_id?></td>
		<td><?=$Result[$i][1]?></td>
		<td><?=$Result[$i][2]?></td>
		<td><?=$Result[$i][3]?></td>
	<?
		$TraderCountQuery = $Database->query("select count(TI.ID) from traderitem TI where TI.trader_id = $trader_id");
		$TraderCount = $TraderCountQuery->fetch();
	?>
		<td><a href="traderitems.php?id=<?=$Result[$i][0]?>">Edit item (<?=$TraderCount[0]?>)</a></td>
	</tr>
<?
	}
?>
</table>

<?
	include("footer.php");
?>