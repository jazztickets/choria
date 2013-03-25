<?
	include("topinclude.php");
	$VendorsID = intval($_GET["id"]);
	
	if(isset($_POST["Submit"])) {

		// Make sure there are items
		if(isset($_POST["MDItems"])) {
			
			// Delete old data
			if(!$Database->query("delete from VendorItems where VendorsID = $VendorsID")) {
				print_r($Database->errorInfo());
				exit();
			}
			
			// Get new data
			$MDItems = $_POST["MDItems"];
			$ItemCount = count($MDItems);
			
			// Add items
			for($i = 0; $i < $ItemCount; $i++) {
				$ItemID = $MDItems[$i];
				if(!$Database->query("insert into VendorItems(VendorsID, ItemsID) values($VendorsID, $ItemID)")) {
					print_r($Database->errorInfo());
					exit();
				}
			}
		}
		
		header("Location: vendoritems.php?id=$VendorsID&changed=true");
		exit;	
	}
	
	$VendorsQuery = $Database->query("select * from Vendors where ID = $VendorsID");
	$VendorsResult = $VendorsQuery->fetch();

	$VendorItemsQuery = $Database->query("select I.ID, I.Name, I.Level from VendorItems VI left join Items I on VI.ItemsID = I.ID where VI.VendorsID = $VendorsID");
	$VendorItemsResult = $VendorItemsQuery->fetchAll();
	$VendorItemsCount = count($VendorItemsResult);

	$ItemsQuery = $Database->query("select * from Items order by Name");
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
		Items for <?=$VendorsResult[1]?>
	</div>
	<div>
	<form name="DataForm" onSubmit="AddItem(); return false;">
		<div>
			<div style="margin-bottom: 15px;">
				<select name="AllItems">
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
	<form name="Form" action="vendoritems.php?id=<?=$VendorsID?>" method="post" onSubmit="return SubmitForm();">

		<div style="float: left;">
			<div class="selectheader">
				Items
			</div>
			<div>
				<select name="MDItems[]" multiple="multiple" style="height: 35em; margin-right: 10px">
				<?
					for($i = 0; $i < $VendorItemsCount; $i++) {
						$ID = $VendorItemsResult[$i][0];
						$Name = $VendorItemsResult[$i][1];
						$Level = "({$VendorItemsResult[$i][2]})";
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
				<select name="MDOdds[]" multiple="multiple" style="height: 35em;" onClick="SelectItem(this.selectedIndex);">
				<?
					for($i = 0; $i < $VendorItemsCount; $i++) {
						$Odds = 1;
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