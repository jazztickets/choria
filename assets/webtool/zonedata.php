<?
	include("topinclude.php");
	$ZoneID = intval($_GET["id"]);
	
	if(isset($_POST["Submit"])) {

		// Make sure there are monsters
		if(isset($_POST["MDMonsters"])) {
			
			// Delete old data
			$Database->query("delete from ZoneData where ZonesID = $ZoneID");
			
			// Get new data
			$MDMonsters = $_POST["MDMonsters"];
			$MDOdds = $_POST["MDOdds"];
			$ItemCount = count($MDMonsters);
			
			// Add MonsterSpawn
			for($i = 0; $i < $ItemCount; $i++) {
				$MonsterID = $MDMonsters[$i];
				$Odds = $MDOdds[$i];
				$Database->query("insert into ZoneData(ZonesID, MonstersID, Odds) values($ZoneID, $MonsterID, $Odds)");
			}
		}
		
		header("Location: zonedata.php?id=$ZoneID&changed=true");
		exit;	
	}
	
	$ZonesQuery = $Database->query("select * from Zones where ID = $ZoneID");
	$ZonesResult = $ZonesQuery->fetch();

	$MonsterSpawnQuery = $Database->query("select ZD.Odds, M.ID, M.Name, M.Level from ZoneData ZD left join Monsters M on ZD.MonstersID = M.ID where ZD.ZonesID = $ZoneID order by Odds desc");
	$MonsterSpawnResult = $MonsterSpawnQuery->fetchAll();
	$MonsterSpawnCount = count($MonsterSpawnResult);

	$MonstersQuery = $Database->query("select * from Monsters order by Level");
	$MonstersResult = $MonstersQuery->fetchAll();
	$MonstersCount = count($MonstersResult);

	if(isset($_GET["changed"])) {
?>
	<div class="changed">
	Changed
	</div>
<?
	}
?>
	<div style="margin-bottom: 10px; font-weight: bold;">
		Monsters for Zone <?=$ZoneID?>
	</div>
	<div>
	<form name="DataForm" onSubmit="AddItem(); return false;">
		<div>
			<div style="margin-bottom: 15px;">
				<select name="AllMonsters">
					<option value="0">None</option>
				<?
					for($i = 0; $i < $MonstersCount; $i++) {
						echo "<option value='{$MonstersResult[$i][0]}'>{$MonstersResult[$i][2]} ({$MonstersResult[$i][1]})</option>\n";
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
	<form name="Form" action="zonedata.php?id=<?=$ZoneID?>" method="post" onSubmit="return SubmitForm();">

		<div style="float: left;">
			<div class="selectheader">
				Items
			</div>
			<div>
				<select name="MDMonsters[]" multiple="multiple" style="height: 15em; margin-right: 10px">
				<?
					for($i = 0; $i < $MonsterSpawnCount; $i++) {
						$ID = $MonsterSpawnResult[$i][1];
						$Name = $MonsterSpawnResult[$i][2];
						$Level = "({$MonsterSpawnResult[$i][3]})";
						if($Name == '') {
							$ID = 0;
							$Name = "None";
							$Level = "";
						}
						echo "<option value='$ID'>$Name $Level</option>\n";
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
					for($i = 0; $i < $MonsterSpawnCount; $i++) {
						$Odds = $MonsterSpawnResult[$i][0];
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
	AllItemsElement = DataFormElement.AllMonsters;
	OddsBox = DataFormElement.Odds;
	MDItemsElement = FormElement.elements["MDMonsters[]"];
	MDOddsElement = FormElement.elements["MDOdds[]"];
	OddsEdit = FormElement.OddsEdit;

	CalculatePercents();
</script>
<?
	include("footer.php");
?>