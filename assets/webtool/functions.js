function AddItem() {
	ItemIndex = AllItemsElement.selectedIndex;
	Odds = OddsBox.value;

	NewMDItem = new Option(AllItemsElement.options[ItemIndex].text, AllItemsElement.options[ItemIndex].value);
	NewMDOdd = new Option(Odds, Odds);

	MDItemsElement.add(NewMDItem, null);
	MDOddsElement.add(NewMDOdd, null);
	CalculatePercents();
}

function RemoveItem() {
	RemoveIndex = MDItemsElement.selectedIndex;
	MDItemsElement.remove(RemoveIndex);
	MDOddsElement.remove(RemoveIndex);
	CalculatePercents();
}

function SelectItem(TIndex) {
	OddsEdit.value = MDOddsElement.options[TIndex].value;
	OddsEdit.select();
}

function EditItem() {

	SelectedIndex = MDOddsElement.selectedIndex;
	if(SelectedIndex < 0)
		return;

	MDOddsElement.options[SelectedIndex].text = OddsEdit.value;
	MDOddsElement.options[SelectedIndex].value = OddsEdit.value;
	
	CalculatePercents();
}

function SubmitForm() {
	for(i = 0; i < MDItemsElement.options.length; i++) {
		MDItemsElement.options[i].selected = true;
	}
	for(i = 0; i < MDOddsElement.options.length; i++) {
		MDOddsElement.options[i].selected = true;
	}

	return true;
}

function CalculatePercents() {

	Sum = 0;
	for(i = 0; i < MDOddsElement.options.length; i++) {
		Sum += parseInt(MDOddsElement.options[i].value);
	}

	for(i = 0; i < MDOddsElement.options.length; i++) {
		Value = MDOddsElement.options[i].value;
		Percent = parseFloat(Value) / Sum * 100;
		MDOddsElement.options[i].text = Value + ' - ' + Percent.toFixed(1) + '%'; 
	}

	//alert(Sum);
}