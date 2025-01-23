const char loginIndex[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <title>Update unsuccessful</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body {
        background-color: #f7f7f7;
      }
      #spacer_50 {
        height: 50px;
      }
    </style>
  </head>
  <body>
    <form name='loginForm'>
        <table width='20%' bgcolor='A09F9F' align='center'>
            <tr>
                <td colspan=2>
                    <center><font size=4><b>eNable.it Platform Login Page</b></font></center>
                    <br>
                </td>
                <br>
                <br>
            </tr>
            <tr>
                <td>Username:</td>
                <td><input type='text' size=25 name='userid'><br></td>
            </tr>
            <br>
            <br>
            <tr>
                <td>Password:</td>
                <td><input type='Password' size=25 name='pwd'><br></td>
                <br>
                <br>
            </tr>
            <tr>
                <td><input type='submit' onclick='check(this.form)' value='Login'></td>
            </tr>
        </table>
    </form>
    <script>
        function check(form)
        {
            if(form.userid.value=='admin' && form.pwd.value=='admin')
            {
                window.open('/upload')
            }
            else
            {
                alert('Error Password or Username')/*displays error message*/
            }
        }
    </script>
</body>
</html> )rawliteral";;