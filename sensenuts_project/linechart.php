<?php
$servername = "localhost";
$dbname = "tempandlight";
$username = "root";
$password = "";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);
// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}
?>

<html>
<meta http-equiv="refresh" content="10">
<link rel="stylesheet" href="style.css">

<head>


    <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
    <script type="text/javascript">
        google.charts.load('current', { 'packages': ['corechart'] });
        google.charts.setOnLoadCallback(drawChart);

        function drawChart() {
            var data = google.visualization.arrayToDataTable([
                ['Time', 'Temp', 'Light'],
                <?php
                $query = "SELECT * FROM `sensordata` ";
                $res = mysqli_query($conn, $query);
                while ($data = mysqli_fetch_array($res)) {
                    $row_DATE = $row["DATE"];
                    $row_TEMP = $row["TEMP"];
                    $row_LIGHT = $row["LIGHT"];
                    ?>
                    ['<?php echo $row_DATE; ?>', '<?php echo $row_TEMP; ?>', '<?php echo $row_LIGHT; ?>']
                <?php } ?>

            ]);

            var options = {
                title: 'Temperature and Light',
                curveType: 'function',
                legend: { position: 'bottom' }
            };

            var chart = new google.visualization.LineChart(document.getElementById('curve_chart'));

            chart.draw(data, options);
        }
    </script>

</head>

<body>
    <div id="curve_chart" style="width: 900px; height: 500px"></div>
</body>

</html>