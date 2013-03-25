<?
	include("topinclude.php");
	$changed = intval($_GET['changed']);
	$table = $_GET['table'];
?>

<? if($changed) echo "<div class=\"changed\">Changed</div>"; ?>
<table border="0">
	<tr class="header">
		<td>ID</td>
		<td>Name</td>
		<td></td>
		<td></td>
	</tr>
<?
	$query = $Database->query("select * from $table");
	if(!$query) {
		print_r($Database->errorInfo());
	}
	$results = $query->fetchAll(PDO::FETCH_ASSOC);

	foreach($results as $result) {
		$id = $result['ID'];
		$name = $result['Name'];
?>
	<tr>
		<td><?=$id?></td>
		<td><?=$name?></td>
		<td><a href="data_ae.php?id=<?=$id?>&table=<?=$table?>&mode=edit">Edit</a></td>
	</tr>
<?
	}
?>
</table>

<?
	include("footer.php");
?>