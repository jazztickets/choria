<?
	include("topinclude.php");
	$MonsterID = intval($_GET["id"]);

	if(isset($_POST["Submit"])) {

		// Make sure there are items
		if(isset($_POST["MDItems"])) {

			// Delete old data
			$Database->query("delete from monsterdrop where monster_id = $MonsterID");

			// Get new data
			$MDItems = $_POST["MDItems"];
			$MDOdds = $_POST["MDOdds"];
			$ItemCount = count($MDItems);

			// Add drops
			for($i = 0; $i < $ItemCount; $i++) {
				$ItemID = $MDItems[$i];
				$Odds = $MDOdds[$i];
				$Database->query("insert into monsterdrop(monster_id, item_id, odds) values($MonsterID, $ItemID, $Odds)");
			}
		}

		header("Location: monsterdrops.php?id=$MonsterID&changed=true");
		exit;
	}

	$MonsterQuery = $Database->query("select * from monster where ID = $MonsterID");
	$MonsterResult = $MonsterQuery->fetch();

	$DropsQuery = $Database->query("select MD.Odds, I.ID, I.Name, I.Level from monsterdrop MD left join item I on MD.item_id = I.ID where MD.monster_id = $MonsterID order by odds desc");
	$DropsResult = $DropsQuery->fetchAll();
	$DropsCount = count($DropsResult);

	$ItemsQuery = $Database->query("select * from item order by Name");
	$ItemsResult = $ItemsQuery->fetchAll();
	$ItemsCount = count($ItemsResult);

	if(isset($_GET["changed"])) {
?>
	<div class="changed">
	Changed
	</div>
<?
	}
?>
	<div style="margin-bottom: 10px; font-weight: bold;">
		Drops for <?=$MonsterResult[2]?> (<?=$MonsterResult[1]?>)
	</div>
	<div>
	<form name="DataForm" onSubmit="AddItem(); return false;">
		<div>
			<div style="margin-bottom: 15px;">
				<select name="AllItems">
					<option value="0">None</option>
				<?
					for($i = 0; $i < $ItemsCount; $i++) {
						echo "<option value='{$ItemsResult[$i][0]}'>{$ItemsResult[$i][1]}</option>\n";
					}
				?>
				</select>
				<input name="Odds" type="text" value="0" style="width: 8ex;">
			</div>
			<div style="margin-bottom: 20px">
				<input type="button" onClick="AddItem();" value="add">
			</div>
		</div>
	</form>
	<form name="Form" action="monsterdrops.php?id=<?=$MonsterID?>" method="post" onSubmit="return SubmitForm();">

		<div style="float: left;">
			<div class="selectheader">
				Items
			</div>
			<div>
				<select name="MDItems[]" multiple="multiple" style="height: 15em; margin-right: 10px">
				<?
					for($i = 0; $i < $DropsCount; $i++) {
						$ID = $DropsResult[$i][1];
						$Name = $DropsResult[$i][2];
						$Level = "({$DropsResult[$i][3]})";
						if($Name == '') {
							$ID = 0;
							$Name = "None";
							$Level = "";
						}
						echo "<option value='$ID'>$Name</option>\n";
					}
				?>
				</select>
			</div>
		</div>
		<div style="float: left;">
			<div class="selectheader">
				Odds
			</div>
			<div>
				<select name="MDOdds[]" multiple="multiple" style="height: 15em;" onClick="SelectItem(this.selectedIndex);">
				<?
					for($i = 0; $i < $DropsCount; $i++) {
						$Odds = $DropsResult[$i][0];
						echo "<option value='$Odds'>$Odds</option>\n";
					}
				?>
				</select>
			</div>
		</div>
	</div>
	<div style="clear: both;"></div>
	<div style="margin-top: 10px">
		<input name="OddsEdit" type="text" value="0" style="width: 8ex;">
		<input type="button" onClick="EditItem();" value="edit">
		<input type="button" onClick="RemoveItem();" value="remove">
	</div>
	<div style="margin-top: 20px">
		<input type="submit" name="Submit" value="Submit">
	</div>
</form>
<script type="text/javascript" src="functions.js"></script>
<script type="text/javascript">
	DataFormElement = document.forms["DataForm"];
	FormElement = document.forms["Form"];
	AllItemsElement = DataFormElement.AllItems;
	OddsBox = DataFormElement.Odds;
	MDItemsElement = FormElement.elements["MDItems[]"];
	MDOddsElement = FormElement.elements["MDOdds[]"];
	OddsEdit = FormElement.OddsEdit;

	CalculatePercents();
</script>
<?
	include("footer.php");
?>
