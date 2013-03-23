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
	$Query = $Database->query("select T.ID, T.Name, I.Name, T.Count from Traders T inner join Items I on T.ItemsID = I.ID");
	$Result = $Query->fetchAll();

	$Count = count($Result);
	for($i = 0; $i < $Count; $i++) {
		$TradersID = $Result[$i][0];
?>
	<tr>
		<td><?=$TradersID?></td>
		<td><?=$Result[$i][1]?></td>
		<td><?=$Result[$i][2]?></td>
		<td><?=$Result[$i][3]?></td>
	<?
		$TraderCountQuery = $Database->query("select count(TI.ID) from TraderItems TI where TI.TradersID = $TradersID");
		$TraderCount = $TraderCountQuery->fetch();
	?>
		<td><a href="traderitems.php?id=<?=$Result[$i][0]?>">Edit Items (<?=$TraderCount[0]?>)</a></td>
	</tr>
<?
	}
?>
</table>

<?
	include("footer.php");
?>