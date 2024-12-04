<?php

$servername = "localhost";
$dbname = "gas";
$username = "root";
$password = "";

$key = $Node_id = $gas2 = "";
$key = "NL99WFDYPTYAB827";

$Node_id = test_input($_GET["field1"]);
$gas2 =test_input($_GET["field2"]); 

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);
// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

$sql = "INSERT INTO `gas` (Node_id,gas2,date) VALUES ('" . $Node_id . "','" . $gas2 . "',current_timestamp())";

if ($conn->query($sql) === TRUE) {
    echo "New record created successfully";
} else {
    echo "Error: " . $sql . "<br>" . $conn->error;
}

$conn->close();

?>

<?php


function test_input($data)
{
    $data = trim($data);
    $data = stripslashes($data);
    $data = htmlspecialchars($data);
    return $data;
}
?>