<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<!-- TemplateBeginEditable name="doctitle" -->
<title>Client message</title>
<!-- TemplateEndEditable -->
<!-- TemplateBeginEditable name="head" -->
<!-- TemplateEndEditable -->
</head>

<body>
<form action="/update_message.php">
<table width="500" border="0">

  <tr>
    <td width=50>client_name</td>
    <td><input name="client_name" type="text" maxlength="20" /></td>
  </tr>
  <tr>
    <td>message</td>
    <td><input name="message" type="text" maxlength="50" /></td>
  </tr>
</table>

<input type='submit' value='submit'>
</form>
<script type="text/javascript">

var c_e=document.getElementById("client_name");
var m_e=document.getElementById("message");
        function add() {
			alert('Welcome!');
            $("#button1").attr("action", "update_message.php").submit();
            }
</script>
</body>
</html>