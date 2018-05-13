<?php 
$client_name=$_GET["client_name"];
$message=$_GET["message"];
$con = mysqli_connect("localhost","duckweather","duckweather","duckweather");
if (!$con)
  {
  die('Could not connect: ' . mysqli_error());
  echo 'error sql connect';
  }
$result_message=mysqli_query($con,"SELECT * FROM message WHERE client_name='".$client_name."'");  
 if(mysqli_num_rows($result_message)==0)
{
mysqli_query($con,"INSERT INTO message (client_name,message) VALUES ('".$client_name."', '".$message."')");
echo("success");
}
else
{
$row=mysqli_fetch_array($result_message);
mysqli_query($con,"UPDATE message SET message='".$message."' WHERE client_name='".$client_name."'");
echo("success");
} 
  
?>