<!DOCTYPE html>
<html>
<meta http-equiv="refresh" content="2">
<link rel="stylesheet" href="style.css">
<body>

<?php

  
$servername = "localhost";
$dbname = "gas";
$username = "root";
$password = "";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);
// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
} 

// $sql = "SELECT * FROM sensordata ORDER BY id DESC";
$sql = "SELECT *FROM gas ORDER BY id DESC";



echo "<h1>SENSEnuts Temperature and Light Data</h1>";

echo '<table cellspacing="5" cellpadding="5" style="width:100%">
      <tr> 
        <td style="background-color: #e10dac";>ID</td> 
        <td style="background-color: #e10dac";>Node_id</td> 
        <td style="background-color: #e10dac";>GAS 2</td> 
        <td style="background-color: #e10dac";>DATE</td> 
       
      </tr>';
     
      
 
if ($result = $conn->query($sql)) {
  
    while ($row = $result->fetch_assoc()) {
        $row_id = $row["id"];
        $row_Node_id = $row["Node_id"];
        $row_gas2 = $row["gas2"];        
        $row_date = $row["date"];
       
 echo '<tr id="tabledata"> 
                <td>' . $row_id . '</td> 
                <td>' . $row_Node_id . '</td> 
                <td>' . $row_gas2 . '</td>              
                <td>' . $row_date . '</td>
              </tr>';
    }
    $result->free();
    $conn->close();
}
?>
</table>
</body>
</html>