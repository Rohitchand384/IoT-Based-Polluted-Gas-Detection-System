<?php
$servername = "localhost";
$username = "root";
$password = "";
$dbname = "gas"; // Replace with your actual database name

$conn = new mysqli($servername, $username, $password, $dbname);

if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

$data = array();

$sql = "SELECT * FROM gas ORDER BY id DESC LIMIT 20"; // Replace with your actual table name and column names
$result = mysqli_query($conn, $sql);

if ($result->num_rows > 0) {
    while($row = $result->fetch_assoc()) {
        $data[] = array("x" => $row["date"], "y" => $row["gas2"]);
    }
}

$conn->close();

echo json_encode($data);
?>