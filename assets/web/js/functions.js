var querystring = {};
location.search.substr(1).split("&").forEach(function(item) {
	var tokens = item.split("="),
	key = tokens[0],
	value = tokens[1] && decodeURIComponent(tokens[1]);
	if(key in querystring)
		querystring[key].push(value);
	else
		querystring[key] = [value];
})
