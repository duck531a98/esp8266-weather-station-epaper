<?php 
error_reporting(E_ERROR | E_PARSE);
date_default_timezone_set("Asia/Shanghai");
$city=$_GET["city"];echo $city;
$lang=$_GET["lang"];
$client_name=$_GET["client_name"];
$message;
class weather{
    public $aqi;
    public $now;
    public $today;
    public $tomorrow;
    public $thedayaftertomorrow;
    public $city;
}
$con = mysqli_connect("localhost","duckweather","duckweather","duckweather");
if (!$con)
  {
  die('Could not connect: ' . mysqli_error());
  echo 'error sql connect';
  }
//mysqli_select_db("id2064398_database", $con);  
$result_message=mysqli_query($con,"SELECT * FROM message WHERE client_name='".$client_name."'");
if(mysqli_num_rows($result_message)==0)
{
$message=" ";
}
else
{
$row=mysqli_fetch_array($result_message);
$message=$row['message'];
}
$result=mysqli_query($con,"SELECT * FROM weatherdata WHERE city='".$city."' AND lang='".$lang."'");
if(mysqli_num_rows($result)==0)
{
$url="https://free-api.heweather.com/v5/weather?city=".$city."&key=ae71d94bace847879eadddad28e17776&lang=".$lang;  
$html = file_get_contents($url); 
mysqli_query($con,"INSERT INTO weatherdata (city, lang, weatherdata, updatetime) VALUES ('".$city."', '".$lang."','".$html."','".date("Y-m-d H:i:s")."')");
//mysqli_query($con,"INSERT INTO weatherdata (city, weatherdata, updatetime) VALUES ('test', 'data', '2017-6-10 15:15:15')");
echo "not exist";
}
else
{
echo 'cached';
$row=mysqli_fetch_array($result);
//echo strtotime(date("Y-m-d H:i:s"))-strtotime($row['updatetime']);
$minute=(strtotime(date("Y-m-d H:i:s"))-strtotime($row['updatetime']))/60;	
//echo $minute;
if($minute>=60.0)
{
echo 'new';
$url="https://free-api.heweather.com/v5/weather?city=".$city."&key=ae71d94bace847879eadddad28e17776&lang=".$lang;  
$html = file_get_contents($url); 
mysqli_query($con,"UPDATE weatherdata SET weatherdata='".$html."', updatetime='".date("Y-m-d H:i:s")."' , lang='".$lang."' WHERE city='".$city."' AND lang='".$lang."'");

}	
else
{
$html=$row['weatherdata'];
}	
	
}

  mysqli_close($con);

$arr = array("100"=>"0","101"=>"1","102"=>"2","103"=>"3","104"=>"4","200"=>"5","201"=>"6","202"=>"7","203"=>"8","204"=>"9","205"=>"10","206"=>"11","207"=>"12","208"=>"13","209"=>"14","210"=>"15","211"=>"16","212"=>"17","213"=>"18","300"=>"19","301"=>"20","302"=>"21","303"=>"22","304"=>"23","305"=>"24","306"=>"25","307"=>"26","308"=>"27","309"=>"28","310"=>"29","311"=>"30","312"=>"31","313"=>"32","400"=>"33","401"=>"34","402"=>"35","403"=>"36","404"=>"37","405"=>"38","406"=>"39","407"=>"40","500"=>"41","501"=>"42","502"=>"43","503"=>"44","504"=>"45","507"=>"46","508"=>"47","900"=>"48","901"=>"49","999"=>"50");
//$url="https://free-api.heweather.com/v5/weather?city=".$city."&key=fab38a62dcaa4630aa9eac1a3ae097aa";  
//$html = file_get_contents($url);  
$json=$html;
$obj=json_decode($html);

$weatherdata=new weather();


$weatherdata->aqi=$obj->HeWeather5[0]->aqi->city;

$weatherdata->now->cond=$obj->HeWeather5[0]->now->cond->code;
$weatherdata->now->cond_index=$arr[$obj->HeWeather5[0]->now->cond->code];
$weatherdata->now->hum=$obj->HeWeather5[0]->now->hum;
$weatherdata->now->tmp=$obj->HeWeather5[0]->now->tmp;
$weatherdata->now->dir=$obj->HeWeather5[0]->now->wind->dir;
$weatherdata->now->sc=$obj->HeWeather5[0]->now->wind->sc;

$weatherdata->today->cond_d=$obj->HeWeather5[0]->daily_forecast[0]->cond->code_d;
$weatherdata->today->txt_d=$obj->HeWeather5[0]->daily_forecast[0]->cond->txt_d;
$weatherdata->today->txt_n=$obj->HeWeather5[0]->daily_forecast[0]->cond->txt_n;
$weatherdata->today->cond_d_index=$arr[$obj->HeWeather5[0]->daily_forecast[0]->cond->code_d];
$weatherdata->today->cond_n=$obj->HeWeather5[0]->daily_forecast[0]->cond->code_n;
$weatherdata->today->cond_n_index=$arr[$obj->HeWeather5[0]->daily_forecast[0]->cond->code_n];
$weatherdata->today->tmpmax=$obj->HeWeather5[0]->daily_forecast[0]->tmp->max;
$weatherdata->today->tmpmin=$obj->HeWeather5[0]->daily_forecast[0]->tmp->min;
$weatherdata->today->date=$obj->HeWeather5[0]->daily_forecast[0]->date;

$weatherdata->tomorrow->cond_d=$obj->HeWeather5[0]->daily_forecast[1]->cond->code_d;
$weatherdata->tomorrow->txt_d=$obj->HeWeather5[0]->daily_forecast[1]->cond->txt_d;
$weatherdata->tomorrow->txt_n=$obj->HeWeather5[0]->daily_forecast[1]->cond->txt_n;
$weatherdata->tomorrow->cond_d_index=$arr[$obj->HeWeather5[0]->daily_forecast[1]->cond->code_d];
$weatherdata->tomorrow->cond_n=$obj->HeWeather5[0]->daily_forecast[1]->cond->code_n;
$weatherdata->tomorrow->cond_n_index=$arr[$obj->HeWeather5[0]->daily_forecast[1]->cond->code_n];
$weatherdata->tomorrow->tmpmax=$obj->HeWeather5[0]->daily_forecast[1]->tmp->max;
$weatherdata->tomorrow->tmpmin=$obj->HeWeather5[0]->daily_forecast[1]->tmp->min;

$weatherdata->thedayaftertomorrow->cond_d=$obj->HeWeather5[0]->daily_forecast[2]->cond->code_d;
$weatherdata->thedayaftertomorrow->cond_d_index=$arr[$obj->HeWeather5[0]->daily_forecast[2]->cond->code_d];
$weatherdata->thedayaftertomorrow->cond_n=$obj->HeWeather5[0]->daily_forecast[2]->cond->code_n;
$weatherdata->thedayaftertomorrow->cond_n_index=$arr[$obj->HeWeather5[0]->daily_forecast[2]->cond->code_n];
$weatherdata->thedayaftertomorrow->tmpmax=$obj->HeWeather5[0]->daily_forecast[2]->tmp->max;
$weatherdata->thedayaftertomorrow->tmpmin=$obj->HeWeather5[0]->daily_forecast[2]->tmp->min;

$weatherdata->city=$obj->HeWeather5[0]->basic->city;
$weatherdata->message=$message;
echo json_encode($weatherdata,JSON_UNESCAPED_UNICODE);
?>  