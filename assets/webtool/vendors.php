<?
	include("topinclude.php");
?>

<table border="0">
	<tr class="header">
		<td>ID</td>
		<td>Name</td>
		<td></td>
	</tr>
<?
	$Query = $Database->query("select ID, Name from Vendors");
	$Result = $Query->fetchAll();

	$Count = count($Result);
	for($i = 0; $i < $Count; $i++) {
		$VendorID = $Result[$i][0];
?>
	<tr>
		<td><?=$VendorID?></td>
		<td><?=$Result[$i][1]?></td>
	<?
		$DropsCountQuery = $Database->query("select count(VI.ID) from VendorItems VI where VI.VendorsID = $VendorID");
		$DropCount = $DropsCountQuery->fetch();
	?>
		<td><a href="vendoritems.php?id=<?=$Result[$i][0]?>">Edit Items (<?=$DropCount[0]?>)</a></td>
	</tr>
<?
	}
?>
</table>

<?
	include("footer.php");
?>