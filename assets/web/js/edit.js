var hot = null;
$(document).ready(function() {
	$(document).ajaxError(function() {
		  $("#message").text("check console");
	});

	try {
		tablename = querystring["table"][0];
	}
	catch(error) {
		tablename = "item";
	}

	// Load spreadsheet
	var container = document.getElementById('sheet');
	data_url = "/data?table=" + tablename;
	$.getJSON(data_url, function(response) {
		columns = response['columns']
		data = response['data']
		hot = new Handsontable(container, {
			data: data,
			rowHeaders: true,
			colHeaders: columns,
			columnSorting: true,
			contextMenu: {
				items: {
					"select_column": { name: 'Select Column' },
					"select_row": { name: 'Select Row' },
					"hsep0": "---------",
					"freeze_column": { name: 'Freeze Column' },
					"freeze_row": { name: 'Freeze Row' },
					"hsep1": "---------",
					"undo": {},
					"redo": {},
				},
				callback: function(key, options) {
					if(key === 'select_column') {
						var selected = hot.getSelected();
						hot.selectCell(0, selected[1], hot.countRows()-1, selected[1], false);
					}
					else if(key === 'select_row') {
						var selected = hot.getSelected();
						hot.selectCell(selected[0], 0, selected[0], hot.countCols()-1, false);
					}
					else if(key === 'freeze_column') {
						var selected = hot.getSelected();
						hot.updateSettings({ fixedColumnsLeft: selected[1] });
					}
					else if(key === 'freeze_row') {
						var selected = hot.getSelected();
						hot.updateSettings({ fixedRowsTop: selected[0] });
					}
				},
			},
			cells: function(row, col, prop) {
				var cellProperties = {};
				if(col == 0) {
					cellProperties.readOnly = true;
				}

				return cellProperties;
			},
			afterChange: function(changes, source) {
				update_buttons(0);
			},
			afterSelection: function(r, c, r2, c2) {
				$('#delete_id').val(hot.getDataAtCell(r, 0));
			},
		});

		update_buttons(1);
	});

	// load nav links
	$.getJSON("/tables", function(response) {
		tables = response;
		for(i in tables) {
			table = tables[i];
			li_element = document.createElement("li");
			a_element = document.createElement("a");
			$(a_element).attr("href", "/?table=" + table);
			$(a_element).html(table);
			$(li_element).append(a_element);
			$('#tables').append(li_element);
		}
	});

	// Add handlers
	$('#save').click(function() { save(); });
	$('#add').click(function() { add(); });
	$('#delete').click(function() { remove(); });
});

// Change add row/delete button state
function update_buttons(state) {
	$('#add').prop('disabled', !state);
	$('#delete').prop('disabled', !state);
	$('#delete_id').prop('disabled', !state);
}

// Reload data
function reload() {
	$.getJSON(data_url, function(response) {
		hot.loadData(response['data']);
		update_buttons(1);
	});
}

// Save data
function save() {

	// Send request
	var request = $.ajax({
		type: "POST",
		url: "/save?table=" + tablename,
		data: jQuery.param({'data': hot.getData()}),
		dataType: "html",
		success: function(response, text_status, jqxhr) {
			ajax_success(response, text_status, jqxhr);
			update_buttons(1);
		},
		error: ajax_error,
	});
}

// Add new row
function add() {

	// Send request
	var request = $.ajax({
		type: "POST",
		url: "/add?table=" + tablename,
		data: {},
		dataType: "html",
		success: function(response, text_status, jqxhr) {
			ajax_success(response, text_status, jqxhr);
			reload();
		},
		error: ajax_error,
	});
}

// Delete row
function remove() {
	id = parseInt($('#delete_id').val());
	if(id <= 0 || isNaN(id) || !confirm("Sure ?"))
		return;

	// Send request
	var request = $.ajax({
		type: "POST",
		url: "/remove?table=" + tablename,
		data: {'id': id},
		dataType: "html",
		success: function(response, text_status, jqxhr) {
			ajax_success(response, text_status, jqxhr);
			reload();
		},
		error: ajax_error,
	});
}

function ajax_success(response, text_status, jqxhr) {
	result = JSON.parse(response);
	if(text_status == "success") {
		message = result['message'];
		if(result['message'] != undefined)
			$('#message').html(message);
	}
	else
		$('#message').html(text_status);
}

function ajax_error(jqxhr, text_status, error) {
	$('#message').html(error);
}

// Ctrl+s hotkey
$(document).keydown(function(event) {
	if(!(String.fromCharCode(event.which).toLowerCase() == 's' && event.ctrlKey) && !(event.which == 19))
		return true;

	event.preventDefault();
	save();

	return false;
});
