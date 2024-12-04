<!DOCTYPE html>
<html lang="en" dir="ltr">
<link rel="stylesheet" href="style.css">
  <head>
    <meta charset="utf-8">
    <title>Real Time Data Display</title>
  </head>
  <body onload = "table();">
    <script type="text/javascript">
      function table(){
        const xhttp = new XMLHttpRequest();
        xhttp.onload = function(){
          document.getElementById("table").innerHTML = this.responseText;
        }
        xhttp.open("GET", "auto_sync_system.php");
        xhttp.send();
      }

      setInterval(function(){
        table();
      }, 1000);
    </script>
    <div id="table">

    </div>
  </body>
</html>
