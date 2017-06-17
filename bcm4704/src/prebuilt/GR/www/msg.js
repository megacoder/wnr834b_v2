//public message
var msg_blank = "%s darf nicht leer sein.\n";
var msg_space = "%s darf keine Leerzeichen enthalten.\n";
var msg_blank_in = "%s darf keine Leerzeichen enthalten.\n";
var msg_invalid = "\nEin oder mehrere ung­ítige Zeichen in %s\nG­ítige Zeichen sind:\n%s\n\n";
var msg_check_invalid = "%s enth?t eine ung­ítige Zahl.";
var msg_greater = "%s muss grcer sein als %s";
var msg_less = "%s muss kleiner sein als %s";
var msg_first = "1";
var msg_second = "2";
var msg_third = "3";
var msg_fourth = "4";
function addstr(input_msg)
{
	var last_msg = "";
	var str_location;
	var temp_str_1 = "";
	var temp_str_2 = "";
	var str_num = 0;
	temp_str_1 = addstr.arguments[0];
	while(1)
	{
		str_location = temp_str_1.indexOf("%s");
		if(str_location >= 0)
		{
			str_num++;
			temp_str_2 = temp_str_1.substring(0,str_location);
			last_msg += temp_str_2 + addstr.arguments[str_num];
			temp_str_1 = temp_str_1.substring(str_location+2,temp_str_1.length);
			continue;
		}
		if(str_location < 0)
		{
			last_msg += temp_str_1;
			break;
		}
	}
	return last_msg;
}
