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


$rows = mysqli_query($conn, "SELECT * FROM gas ORDER BY id DESC limit 1");
?>
<table class="table1" border=1 cellspacing="5" cellpadding="5" style="width:80%">
  <tr>
    <!-- <td style="background-color: #e10dac">#</td> -->
    <td class="sys1" style="background-color: #e10dac" ;>GAS 1</td>
    <td class="sys1" style="background-color: #e10dac" ;>GAS 2</td>

  </tr>
  <?php $i = 1; ?>
  <?php foreach ($rows as $row) : ?>
    <tr>
      <!-- <td><?php echo $i++; ?></td>       -->
      <td class="sys"><?php echo $row["gas1"]; ?></td>
      <td class="sys"><?php echo $row["gas2"]; ?></td>

    </tr>
  <?php endforeach; ?>
</table>