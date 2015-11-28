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
	$Query = $Database->query("select ID, Name from vendor");
	$Result = $Query->fetchAll();

	$Count = count($Result);
	for($i = 0; $i < $Count; $i++) {
		$VendorID = $Result[$i][0];
?>
	<tr>
		<td><?=$VendorID?></td>
		<td><?=$Result[$i][1]?></td>
	<?
		$DropsCountQuery = $Database->query("select count(VI.ID) from vendoritem VI where VI.vendor_id = $VendorID");
		$DropCount = $DropsCountQuery->fetch();
	?>
		<td><a href="vendoritems.php?id=<?=$Result[$i][0]?>">Edit item (<?=$DropCount[0]?>)</a></td>
	</tr>
<?
	}
?>
</table>

<?
	include("footer.php");
?>