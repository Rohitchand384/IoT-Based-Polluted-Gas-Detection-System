<html>
<meta http-equiv="refresh" content="5">
<Title>
  Dashboard
</Title>

<head>
  <meta charset="utf-8">


  <script type="text/javascript">
    function playSound() {
      var audio = new Audio("siren.mp3");
      audio.play();
    }
  </script>


  <style>
    body {
      background-image: url('orange.jpg');
      background-size: 100% 100%;
    }

    .parent {
      position: absolute;
      width: 90%;
      height: 80%;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
      padding: 30px;
      box-shadow: 0.5rem 0.5rem 1rem 0rem rgba(0, 0, 0, 0.45);
      border-radius: 35px;
      backdrop-filter: blur(10px);
      background-color: rgba(23, 23, 23, 0.238);
      border-left-color: rgb(228, 227, 227);
      border-left-style: solid;
      border-left-width: 1.5px;
      border-top-color: rgb(228, 227, 227);
      border-top-style: solid;
      border-top-width: 1.5px;
    }

    .title {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue', sans-serif;
      font-size: 40px;
      font-weight: 500;
      color: white;
      text-align: center;
      margin: 5px;

    }

    .center {
      text-align: center;
    }

    table {
      font-family: arial;
      font-size: 18px;
      color: white;
      border-collapse: collapse;
      width: 90%;
      height: 20px;
      position: center;
      margin-left: 70px;
      margin-bottom: 10px;
      text-align: center;
      backdrop-filter: blur(5px);
      border: 2px;
      border-color: black;
      border-style: solid;
    }

    tr:nth-child(odd) {
      background-color: #0000008f;
      border: 2px;
      border-color: black;
    }

    iframe {
      height: 280px;
      width: 600px;
      margin: 10px;
      box-shadow: 0.5rem 0.5rem 1rem 0rem rgba(0, 0, 0, 0.45);
      border-radius: 17px;
      backdrop-filter: blur(10px);
      background-color: rgba(23, 23, 23, 0.238);
      border-left-color: rgb(228, 227, 227);
      border: none;
      padding: 20px;

    }
  </style>



</head>




<body>
  <div class="parent">
    <p class="title">SENSEnuts GAS Data</p>
    <div class="center">
      <table cellpadding="5">

        <tr>
          <th>Node id</td>
          <th>GAS</td>
        </tr>


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

        $sql = "SELECT * FROM gas ORDER BY id DESC limit 1";

        if ($result = $conn->query($sql)) {
          while ($row = $result->fetch_assoc()) {
            $nid = $row["Node_id"];
            $gas = $row["gas2"];

            echo '<tr>                 
                <td >' . $nid . '</td> 
                <td >' . $gas . '</td>                 
              </tr>';

            if ($gas > 900) {
              echo '<script type="text/javascript">playSound();</script>';
            }
          }
          $result->free();
          $conn->close();
        }

        ?>

      </table>
    </div>

    <div class="center">
      <div class="iframe">
        <iframe src="http://localhost/sensenuts_project/temp_system.php"></iframe>
        <iframe src="http://localhost/sensenuts_project/light.php"></iframe>
      </div>
    </div>
  </div>




</body>


</html>