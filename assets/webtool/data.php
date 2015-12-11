<?
	include("topinclude.php");
	@$changed = intval($_GET['changed']);
	$table = $_GET['table'];
	@$sort = $_GET['sort'];
	@$reverse = intval($_GET['reverse']);
?>

<? if($changed) echo "<div class=\"changed\">Changed</div>"; ?>

<a href="data_ae.php?idname=<?=$array_keys[0]?>&table=<?=$table?>&mode=add">Add new <?=$table?> row</a>
<table border="0" style="margin-top: 10px">
	
<?
	$sort_sql = "";
	if($sort != "")
		$sort_sql = "order by $sort";
	
	$reverse_sql = "";
	if($reverse)
		$reverse_sql = "desc";
	$query = $Database->query("select * from $table $sort_sql $reverse_sql");
	if(!$query) {
		print_r($Database->errorInfo());
	}

	$results = $query->fetchAll(PDO::FETCH_ASSOC);
	if(count($results)) {
		$array_keys = array_keys($results[0]);
		$key_count = count($array_keys);
	}
?>
	<tr class="header">
		<? for($i = 0; $i < $key_count; $i++) {
			$field = $array_keys[$i];
			$reverse_string = "";
			if($sort == $field)
				$reverse_string = "&reverse=" . (1-$reverse);
		?>
		<td><a href="data.php?table=<?=$table?>&sort=<?=$field?><?=$reverse_string?>"><?=$field?></a></td>
		<? } ?>
		<td></td>
	</tr>
<?

	foreach($results as $result) {
		$id = $result[$array_keys[0]];
?>
	<tr>
		<td><?=$id?></td>
		<? for($i = 1; $i < $key_count; $i++) { ?>
		<td><?=$result[$array_keys[$i]]?></td>
		<? } ?>
		<td><a href="data_ae.php?id=<?=$id?>&idname=<?=$array_keys[0]?>&table=<?=$table?>&mode=edit">Edit</a></td>
		<td><a href="data_ae.php?id=<?=$id?>&idname=<?=$array_keys[0]?>&table=<?=$table?>&mode=remove" onclick="return confirm('Sure about that?');">Remove</a></td>
	</tr>
<?
	}
?>
</table>

<?
	include("footer.php");
?>
