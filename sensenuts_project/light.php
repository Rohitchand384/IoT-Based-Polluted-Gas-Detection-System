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
?>


<html>
<meta http-equiv="refresh" content="5">

<head>

    <style>
        h3 {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue', sans-serif;
            font-size: 20px;
            font-weight: 500;
            color: white;
            text-align: center;

        }
    </style>




    <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
    <script type="text/javascript">
        google.charts.load('current', {
            'packages': ['corechart']
        });
        google.charts.setOnLoadCallback(drawChart);

        function drawChart() {
            var data = google.visualization.arrayToDataTable([
                ['date', 'gas2'],
                <?php
                $query = "(SELECT * FROM `gas` ORDER BY id desc limit 50)order by id ";
                $res = mysqli_query($conn, $query);
                while ($data = mysqli_fetch_array($res)) {
                    $date = $data['date'];
                    $gas2 = $data['gas2'];
                    $nodeid = $data['Node_id'];
                    if ($nodeid == 7793) {
                ?>['<?php echo $date; ?>', <?php echo $gas2; ?>],
                <?php
                    }
                }
                ?>
            ]);

            var options = {
                vAxis: {
                    textStyle: {
                        color: '#ffffff',
                        fontSize: 14,
                        bold: true
                    },
                },
                hAxis: {
                    textStyle: {
                        color: '#ffffff',
                        fontSize: 14,
                        bold: true
                    },
                },
                legend: 'none',
                lineWidth: 2,
                series: {
                    0: {
                        color: 'yellow'
                    },
                },
                backgroundColor: "transparent"

            };


            var chart = new google.visualization.LineChart(document.getElementById('curve_chart'));

            chart.draw(data, options);
        }
    </script>
</head>

<body>
    <h3>GAS 2 (7793)</h3>
    <div id="curve_chart"></div>

</body>

</html>