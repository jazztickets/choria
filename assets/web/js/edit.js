var use_sum = false;
var hot = null;
var last_search = "";
var container = null;
var data = null;
var column_names = null;

$(document).ready(function() {
	$(document).ajaxError(function() {
		  $("#message").text("check console");
	});

	querystring = location.search;
	if(querystring == "") {
		querystring = "?table=item";
	}

	// Load spreadsheet
	container = document.getElementById('sheet');
	data_url = "/data" + querystring;
	$.getJSON(data_url, function(response) {
		data = response['data'];
		message = response['message'];
		column_names = response['column_names'];
		references = response['references'];
		children = response['children'];

		// Show messages
		if(message != undefined) {
			$('#message').html(message);
			return;
		}

		// Build dropdown data
		var columns = [];
		transform_data(data, columns);
		calculate_all_sum(data);

		// Create spreadsheet
		hot = new Handsontable(container, {
			data: data,
			rowHeaders: true,
			colHeaders: column_names,
			columnSorting: true,
			columns: columns,
			trimDropdown: false,
			manualColumnResize: true,
			manualColumnFreeze: true,
			wordWrap: false,
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
				else if(column_names[col] == "") {
					cellProperties.editor = false;
					cellProperties.disableVisualSelection = true;
				}

				return cellProperties;
			},
			afterChange: function(changes, source) {
				update_buttons(0);
				if(use_sum && source != "external" && source != "loadData" && changes) {

					// Get unique columns to update
					column_updates = [];
					for(change in changes)
						column_updates[changes[change][1]] = 1;

					// Get data
					sum_data = hot ? hot.getData() : data;

					// Update columns
					for(col in column_updates)
						calculate_sum(sum_data, changes[change][1], true);
				}
			},
			afterSelection: function(r, c, r2, c2) {
				if(r != data.length-1)
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
	$('#search_field').keyup({ force: 0 }, search);
});

// Function to render related table links
function link_renderer(instance, td, row, col, prop, value, cellProperties) {
	td.innerHTML = value;
	td.className = 'empty';

	return td;
}

// Calculate sum for each column
function calculate_all_sum(data) {
	if(!use_sum)
		return;

	for(col in column_names)
		calculate_sum(data, col);
}

// Calculate sum for each column
function calculate_sum(data, col, load=false) {
	if(!use_sum)
		return;

	if(col == 0 || column_names[col] == "" || references.hasOwnProperty(column_names[col]))
		return;

	column_total = 0;
	for(var row in data) {
		if(row == data.length-1)
			continue;

		if(!isNumeric(data[row][col])) {
			column_total = 0;
			break;
		}

		column_total += Number(data[row][col]);
	}

	data[data.length-1][col] = column_total;
	if(hot) {
		if(load)
			hot.loadData(data);
	}
}

// Function to determine if value is a number
function isNumeric(value) {
	return !isNaN(parseFloat(value)) && isFinite(value);
}

// Change add row/delete button state
function update_buttons(state) {
	$('#add').prop('disabled', !state);
	$('#delete').prop('disabled', !state);
	$('#delete_id').prop('disabled', !state);
}

// Transform data set by modifying foreign key values to "Name (ID)"
function transform_data(data, columns) {

	// build autocomplete data and change data array
	for(var i in column_names) {
		var column = column_names[i];

		// Check for foreign key reference
		if(references.hasOwnProperty(column)) {
			var reference = references[column];
			var names = []

			// Build drop down
			for(var key in reference) {
				if(reference.hasOwnProperty(key)) {
					var value = reference[key] + " (" + key + ")";
					names.push(value);
				}
			}

			// Add to columns list
			columns.push({
				type: 'dropdown',
				source: names,
			});

			// Change data
			for(var data_row in data) {
				var id = data[data_row][i];

				// Skip last row
				if(data_row != data.length-1)
					data[data_row][i] = reference[id] + " (" + id + ")";
			}
		}
		else
			columns.push({});
	}

	// Add children links
	if(Object.keys(children).length > 0) {
		column_names.push('');
		columns.push({ renderer: link_renderer });
		for(var row in data) {
			var id = data[row][0];
			var link = '';
			for(var child in children) {
				var field_name = children[child][0];
				link += '<a href="/?table=' + child + '&' + field_name + '=' + id + '">' + child + '</a>&nbsp;';
			}

			data[row].push(link);
		}
	}
}

// Reload data
function reload() {
	$.getJSON(data_url, function(response) {
		data = response['data'];
		var columns = [];
		transform_data(data, columns)
		calculate_all_sum(data);

		hot.loadData(data);
		update_buttons(1);
	});
}

function escape_regex(string){
	return string.replace(/([.*+?^=!:${}()|\[\]\/\\])/g, "\\$1");
}

function get_search_parameters(search) {

	var option_string = "g";
	//if(!option_case.prop('checked'))
	option_string += "i";

	// Convert search query to regex
	var regex;
	/*if(option_regex.prop('checked')) {
		try {
			regex = new RegExp(search, option_string);
		}
		catch(error) {
		}
	} else */{
		regex = new RegExp(escape_regex(search), option_string);
	}

	return [option_string, regex];
}

function search(event) {
	search_field = $('#search_field');
	var search = ('' + search_field.val());
	if(search == "") {
		calculate_all_sum(data);
		return hot.loadData(data);
	}

	// Only search when query has changed
	if(last_search == search && event.data.force == 0)
		return;

	var search_parameters = get_search_parameters(search);
	var option_string = search_parameters[0];
	var regex = search_parameters[1];

	var search_data = data;
	var filtered = []
	for(var row = 0, r_len = search_data.length; row < r_len; row++) {
		var row_found = false;
		for(var col = 0, c_len = search_data[row].length; col < c_len; col++) {

			var cell_meta = hot.getCellMeta(row, col);
			if(cell_meta.renderer != link_renderer) {

				var cell_value = ('' + search_data[row][col]);
				if(regex)
					regex.lastIndex = 0;

				if(search != "" && regex && regex.test(cell_value)) {
					row_found = true;
				}
				//else {
					//delete cell_meta.renderer;
				//}
			}
		}

		if(row_found)
			filtered.push(search_data[row])
	}
	filtered.push(search_data[data.length-1])

	last_search = search;

	calculate_all_sum(filtered);
	hot.loadData(filtered);
	hot.render();
}

// Save data
function save() {

	// Convert dropdowns back to ids
	var headers = hot.getColHeader();
	var data = hot.getData();
	remove_row = -1;
	for(var row in data) {
		if(data[row][0] == "sum") {
			remove_row = row;
			continue;
		}
		for(var col in headers) {

			// Remove empty columns from data array
			if(column_names[col] == "") {
				data[row].splice(col, 1);
				continue;
			}

			// Set null values to empty string
			if(data[row][col] == null)
				data[row][col] = "";

			// Parse id from dropdown data
			if(references.hasOwnProperty(headers[col]) && data[row][col]) {
				match = data[row][col].match(/\((.*?)\)$/);
				if(match)
					data[row][col] = parseInt(match[1]);
			}
		}
	}

	// Remove sum row
	if(remove_row != -1)
		data.splice(remove_row, 1);

	// Send request
	var request = $.ajax({
		type: "POST",
		url: "/save" + querystring,
		data: jQuery.param({'data': data}),
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
		url: "/add" + querystring,
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
		url: "/remove" + querystring,
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

// Global hotkeys
$(document).keydown(function(event) {

	key = String.fromCharCode(event.which).toLowerCase();
	if(event.ctrlKey) {
		if(key == 's') {
			event.preventDefault();
			save();
			return false;
		}
	}

	return true;
});
