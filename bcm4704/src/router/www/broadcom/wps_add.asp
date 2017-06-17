<!--
Copyright 2005, Broadcom Corporation
All Rights Reserved.

THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.

$Id: wps_add.asp,v 1.1.1.1 2010/03/05 07:31:37 reynolds Exp $
-->

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html lang="en">
<head>
<title>Broadcom Home Gateway Reference Design: WPS Pin</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<link rel="stylesheet" type="text/css" href="style.css" media="screen">
<script language="JavaScript" type="text/javascript" src="overlib.js"></script>
<script language="JavaScript" type="text/javascript">

</script>
</head>

<div id="overDiv" style="position:absolute; visibility:hidden; z-index:1000;"></div>

<form method="post" action="wps_add.asp">
<input type="hidden" name="page" value="wps_add.asp">

<p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <th width="310"
	onMouseOver="return overlib('SSID.', LEFT);"
	onMouseOut="return nd();">
	Network Name(SSID):&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<% nvram_get("wl_ssid"); %>
    </td>
  </tr>
  <tr>
    <th width="310"
	onMouseOver="return overlib('Selects which config method you need.', LEFT);"
	onMouseOut="return nd();">
	Config Method:&nbsp;&nbsp;
    </th>
    <td>&nbsp;&nbsp;</td>
    <td>
	<select name="wsc_method" onChange="wl_wps_change();">
	  <option value="PIN" <% nvram_match("wsc_method", "1", "selected"); %>>PIN</option>
	  <option value="PBC" <% nvram_match("wsc_method", "2", "selected"); %>>PBC</option>
	</select>
    </td>
  </tr>
  <tr>
    <th width="310">PIN Number:&nbsp;&nbsp;</th>
    <td>&nbsp;&nbsp;</td>
    <td>
<!--		<input name="wsc_sta_pin" value="" size="8" maxlength="8">    !-->
		<% wsc_pin(); %>
	</td>
  </tr>
</table>

<p><p><p><p>
<table border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td width="310"></td>
    <td>&nbsp;&nbsp;</td>
    <td>
	<button type="submit" name="action" value="Start">Start</button>
    </td>
  </tr>
</table>

</form>

</body>
</html>
