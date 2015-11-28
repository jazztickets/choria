<?
	include("topinclude.php");
	$TraderID = intval($_GET["id"]);

	if(isset($_POST["Submit"])) {

		// Make sure there are items
		if(isset($_POST["MDItems"])) {

			// Delete old data
			$Database->query("delete from traderitem where trader_id = $TraderID");

			// Get new data
			$MDItems = $_POST["MDItems"];
			$MDOdds = $_POST["MDOdds"];
			$ItemCount = count($MDItems);

			// Add TraderItems
			for($i = 0; $i < $ItemCount; $i++) {
				$ItemID = $MDItems[$i];
				$Count = $MDOdds[$i];
				$Database->query("insert into traderitem(trader_id, item_id, Count) values($TraderID, $ItemID, $Count)");
			}
		}

		header("Location: traderitems.php?id=$TraderID&changed=true");
		exit;
	}

	$TraderQuery = $Database->query("select * from trader where ID = $TraderID");
	$TraderResult = $TraderQuery->fetch();

	$TraderItemsQuery = $Database->query("select TI.Count, I.ID, I.Name, I.Level from traderitem TI left join item I on TI.item_id = I.ID where TI.trader_id = $TraderID");
	$TraderItemsResult = $TraderItemsQuery->fetchAll();
	$TraderItemsCount = count($TraderItemsResult);

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
		Items for <?=$TraderResult[1]?>
	</div>
	<div>
	<form name="DataForm" onSubmit="AddItem(); return false;">
		<div>
			<div style="margin-bottom: 15px;">
				<select name="AllItems">
					<option value="0">None</option>
				<?
					for($i = 0; $i < $ItemsCount; $i++) {
						echo "<option value='{$ItemsResult[$i][0]}'>{$ItemsResult[$i][1]} ({$ItemsResult[$i][2]})</option>\n";
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
	<form name="Form" action="traderitems.php?id=<?=$TraderID?>" method="post" onSubmit="return SubmitForm();">

		<div style="float: left;">
			<div class="selectheader">
				Items
			</div>
			<div>
				<select name="MDItems[]" multiple="multiple" style="height: 15em; margin-right: 10px">
				<?
					for($i = 0; $i < $TraderItemsCount; $i++) {
						$ID = $TraderItemsResult[$i][1];
						$Name = $TraderItemsResult[$i][2];
						$Level = "({$TraderItemsResult[$i][3]})";
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
					for($i = 0; $i < $TraderItemsCount; $i++) {
						$Odds = $TraderItemsResult[$i][0];
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