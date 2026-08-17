#define WIFI_getChipId() ESP.getChipId()

const char QWEATHER_INDEX[] PROGMEM=R"QWHTML(
<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
  <meta http-equiv="Cache-Control" content="no-store">
  <title>墨水屏天气站设置</title>
  <style>
    :root{color-scheme:light;--ink:#20242a;--muted:#68707b;--line:#d9dde2;--paper:#fff;--accent:#1769aa}
    *{box-sizing:border-box}
    body{margin:0;background:#f1f3f5;color:var(--ink);font:16px/1.45 system-ui,-apple-system,"Segoe UI","Microsoft YaHei",sans-serif}
    main{width:min(100% - 24px,560px);margin:18px auto;padding:20px;background:var(--paper);border-radius:14px;box-shadow:0 4px 18px #0001}
    h1{margin:0 0 6px;font-size:1.45rem}
    .lead{margin:0 0 20px;color:var(--muted);font-size:.92rem}
    .field{margin:0 0 15px}
    label{display:block;margin-bottom:6px;font-weight:650}
    small{display:block;margin-top:5px;color:var(--muted)}
    input,select,button{width:100%;min-height:46px;border:1px solid var(--line);border-radius:9px;background:#fff;color:var(--ink);font:inherit}
    input,select{padding:10px 12px}
    input:focus,select:focus{border-color:var(--accent);outline:2px solid #1769aa26}
    .row{display:grid;grid-template-columns:1fr 1fr;gap:12px}
    .actions{display:grid;gap:10px;margin-top:22px}
    button{border:0;background:var(--accent);color:#fff;font-weight:700}
    button.secondary{background:#535b65}
    button:disabled{opacity:.65}
    #result{min-height:1.45em;margin:12px 0 0;text-align:center}
    #result.ok{color:#18733a}
    #result.error{color:#b42318}
    @media(max-width:420px){main{width:100%;min-height:100vh;margin:0;border-radius:0;padding:18px 16px}.row{grid-template-columns:1fr}}
  </style>
</head>
<body>
<main>
  <h1>墨水屏天气站设置</h1>
  <p class="lead">设备将直接连接和风天气。密码和 API Key 只保存在设备中，本页不会读取或显示它们。</p>

  <form id="settings" autocomplete="off">
    <div class="field">
      <label for="ssid">Wi-Fi 名称</label>
      <input id="ssid" name="ssid" type="text" maxlength="32" required autocomplete="username">
    </div>
    <div class="field">
      <label for="password">Wi-Fi 密码</label>
      <input id="password" name="password" type="password" maxlength="64" autocomplete="new-password">
      <small id="password_hint">开放网络可留空</small>
    </div>
    <div class="field">
      <label for="city">天气城市或 LocationID</label>
      <input id="city" name="city" type="text" maxlength="64" required placeholder="例如：北京、101010100 或经度,纬度">
    </div>
    <div class="field">
      <label for="qweather_host">和风天气 API Host</label>
      <input id="qweather_host" name="qweather_host" type="text" maxlength="96" required
             inputmode="url" autocapitalize="none" spellcheck="false" placeholder="控制台中的专属 API Host">
      <small>只填写主机名，不要填写 https:// 或路径。</small>
    </div>
    <div class="field">
      <label for="qweather_api_key">和风天气 API Key</label>
      <input id="qweather_api_key" name="qweather_api_key" type="password" maxlength="64"
             autocomplete="new-password" autocapitalize="none" spellcheck="false">
      <small id="qweather_api_key_hint">请输入控制台创建的 API Key</small>
    </div>
    <div class="field">
      <label for="screen">屏幕类型</label>
      <select id="screen" name="screen">
        <option value="wx29">2.9 寸 HINK</option>
        <option value="wf29">2.9 寸 WFT</option>
        <option value="opm42">4.2 寸 HINK / OPM</option>
        <option value="wft29bz03">WFT0000BZ03</option>
        <option value="dke42">4.2 寸佳显</option>
        <option value="dke29">2.9 寸佳显</option>
        <option value="wf42">4.2 寸 WF</option>
        <option value="wf32">3.2 寸 WF</option>
      </select>
    </div>
    <div class="row">
      <div class="field">
        <label for="showtime">显示时间</label>
        <select id="showtime" name="showtime">
          <option value="0">不显示</option>
          <option value="1">显示</option>
        </select>
      </div>
      <div class="field">
        <label for="interval">更新间隔</label>
        <select id="interval" name="interval">
          <option value="1">1 小时</option>
          <option value="2">2 小时</option>
          <option value="3">3 小时</option>
          <option value="4">4 小时</option>
        </select>
      </div>
    </div>
    <div class="row">
      <div class="field">
        <label for="start">开始更新（时）</label>
        <input id="start" name="start" type="number" min="0" max="23" step="1" value="0" inputmode="numeric">
      </div>
      <div class="field">
        <label for="end">停止更新（时）</label>
        <input id="end" name="end" type="number" min="1" max="24" step="1" value="24" inputmode="numeric">
      </div>
    </div>
    <div class="field">
      <label for="contrast">屏幕对比度</label>
      <input id="contrast" name="contrast" type="range" min="0" max="255" value="80">
    </div>
    <div class="actions">
      <button id="save_button" type="submit">保存设置</button>
      <button id="reset_button" class="secondary" type="button">关闭热点并开始运行</button>
    </div>
    <p id="result" role="status" aria-live="polite"></p>
  </form>
</main>

<script>
(function(){
  "use strict";
  var form=document.getElementById("settings");
  var saveButton=document.getElementById("save_button");
  var result=document.getElementById("result");
  var publicFields=["ssid","city","qweather_host","screen","showtime","interval","start","end","contrast"];

  function show(message,isError){
    result.textContent=message;
    result.className=isError?"error":"ok";
  }

  function setSecretState(id,hintId,isSaved,emptyHint){
    var input=document.getElementById(id);
    input.value="";
    input.placeholder=isSaved?"已保存，留空保持不变":emptyHint;
    document.getElementById(hintId).textContent=isSaved?"已保存，留空保持不变":emptyHint;
  }

  function loadSettings(){
    var request=new XMLHttpRequest();
    request.onreadystatechange=function(){
      if(request.readyState!==4)return;
      if(request.status!==200){show("读取设置失败，请刷新页面重试",true);return;}
      try{
        var data=JSON.parse(request.responseText);
        publicFields.forEach(function(id){
          if(Object.prototype.hasOwnProperty.call(data,id)&&data[id]!==null){
            document.getElementById(id).value=String(data[id]);
          }
        });
        setSecretState("password","password_hint",Boolean(data.has_wifi_password),"开放网络可留空");
        setSecretState("qweather_api_key","qweather_api_key_hint",
                       Boolean(data.has_qweather_api_key),"请输入控制台创建的 API Key");
      }catch(error){
        show("设备返回的设置数据无效",true);
      }
    };
    request.open("GET","/readsettings",true);
    request.setRequestHeader("Cache-Control","no-store");
    request.send();
  }

  function encodeForm(){
    var names=["ssid","password","city","qweather_host","qweather_api_key",
               "screen","showtime","interval","start","end","contrast"];
    return names.map(function(name){
      return encodeURIComponent(name)+"="+encodeURIComponent(document.getElementById(name).value);
    }).join("&");
  }

  form.addEventListener("submit",function(event){
    event.preventDefault();
    if(!form.checkValidity()){form.reportValidity();return;}
    saveButton.disabled=true;
    saveButton.textContent="正在保存…";
    show("",false);
    var request=new XMLHttpRequest();
    request.onreadystatechange=function(){
      if(request.readyState!==4)return;
      saveButton.disabled=false;
      saveButton.textContent="保存设置";
      if(request.status>=200&&request.status<300){
        var passwordInput=document.getElementById("password");
        var keyInput=document.getElementById("qweather_api_key");
        var passwordSaved=passwordInput.value.length>0||passwordInput.placeholder.indexOf("已保存")===0;
        var keySaved=keyInput.value.length>0||keyInput.placeholder.indexOf("已保存")===0;
        setSecretState("password","password_hint",passwordSaved,"开放网络可留空");
        setSecretState("qweather_api_key","qweather_api_key_hint",keySaved,
                       "请输入控制台创建的 API Key");
        show(request.responseText||"设置已保存",false);
      }else{
        show(request.responseText||"保存失败，请检查输入后重试",true);
      }
    };
    request.onerror=function(){
      saveButton.disabled=false;
      saveButton.textContent="保存设置";
      show("保存失败：与设备的连接已中断",true);
    };
    request.open("POST","/saveconfig",true);
    request.setRequestHeader("Content-Type","application/x-www-form-urlencoded");
    request.send(encodeForm());
  });

  document.getElementById("reset_button").addEventListener("click",function(){
    if(!window.confirm("确定关闭配置热点并开始正常运行吗？"))return;
    var request=new XMLHttpRequest();
    request.open("GET","/reset",true);
    request.send();
    show("热点正在关闭，设备即将开始正常运行。",false);
  });

  loadSettings();
})();
</script>
</body>
</html>
)QWHTML";

const char SAVE_CONFIG_JS[] PROGMEM=R"JS(
function saveconfig(){
  var button=document.getElementById("save_button");
  button.value="正在保存...";
  var ids=["wifi_ssid","wifi_password","city","client_name","epd","showtime","update_interval","update_time","cdkey","contrast"];
  var query=[];
  for(var i=0;i<ids.length;i++){
    query.push(ids[i]+"="+encodeURIComponent(document.getElementById(ids[i]).value));
  }
  var xhttp=new XMLHttpRequest();
  xhttp.onreadystatechange=function(){
    if(this.readyState===4){
      button.value="保存设置";
      if(this.status===200) window.alert(this.responseText);
      else window.alert("保存失败："+this.responseText);
    }
  };
  xhttp.onerror=function(){
    button.value="保存设置";
    window.alert("保存失败：网络连接中断");
  };
  xhttp.open("GET","/saveconfig?"+query.join("&"),true);
  xhttp.send();
}
(function(){
  var wf58=document.querySelector('#epd option[value="wf58"]');
  if(wf58){
    wf58.disabled=true;
    wf58.textContent+="（当前固件暂不支持）";
  }
})();
)JS";
const char INDEX[] PROGMEM="<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /><title>设置</title></head><style>.text{}<style>body{padding-top:40px;}.weather{width:80vw;height:calc(80vw * 0.45);background:#252d3a;margin:0 auto;border-radius:30px;padding:20px;box-sizing:border-box;position:relative;}.key{width:calc(80vw * 0.05);height:8px;background:#BB3F3F;position:absolute;right:calc(80vw * 0.05 * 1 + 10px);top:-8px;}.key:nth-of-type(2){right:calc(80vw * 0.05 * 2 + 10px * 2);}.key:nth-of-type(3){right:calc(80vw * 0.05 * 3 + 10px * 3);}.screen{width:100%;height:100%;background:#383838;border-radius:20px;overflow:hidden;position:relative;}.sun{background:#383838;height:100%;width:40%;position:relative;left:0;top:0;}.sun-body{width:30px;height:calc(80vw * 0.45 * 0.8);position:absolute;left:50%;top:50%;background:-webkit-linear-gradient(top,rgba(255,255,255,0) 0%,rgba(255,255,255,0.8) 50%,rgba(255,255,255,0) 100%);animation:sunny 15s linear infinite;transform:translate(-50%,-50%) rotate(0deg);}@keyframes sunny{0%{transform:translate(-50%,-50%) rotate(0deg);}100%{transform:translate(-50%,-50%) rotate(360deg);}}.sun-body::before{content:'';width:30px;height:calc(80vw * 0.45 * 0.8);position:absolute;left:50%;top:50%;background:-webkit-linear-gradient(top,rgba(255,255,255,0) 0%,rgba(255,255,255,0.8) 50%,rgba(255,255,255,0) 100%);transform:translate(-50%,-50%) rotate(90deg);}.sun::after{content:'';width:calc(80vw * 0.45 * 0.4);height:calc(80vw * 0.45 * 0.4);position:absolute;left:50%;top:50%;background:#ffee44;border-radius:50%;transform:translate(-50%,-50%);box-shadow:rgba(255,255,0,0.2) 0 0 0 20px;}.content{background:#fff;width:60%;height:100%;position:absolute;right:0;top:0;border-radius:0 20px 20px 0;}.content::after{background:#383838;content:\"\";width:calc(100% - 20px);height:2px;position:absolute;top:calc(100% / 3 * 1);animation:content 2s linear infinite;}.content::before{background:#383838;content:\"\";width:calc(100% - 20px);height:2px;position:absolute;top:calc(100% / 3 * 2);animation:content 1.5s linear infinite;}@keyframes content{0%{width:calc(100% - 20px);}50%{width:calc(100% - 60px);}100%{width:calc(100% - 20px);}}</style><body onload=\"load()\"><div style=\"width:100%\"><div style=\"width:100%;margin:0 auto;\"><div class=\"weather\"><div class=\"key\"></div><div class=\"key\"></div><div class=\"key\"></div><div class=\"screen\"><div class=\"sun\"><div class=\"sun-body\"></div></div><div class=\"content\"></div></div></div></div><div style=\"height:10vw\"></div><div style=\"width:80%;margin:0 auto;\"><table ><tr style=\"width:5vw;text-align: center\"><td style=\"stroke: green;fill: green; width:100vw;\"  ><span style=\"font-size:3vw;\">AP:Epaper Weather Station</span></td></tr><tr style=\"width:5vw;text-align: center\"><td><span style=\"font-size:3vw;\" id=\"sta_status\">Station:未连接</span></td></tr><tr style=\"width:5vw;text-align: center\"><td><svg version=\"1.1\" id=\"图层_1\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" x=\"0px\" y=\"0px\"height=\"2.5vw\" viewBox=\"0 0 114.5 59.5\" enable-background=\"new 0 0 114.5 59.5\" xml:space=\"preserve\"><rect x=\"12\" y=\"4\" fill=\"#FFFFFF\" stroke=\"#000000\" stroke-width=\"4\" stroke-miterlimit=\"10\" width=\"99\" height=\"52\"/><rect x=\"3.167\" y=\"15.167\" fill=\"#040000\" stroke=\"#000000\" stroke-miterlimit=\"10\" width=\"7.667\" height=\"29.667\"/><rect x=\"88.333\" y=\"11.583\" fill=\"#040000\" stroke=\"#000000\" stroke-miterlimit=\"10\" width=\"14\" height=\"36.833\"/><rect x=\"66.833\" y=\"11.75\" fill=\"#040000\" stroke=\"#000000\" stroke-miterlimit=\"10\" width=\"14\" height=\"36.833\"/><rect x=\"45\" y=\"11.583\" fill=\"#040000\" stroke=\"#000000\" stroke-miterlimit=\"10\" width=\"14\" height=\"36.833\"/><rect x=\"22.5\" y=\"11.75\" fill=\"#040000\" stroke=\"#000000\" stroke-miterlimit=\"10\" width=\"14\" height=\"36.833\"/></svg><span style=\"font-size:3vw;\" id=\"batt_vol\">未知</span></td></tr></table></div><div style=\"width: auto; margin: 0 auto; margin-top: 10vw; \"><table><tr><td></tr></table></div><div  style=\"font-size:4vw;width:90%;margin:0 auto;\"><table  style=\"width: 90%; margin: 0 auto; \"><tr><td colspan=\"2\"></td></tr><tr><td>WIFI名称</td><td><input type=\"text\" id=\"wifi_ssid\" name=\"wifi_ssid\" style=\"font-size:3.5vw;\" value=\"\"></td></tr><tr><td>WIFI密码</td><td><input type=\"text\" id=\"wifi_password\" name=\"wifi_password\" style=\"font-size:3.5vw;\" value=\"\"></td></tr><tr><td>天气预报城市</td><td><input type=\"text\" id=\"city\" name=\"city\" style=\"font-size:3.5vw;\" value=\"\"></td></tr><tr><td>客户端名称</td><td><input type=\"text\" id=\"client_name\" name=\"client_name\" style=\"font-size:3.5vw;\" value=\"\"></td></tr><tr><td>激活码</td><td><input type=\"text\" id=\"cdkey\" name=\"cdkey\" style=\"font-size:3.5vw;\" value=\"\"></td></tr><tr><td>屏幕类型</td><td><select name=\"epd\" id=\"epd\" style=\"font-size:3.5vw;width:100%;height:6vw\"><option value=\"wx29\">2.9寸HINK</option><option value=\"wf29\">2.9寸WFT</option><option value=\"opm42\">4.2寸HINK/OPM</option><option value=\"wf58\">5.8寸WFT</option><option value=\"wft29bz03\">WFT0000BZ03</option><option value=\"dke42\">4.2寸佳显</option><option value=\"dke29\">2.9寸佳显</option><option value=\"wf42\">4.2寸WF</option><option value=\"wf32\">3.2寸WF</option></select></td></tr><tr><td >是否显示时间</td><td ><select name=\"showtime\" id=\"showtime\" style=\"font-size:3.5vw;width:100%;height:6vw\"><option value=\"0\">不显示</option><option value=\"1\">显示</option></select></td></tr><tr><td >天气更新间隔</td><td ><select name=\"update_interval\" id=\"update_interval\" style=\"font-size:3.5vw;width:100%;height:6vw\"><option value=\"1\">1小时</option><option value=\"2\">2小时</option><option value=\"3\">3小时</option><option value=\"4\">4小时</option></select></td></tr><tr><td >天气更新时段</td><td ><select name=\"update_time\" id=\"update_time\" style=\"font-size:3.5vw;width:100%;height:6vw\"><option value=\"1\">全天</option><option value=\"2\">6点-24点</option></select></td></tr><tr><!--<td >电子相框</td><td ><select name=\"photo_frame\" id=\"photo_frame\" style=\"font-size: 3.5vw; width: 100%; height: 6vw;\"><option value=\"1\">开启</option><option value=\"2\">关闭</option></select></td>--></tr><tr style=\"visibility: hidden;\"><td >对比度调节</td><td ><input type=\"range\" name=\"contrast\" id=\"contrast\" min=\"0\" max=\"255\" style=\"font-size: 3.5vw; width: 100%; height: 6vw;\"></td></tr><tr><td colspan=\"2\"><input onclick=\"saveconfig()\" id=\"save_button\" type=\"submit\" value=\"保存设置\"  style=\"font-size: 3.5vw; width: 100%; height: 10vw; background-color: #3A3A3A; color: #FFFFFF;\"></td></tr><tr><td colspan=\"2\"><input onclick=\"reset()\" type=\"submit\" value=\"关闭AP开始正常运行\" style=\"font-size: 3.5vw; width: 100%; height: 10vw; background-color: #3A3A3A; color: #FFFFFF;\"></td></tr><tr><td colspan=\"2\"><input onclick=\"javascript:top.location='/photo';\"  type=\"submit\" value=\"上传图片\" style=\"font-size: 3.5vw; width: 100%; height: 10vw; background-color: #3A3A3A; color: #FFFFFF;\"></td></tr><tr><td colspan=\"2\"><input onclick=\"javascript:top.location='/file';\"  type=\"submit\" value=\"文件管理\" style=\"font-size: 3.5vw; width: 100%; height: 10vw; background-color: #3A3A3A; color: #FFFFFF;\"></td></tr></table></div></div><a href=\"changelog.html\"><span style=\"font-size:3vw;float:right\">固件版本:1.30</span></a></body></html><script type=\"text/javascript\">var int=self.setInterval(\"readstatus()\",1000);function load(){var xhttp = new XMLHttpRequest();xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {data = this.responseText;var obj = JSON.parse(data);document.getElementById(\"wifi_ssid\").value = obj.wifi_ssid;document.getElementById(\"wifi_password\").value = obj.wifi_password;document.getElementById(\"city\").value = obj.city;document.getElementById(\"client_name\").value = obj.client_name;document.getElementById(\"epd\").value = obj.epd;document.getElementById(\"showtime\").value = obj.showtime;document.getElementById(\"update_interval\").value = obj.update_interval;document.getElementById(\"update_time\").value = obj.update_time;document.getElementById(\"contrast\").value = obj.contrast;document.getElementById(\"cdkey\").value = obj.cdkey;}};xhttp.open(\"GET\", \"/readsettings\", true);xhttp.send();}function saveconfig(){document.getElementById(\"save_button\").value=\"正在保存...\";var xhttp = new XMLHttpRequest();xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {data = this.responseText;window.alert(data);}};xhttp.open(\"GET\", \"/saveconfig?wifi_ssid=\"+document.getElementById(\"wifi_ssid\").value+\"&wifi_password=\"+document.getElementById(\"wifi_password\").value+\"&city=\"+document.getElementById(\"city\").value+\"&client_name=\"+document.getElementById(\"client_name\").value+\"&epd=\"+document.getElementById(\"epd\").value+\"&showtime=\"+document.getElementById(\"showtime\").value+\"&update_interval=\"+document.getElementById(\"update_interval\").value+\"&update_time=\"+document.getElementById(\"update_time\").value+\"&cdkey=\"+document.getElementById(\"cdkey\").value+\"&contrast=\"+document.getElementById(\"contrast\").value, true);xhttp.send();document.getElementById(\"save_button\").value=\"保存设置\";}function reset(){var xhttp = new XMLHttpRequest();xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {data = this.responseText;window.close();}};xhttp.open(\"GET\", \"/reset\",true);xhttp.send();window.alert(\"注意，AP立即关闭，此页面失效\");}function readstatus(){var xhttp = new XMLHttpRequest();xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {data = this.responseText;var obj = JSON.parse(data);document.getElementById(\"sta_status\").textContent=obj.sta_ssid;document.getElementById(\"batt_vol\").textContent=obj.batt_vol;}};xhttp.open(\"GET\", \"/readstatus\",true);xhttp.send();}</script>";
const char INDEX_JS[] PROGMEM="var int=self.setInterval(\"readstatus()\",1000);function load(){var xhttp = new XMLHttpRequest();xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {data = this.responseText;var obj = JSON.parse(data);document.getElementById(\"wifi_ssid\").value = obj.wifi_ssid;document.getElementById(\"wifi_password\").value = obj.wifi_password;document.getElementById(\"city\").value = obj.city;document.getElementById(\"client_name\").value = obj.client_name;document.getElementById(\"epd\").value = obj.epd;document.getElementById(\"showtime\").value = obj.showtime;document.getElementById(\"update_interval\").value = obj.update_interval;document.getElementById(\"update_time\").value = obj.update_time;document.getElementById(\"contrast\").value = obj.contrast;document.getElementById(\"cdkey\").value = obj.cdkey;}};xhttp.open(\"GET\", \"/readsettings\", true);xhttp.send();}function saveconfig(){document.getElementById(\"save_button\").value=\"正在保存...\";var xhttp = new XMLHttpRequest();xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {data = this.responseText;window.alert(data);}};xhttp.open(\"GET\", \"/saveconfig?wifi_ssid=\"+document.getElementById(\"wifi_ssid\").value+\"&wifi_password=\"+document.getElementById(\"wifi_password\").value+\"&city=\"+document.getElementById(\"city\").value+\"&client_name=\"+document.getElementById(\"client_name\").value+\"&epd=\"+document.getElementById(\"epd\").value+\"&showtime=\"+document.getElementById(\"showtime\").value+\"&update_interval=\"+document.getElementById(\"update_interval\").value+\"&update_time=\"+document.getElementById(\"update_time\").value+\"&cdkey=\"+document.getElementById(\"cdkey\").value+\"&contrast=\"+document.getElementById(\"contrast\").value, true);xhttp.send();document.getElementById(\"save_button\").value=\"保存设置\";}function reset(){var xhttp = new XMLHttpRequest();xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {data = this.responseText;window.close();}};xhttp.open(\"GET\", \"/reset\",true);xhttp.send();window.alert(\"注意，AP立即关闭，此页面失效\");}function readstatus(){var xhttp = new XMLHttpRequest();xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {data = this.responseText;var obj = JSON.parse(data);document.getElementById(\"sta_status\").textContent=obj.sta_ssid;document.getElementById(\"batt_vol\").textContent=obj.batt_vol;}};xhttp.open(\"GET\", \"/readstatus\",true);xhttp.send();}";
const char CONFIG[] PROGMEM="<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /><title>设置</title></head><body onload=\"load()\"><div style=\"width:100%\"><form action=\"/config\" style=\"font-size:5vw;\"><table ><tr><td>WIFI名称</td><td><input type=\"text\" id=\"ssid\" name=\"ssid\" style=\"font-size:5vw;\" value=\"\"></td></tr><tr><td>WIFI密码</td><td><input type=\"text\" id=\"pass\" name=\"pass\" style=\"font-size:5vw;\" value=\"\"></td></tr><tr><td>ONENET_ID</td><td><input type=\"text\" id=\"onenet_id\" name=\"onenet_id\" style=\"font-size:5vw;\" value=\"\"></td></tr><tr><td>ONENET_KEY</td><td><input type=\"text\" id=\"onenet_key\" name=\"onenet_key\" style=\"font-size:5vw;\" value=\"\"></td></tr><tr><td>天气预报城市</td><td><input type=\"text\" id=\"city\" name=\"city\" style=\"font-size:5vw;\" value=\"\"></td></tr><tr><td>客户端名称</td><td><input type=\"text\" id=\"client_name\" name=\"client_name\" style=\"font-size:5vw;\" value=\"\"></td></tr><tr><td colspan=\"2\"><input type=\"submit\" value=\"保存\" style=\"font-size:5vw;width:100%;height:10vw\"></td></tr></table></form></div></body></html><script type=\"text/javascript\">function load(){var xhttp = new XMLHttpRequest();var data=\"\";var datadiv=\"\",countdiv=\"\";var sum=0;xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {data = this.responseText;var obj = JSON.parse(data);document.getElementById(\"onenet_id\").value = obj.onenet_id;document.getElementById(\"onenet_key\").value = obj.onenet_key;document.getElementById(\"city\").value = obj.city;document.getElementById(\"client_name\").value = obj.client_name;document.getElementById(\"ssid\").value = obj.ssid;document.getElementById(\"pass\").value = obj.pass;}};xhttp.open(\"GET\", \"/settings\", true);xhttp.send();}</script>";
const char PHOTO[] PROGMEM="<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /><title>photo</title></head><style>#fullscreen {background-color    :black;position            :absolute;left                :0;top                 :0;width               :100%;height              :100%;filter              :alpha(Opacity=50);-moz-opacity        :0.5;opacity             :0.5;z-index             :100;visibility: hidden;}</style><body><div id=\"fullscreen\"><div style=\"font-size: 10vw; color: aliceblue; margin: 0 auto; vertical-align: middle; \">上传中……请耐心等待……很慢</div></div><div style=\"width:100%\"><div style=\"width:100%\"><canvas id=\"canva\" width=\"400\" height=\"300\" style=\"background-color: #9C9C9C; margin: 0 auto; width:100%;\"></canvas></div></div><input  onclick=\"javascript:document.getElementById('file').click()\"  id=\"save_button\" type=\"button\" value=\"选择文件\" style=\"font-size:3.5vw;width:100%;height:10vw;background-color:#3A3A3A;color:#FFFFFF;\"><form  method=\"post\" enctype=\"multipart/form-data\" action=\"/upload.php\"><input id=\"file\" type=\"file\" name=\"file\" onchange=\"preImg(this.id,'imgPre');\" style=\"visibility: hidden;\"><input class=\"button\" id=\"upload_file\" type=\"submit\" style=\"visibility: hidden;\" value=\"Upload\"></form><button onclick=\"upload()\" style=\"font-size: 3.5vw; width: 100%; height: 10vw; background-color: #3A3A3A; color: #FFFFFF;\">上传并显示</button><button onclick=\"gray()\" style=\"font-size: 3.5vw; width: 100%; height: 10vw; background-color: #3A3A3A; color: #FFFFFF; visibility:hidden;\">转灰度</button><p>亮度调节</p><input type=\"range\" id=\"contrast\" onchange=\"contrast()\" min=\"0\" max=\"200\" style=\"width:100%;\"><p></p><div id=\"progress\" style=\"width: 0%; height: 4vw; background-color: #000000;\"></div><img id=\"imgPre\" src=\"\" style=\"display: block; visibility: hidden;\" /></body></html><script>var canvas = document.getElementById(\"canva\");var cxt = canvas.getContext(\"2d\");var img;var init_width=400;var ix1,iy1,ix2,iy2;var tx1,ty1,tx2,ty2;var last_posx=0;var last_posy=0;var o_dis,c_dis;var offset=document.getElementById(\"contrast\").value;document.getElementById(\"canva\").addEventListener(\"touchstart\", touch);document.getElementById(\"canva\").addEventListener(\"touchmove\", touch);document.getElementById(\"canva\").addEventListener(\"touchend\", touch);function preImg(sourceId, targetId){cxt.clearRect(0, 0, 400, 400);if (typeof FileReader === 'undefined') {alert('Your browser does not support FileReader...');return;}var reader = new FileReader();reader.onload = function (e) {img = document.getElementById(targetId);var cxt = document.getElementById(\"canva\").getContext(\"2d\");img.src = this.result;img.onload = function () {cxt.drawImage(img, 0, 0, 400,img.height*400/img.width);}};reader.readAsDataURL(document.getElementById(sourceId).files[0]);}function gray(offset){cxt.drawImage(img, last_posx, last_posy, init_width,img.height*init_width/img.width);cxt = document.getElementById(\"canva\").getContext(\"2d\");let imageData = cxt.getImageData(0,0,canvas.width,canvas.height);let pixels = imageData.data;for (let i=0; i<pixels.length; i+=4){let r = pixels[i];let g = pixels[i+1];let b = pixels[i+2];let gray = parseInt(offset/100*(r+g+b)/3);pixels[i] = gray;pixels[i+1] = gray;pixels[i+2] = gray;}cxt.putImageData(imageData, 0,0);}function touch(event){switch(event.type){case \"touchstart\":ix1=event.touches[0].clientX-last_posx;iy1=event.touches[0].clientY-last_posy;if(event.touches.length==2){tx2=event.touches[1].clientX;ty2=event.touches[1].clientY;tx1=event.touches[0].clientX;ty1=event.touches[0].clientY;o_dis=Math.sqrt((tx1-tx2)*(tx1-tx2)+(ty1-ty2)*(ty1-ty2));}break;case \"touchend\":gray(offset);break;case \"touchmove\":event.preventDefault();if(event.touches.length==1){cxt.clearRect(0,0,400,400);cxt.drawImage(img, event.touches[0].clientX-ix1, event.touches[0].clientY-iy1, init_width,img.height*init_width/img.width);last_posx=event.touches[0].clientX-ix1;last_posy=event.touches[0].clientY-iy1;}else if(event.touches.length==2){cxt.clearRect(0,0,400,400);tx2=event.touches[1].clientX;ty2=event.touches[1].clientY;tx1=event.touches[0].clientX;ty1=event.touches[0].clientY;c_dis=Math.sqrt((tx1-tx2)*(tx1-tx2)+(ty1-ty2)*(ty1-ty2));init_width=400*c_dis/o_dis;cxt.drawImage(img, last_posx, last_posy, init_width,img.height*init_width/img.width);}break;}}var button = document.querySelector(\"#btn-upload\"),input = document.querySelector(\"#myfile\"),progress = document.querySelector(\"#progress\"),info = document.querySelector(\"#info\");var upload = function() {/*if (input.files.length === 0) {console.log(\"未选择文件\");return;}*/document.getElementById(\"fullscreen\").style.visibility=\"visible\";for(var i=0;i<1e6;i++);progress.style.width=\"0%\";cxt = document.getElementById(\"canva\").getContext(\"2d\");let imageData = cxt.getImageData(0,0,canvas.width,canvas.height);console.log(\"canvas.width=\"+canvas.width);let pixels = imageData.data;var shit=new Uint8Array(canvas.width*canvas.height/2);console.log(\"length\"+pixels.length);for(let i=0;i<canvas.width;i+=1){for(let j=0;j<canvas.height;j+=1){var pos=j*canvas.width*4+i*4;var r = pixels[pos];var g = pixels[pos+1];var b = pixels[pos+2];var gray = parseInt((r+g+b)/3);var shitpos=parseInt((i*canvas.height+j)/2);if((i*canvas.height+j)%2==0){shit[shitpos]|=(gray/16)<<4;}else{shit[shitpos]|=gray/16;}}}var formData = new FormData();var parts = [shit];var myFile=new window.File(parts,\"pic.xbm\");/*formData.append(\"file\", input.files[0]);*/formData.append(\"file\", myFile);var xhr = new XMLHttpRequest();xhr.onreadystatechange = function() {if (xhr.readyState === 4 && xhr.status === 200) {console.log(xhr.responseText);document.getElementById(\"fullscreen\").style.visibility=\"hidden\";/*info.innerHTML = xhr.responseText;*/}};xhr.onloadstart = function (event) {document.getElementById(\"fullscreen\").style.visibility=\"visible\";console.log(event.lengthComputable);console.log(event.loaded);if (event.lengthComputable) {var loaded = parseInt(event.loaded / event.total * 100) + \"%\";progress.style.width=loaded;}};xhr.open(\"POST\", \"/upload\");xhr.send(formData);};function contrast(){offset=document.getElementById(\"contrast\").value;gray(offset);}/*button.addEventListener(\"click\", upload, false);*/</script>";
const char FILE_WEBPAGE[] PROGMEM="<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /><title>文件管理</title></head><style>#file_list {font-size:4vw;}td{width:33vw;text-align:right;}</style><body onload=\"get_file_list()\"><div  style=\"width:100%\"><div style=\"width: 100%; height: 4vw;  border: medium solid #000000;\"><div id=\"disk_percentage\" style=\"width: 10%; height: 4vw; background-color: #000000;\"></div></div><div id=\"disk_percentage_text\" style=\"font-size: 4vw; text-align: right;\">0KB/0KB</div><p></p><table style=\"font-size:4vw;\"><tr ><td style=\"text-align:left;\">文件名</td><td>大小</td><td style=\"text-align:right;\"></td></tr></table><table id=\"file_list\"><tr><td style=\"text-align:left;\">font12</td><td>596K</td><td id=\"font12\" onclick=\"delete_file(this.id)\"  style=\"text-align:right;\">删除</td></tr></table></div></body></html><script>function get_file_list(){var xhttp = new XMLHttpRequest();xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {data = this.responseText;var obj = JSON.parse(data);document.getElementById(\"disk_percentage_text\").innerHTML = obj.used;document.getElementById(\"file_list\").innerHTML=\"\";document.getElementById(\"disk_percentage\").style.width=obj.percentage+\"%\";for(var i in obj.file_list){document.getElementById(\"file_list\").innerHTML+=\"<tr>\"+\"<td style='text-align:left;'>\"+obj.file_list[i].name+\"</td>\"+\"<td>\"+obj.file_list[i].size+\"</td>\"+\"<td id='\"+obj.file_list[i].name+\"' onclick='delete_file(this.id)'  style='text-align:right;'>删除</td>\"+\"</tr>\"}}};xhttp.open(\"GET\", \"/get_file_list\", true);xhttp.send();}function delete_file(filename){if(confirm(\"确定要删除文件\"+filename+\"吗？\")){var xhttp = new XMLHttpRequest();xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {data = this.responseText;var obj = JSON.parse(data);get_file_list();}};xhttp.open(\"GET\", \"/delete_file?filename=\"+filename, true);xhttp.send();}}</script>";

const char PHOTO_UPLOAD_FEEDBACK_JS[] PROGMEM=R"JS(
(function(){
  var uploadBusy=false;
  var imageReady=false;
  var imageGeneration=0;
  var nativeOpen=XMLHttpRequest.prototype.open;
  var nativeSend=XMLHttpRequest.prototype.send;
  var nativeUpload=upload;
  preImg=function(sourceId,targetId){
    imageGeneration++;
    var generation=imageGeneration;
    imageReady=false;
    var input=document.getElementById(sourceId);
    if(!input||!input.files||!input.files[0]) return;
    if(typeof FileReader==="undefined"){
      window.alert("当前浏览器不支持图片读取");
      return;
    }
    cxt.clearRect(0,0,canvas.width,canvas.height);
    var reader=new FileReader();
    reader.onerror=function(){
      if(generation===imageGeneration) window.alert("图片读取失败");
    };
    reader.onload=function(event){
      if(generation!==imageGeneration) return;
      var candidate=new Image();
      candidate.onerror=function(){
        if(generation===imageGeneration) window.alert("图片解码失败");
      };
      candidate.onload=function(){
        if(generation!==imageGeneration) return;
        img=candidate;
        init_width=400;
        last_posx=0;
        last_posy=0;
        cxt=document.getElementById("canva").getContext("2d");
        cxt.clearRect(0,0,canvas.width,canvas.height);
        cxt.drawImage(candidate,0,0,400,candidate.height*400/candidate.width);
        document.getElementById(targetId).src=event.target.result;
        imageReady=true;
      };
      candidate.src=event.target.result;
    };
    reader.readAsDataURL(input.files[0]);
  };
  upload=function(){
    if(uploadBusy){
      window.alert("图片正在上传，请勿重复提交");
      return;
    }
    if(!imageReady){
      window.alert("请先选择图片并等待预览完成");
      return;
    }
    uploadBusy=true;
    var uploadButton=document.querySelector('button[onclick="upload()"]');
    if(uploadButton) uploadButton.disabled=true;
    try{
      nativeUpload();
    }catch(error){
      uploadBusy=false;
      if(uploadButton) uploadButton.disabled=false;
      var overlay=document.getElementById("fullscreen");
      if(overlay) overlay.style.visibility="hidden";
      window.alert("图片处理失败");
    }
  };
  XMLHttpRequest.prototype.open=function(method,url){
    this.__photoUpload=(url==="/upload");
    return nativeOpen.apply(this,arguments);
  };
  XMLHttpRequest.prototype.send=function(body){
    if(this.__photoUpload){
      this.addEventListener("loadend",function(){
        uploadBusy=false;
        var uploadButton=document.querySelector('button[onclick="upload()"]');
        if(uploadButton) uploadButton.disabled=false;
        var overlay=document.getElementById("fullscreen");
        if(overlay) overlay.style.visibility="hidden";
        if(this.status!==200) window.alert(this.responseText||"图片上传失败");
      },false);
    }
    return nativeSend.apply(this,arguments);
  };
})();
)JS";

const char FILE_MANAGER_SAFE_JS[] PROGMEM=R"JS(
function get_file_list(){
  var xhttp=new XMLHttpRequest();
  xhttp.onreadystatechange=function(){
    if(this.readyState!==4) return;
    if(this.status!==200){
      window.alert(this.responseText||"读取文件列表失败");
      return;
    }
    var obj;
    try{
      obj=JSON.parse(this.responseText);
    }catch(error){
      window.alert("文件列表数据无效");
      return;
    }
    document.getElementById("disk_percentage_text").textContent=obj.used;
    var percentage=Math.max(0,Math.min(100,Number(obj.percentage)||0));
    document.getElementById("disk_percentage").style.width=percentage+"%";
    var table=document.getElementById("file_list");
    table.textContent="";
    var files=Array.isArray(obj.file_list)?obj.file_list:[];
    for(var i=0;i<files.length;i++){
      var row=document.createElement("tr");
      var nameCell=document.createElement("td");
      var sizeCell=document.createElement("td");
      var actionCell=document.createElement("td");
      nameCell.style.textAlign="left";
      actionCell.style.textAlign="right";
      nameCell.textContent=files[i].name;
      sizeCell.textContent=files[i].size;
      var normalized=files[i].name.charAt(0)==="/"?files[i].name:"/"+files[i].name;
      if(normalized==="/pic.xbm"){
        actionCell.textContent="删除";
        (function(name){
          actionCell.onclick=function(){delete_file(name);};
        })(files[i].name);
      }else{
        actionCell.textContent="受保护";
      }
      row.appendChild(nameCell);
      row.appendChild(sizeCell);
      row.appendChild(actionCell);
      table.appendChild(row);
    }
  };
  xhttp.open("GET","/get_file_list",true);
  xhttp.send();
}
function delete_file(filename){
  var normalized=filename.charAt(0)==="/"?filename:"/"+filename;
  if(normalized!=="/pic.xbm"){
    window.alert("配置和字体属于系统文件，仅允许删除用户图片 pic.xbm");
    return;
  }
  if(!window.confirm("确定要删除文件 "+filename+" 吗？")) return;
  var xhttp=new XMLHttpRequest();
  xhttp.onreadystatechange=function(){
    if(this.readyState!==4) return;
    if(this.status===200) get_file_list();
    else window.alert(this.responseText||"删除失败");
  };
  xhttp.open("GET","/delete_file?filename="+encodeURIComponent(filename),true);
  xhttp.send();
}
)JS";

/**
 * strings_en.h
 * engligh strings for
 * WiFiManager, a library for the ESP8266/Arduino platform
 * for configuration of WiFi credentials using a Captive Portal
 * 
 * @author Creator tzapu
 * @author tablatronix
 * @version 0.0.0
 * @license MIT
 */

#ifndef WIFI_MANAGER_OVERRIDE_STRINGS
// !!! THIS DOES NOT WORK, you cannot define in a sketch, if anyone one knows how to order includes to be able to do this help!

const char HTTP_HEAD_START[]       PROGMEM = "<!DOCTYPE html><html lang='en'><head><meta name='format-detection' content='telephone=no'><meta charset='UTF-8'><meta  name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'/><title>{v}</title>";
const char HTTP_SCRIPT[]           PROGMEM = "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>";
const char HTTP_HEAD_END[]         PROGMEM = "</head><body class='{c}'><div class='wrap'>";

const char HTTP_ROOT_MAIN[]        PROGMEM = "<h1>{v}</h1><h3>WiFiManager</h3>";
const char * const HTTP_PORTAL_MENU[] PROGMEM = {
"<form action='/wifi'    method='get'><button>Configure WiFi</button></form><br/>\n", // MENU_WIFI
"<form action='/0wifi'   method='get'><button>Configure WiFi (No Scan)</button></form><br/>\n", // MENU_WIFINOSCAN
"<form action='/info'    method='get'><button>Info</button></form><br/>\n", // MENU_INFO
"<form action='/param'   method='get'><button>Setup</button></form><br/>\n",//MENU_PARAM
"<form action='/close'   method='get'><button>Close</button></form><br/>\n", // MENU_CLOSE
"<form action='/restart' method='get'><button>Restart</button></form><br/>\n",// MENU_RESTART
"<form action='/exit'    method='get'><button>Exit</button></form><br/>\n",  // MENU_EXIT
"<form action='/erase'   method='get'><button class='D'>Erase</button></form><br/>\n", // MENU_ERASE
"<hr><br/>" // MENU_SEP
};
const char HTTP_PORTAL_MENU_CUSTOM[] PROGMEM = {
"<form action='/ccs811'    method='get'><button>CCS 811 baseline</button></form><br/>\n"
};
// const char HTTP_PORTAL_OPTIONS[]   PROGMEM = strcat(HTTP_PORTAL_MENU[0] , HTTP_PORTAL_MENU[3] , HTTP_PORTAL_MENU[7]);
const char HTTP_PORTAL_OPTIONS[]   PROGMEM = "";
const char HTTP_ITEM_QI[]          PROGMEM = "<div role='img' aria-label='{r}%' title='{r}%' class='q q-{q} {i} {h}'></div>"; // rssi icons
const char HTTP_ITEM_QP[]          PROGMEM = "<div class='q {h}'>{r}%</div>"; // rssi percentage
const char HTTP_ITEM[]             PROGMEM = "<div><a href='#p' onclick='c(this)'>{v}</a>{qi}{qp}</div>"; // {q} = HTTP_ITEM_QI, {r} = HTTP_ITEM_QP
// const char HTTP_ITEM[]            PROGMEM = "<div><a href='#p' onclick='c(this)'>{v}</a> {R} {r}% {q} {e}</div>"; // test all tokens

const char HTTP_FORM_START[]       PROGMEM = "<form method='POST' action='{v}'>";
const char HTTP_FORM_WIFI[]        PROGMEM = "<label for='s'>SSID</label><input id='s' name='s' maxlength='32' autocorrect='off' autocapitalize='none' placeholder='{v}'><br/><label for='p'>Password</label><input id='p' name='p' maxlength='64' type='password' placeholder=''>";
const char HTTP_FORM_WIFI_END[]    PROGMEM = "";
const char HTTP_FORM_STATIC_HEAD[] PROGMEM = "<hr><br/>";
const char HTTP_FORM_END[]         PROGMEM = "<br/><br/><button type='submit'>Save</button></form>";
const char HTTP_FORM_LABEL[]       PROGMEM = "<label for='{i}'>{t}</label>";
const char HTTP_FORM_PARAM_HEAD[]  PROGMEM = "<hr><br/>";
const char HTTP_FORM_PARAM[]       PROGMEM = "<br/><input id='{i}' name='{n}' maxlength='{l}' value='{v}' {c}>";

const char HTTP_SCAN_LINK[]        PROGMEM = "<br/><form action='/wifi?refresh=1' method='POST'><button name='refresh' value='1'>Refresh</button></form>";
const char HTTP_SAVED[]            PROGMEM = "<div class='msg'>Saving Credentials<br/>Trying to connect ESP to network.<br />If it fails reconnect to AP to try again</div>";
const char HTTP_PARAMSAVED[]       PROGMEM = "<div class='msg'>Saved<br/></div>";
const char HTTP_END[]              PROGMEM = "</div></body></html>";
const char HTTP_ERASEBTN[]         PROGMEM = "<br/><form action='/erase' method='get'><button class='D'>Erase WiFi Config</button></form>";

const char HTTP_STATUS_ON[]        PROGMEM = "<div class='msg P'><strong>Connected</strong> to {v}<br/><em><small>with IP {i}</small></em></div>";
const char HTTP_STATUS_OFF[]       PROGMEM = "<div class='msg {c}'><strong>Not Connected</strong> to {v}{r}</div>";
const char HTTP_STATUS_OFFPW[]     PROGMEM = "<br/>Authentication Failure"; // STATION_WRONG_PASSWORD,  no eps32
const char HTTP_STATUS_OFFNOAP[]   PROGMEM = "<br/>AP not found";   // WL_NO_SSID_AVAIL
const char HTTP_STATUS_OFFFAIL[]   PROGMEM = "<br/>Could not Connect"; // WL_CONNECT_FAILED
const char HTTP_STATUS_NONE[]      PROGMEM = "<div class='msg'>No AP set</div>";
const char HTTP_BR[]               PROGMEM = "<br/>";

const char HTTP_STYLE[]            PROGMEM = "<style>"
".c,body{text-align:center;font-family:verdana}div,input{padding:5px;font-size:1em;margin:5px 0;box-sizing:border-box;}"
"input,button,.msg{border-radius:.3rem;width: 100%}"
"button,input[type='button'],input[type='submit']{cursor:pointer;border:0;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%}"
"input[type='file']{border:1px solid #1fa3ec}"
".wrap {text-align:left;display:inline-block;min-width:260px;max-width:500px}"
// links
"a{color:#000;font-weight:700;text-decoration:none}a:hover{color:#1fa3ec;text-decoration:underline}"
// quality icons
".q{height:16px;margin:0;padding:0 5px;text-align:right;min-width:38px;float:right}.q.q-0:after{background-position-x:0}.q.q-1:after{background-position-x:-16px}.q.q-2:after{background-position-x:-32px}.q.q-3:after{background-position-x:-48px}.q.q-4:after{background-position-x:-64px}.q.l:before{background-position-x:-80px;padding-right:5px}.ql .q{float:left}.q:after,.q:before{content:'';width:16px;height:16px;display:inline-block;background-repeat:no-repeat;background-position: 16px 0;"
"background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAGAAAAAQCAMAAADeZIrLAAAAJFBMVEX///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADHJj5lAAAAC3RSTlMAIjN3iJmqu8zd7vF8pzcAAABsSURBVHja7Y1BCsAwCASNSVo3/v+/BUEiXnIoXkoX5jAQMxTHzK9cVSnvDxwD8bFx8PhZ9q8FmghXBhqA1faxk92PsxvRc2CCCFdhQCbRkLoAQ3q/wWUBqG35ZxtVzW4Ed6LngPyBU2CobdIDQ5oPWI5nCUwAAAAASUVORK5CYII=');}"
// icons @2x media query (32px rescaled)
"@media (-webkit-min-device-pixel-ratio: 2),(min-resolution: 192dpi){.q:before,.q:after {"
"background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAALwAAAAgCAMAAACfM+KhAAAALVBMVEX///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADAOrOgAAAADnRSTlMAESIzRGZ3iJmqu8zd7gKjCLQAAACmSURBVHgB7dDBCoMwEEXRmKlVY3L//3NLhyzqIqSUggy8uxnhCR5Mo8xLt+14aZ7wwgsvvPA/ofv9+44334UXXngvb6XsFhO/VoC2RsSv9J7x8BnYLW+AjT56ud/uePMdb7IP8Bsc/e7h8Cfk912ghsNXWPpDC4hvN+D1560A1QPORyh84VKLjjdvfPFm++i9EWq0348XXnjhhT+4dIbCW+WjZim9AKk4UZMnnCEuAAAAAElFTkSuQmCC');"
"background-size: 95px 16px;}}"
// msg callouts
".msg{padding:20px;margin:20px 0;border:1px solid #eee;border-left-width:5px;border-left-color:#777}.msg h4{margin-top:0;margin-bottom:5px}.msg.P{border-left-color:#1fa3ec}.msg.P h4{color:#1fa3ec}.msg.D{border-left-color:#dc3630}.msg.D h4{color:#dc3630}"
// lists
"dt{font-weight:bold}dd{margin:0;padding:0 0 0.5em 0;min-height:12px}"
"td{vertical-align: top;}"
".h{display:none}"
"button.D{background-color:#dc3630}"
// invert
"body.invert,body.invert a,body.invert h1 {background-color:#060606;color:#fff;}"
"body.invert .msg{color:#fff;background-color:#282828;border-top:1px solid #555;border-right:1px solid #555;border-bottom:1px solid #555;}"
"body.invert .q[role=img]{-webkit-filter:invert(1);filter:invert(1);}"
"</style>";

const char HTTP_HELP[]             PROGMEM =
 "<br/><h3>Available Pages</h3><hr>"
 "<table class='table'>"
 "<thead><tr><th>Page</th><th>Function</th></tr></thead><tbody>"
 "<tr><td><a href='/'>/</a></td>"
 "<td>Menu page.</td></tr>"
 "<tr><td><a href='/wifi'>/wifi</a></td>"
 "<td>Show WiFi scan results and enter WiFi configuration.(/0wifi noscan)</td></tr>"
 "<tr><td><a href='/wifisave'>/wifisave</a></td>"
 "<td>Save WiFi configuration information and configure device. Needs variables supplied.</td></tr>"
 "<tr><td><a href='/param'>/param</a></td>"
 "<td>Parameter page</td></tr>"
 "<tr><td><a href='/info'>/info</a></td>"
 "<td>Information page</td></tr>"
 "<tr><td><a href='/close'>/close</a></td>"
 "<td>Close the captiveportal popup,configportal will remain active</td></tr>"
 "<tr><td><a href='/exit'>/exit</a></td>"
 "<td>Exit Config Portal, configportal will close</td></tr>"
 "<tr><td><a href='/restart'>/restart</a></td>"
 "<td>Reboot the device</td></tr>"
 "<tr><td><a href='/erase'>/erase</a></td>"
 "<td>Erase WiFi configuration and reboot Device. Device will not reconnect to a network until new WiFi configuration data is entered.</td></tr>"
 "</table>"
 "<p/>More information about WiFiManager at <a href='https://github.com/tzapu/WiFiManager'>https://github.com/tzapu/WiFiManager</a>.";

#ifdef JSTEST
const char HTTP_JS[] PROGMEM = 
"<script>function postAjax(url, data, success) {"
"    var params = typeof data == 'string' ? data : Object.keys(data).map("
"            function(k){ return encodeURIComponent(k) + '=' + encodeURIComponent(data[k]) }"
"        ).join('&');"
"    var xhr = window.XMLHttpRequest ? new XMLHttpRequest() : new ActiveXObject(\"Microsoft.XMLHTTP\");"
"    xhr.open('POST', url);"
"    xhr.onreadystatechange = function() {"
"        if (xhr.readyState>3 && xhr.status==200) { success(xhr.responseText); }"
"    };"
"    xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest');"
"    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');"
"    xhr.send(params);"
"    return xhr;}"
"postAjax('/status', 'p1=1&p2=Hello+World', function(data){ console.log(data); });"
"postAjax('/status', { p1: 1, p2: 'Hello World' }, function(data){ console.log(data); });"
"</script>";
#endif

// Info html
#ifdef ESP32
  const char HTTP_INFO_esphead[]    PROGMEM = "<h3>esp32</h3><hr><dl>";
  const char HTTP_INFO_chiprev[]    PROGMEM = "<dt>Chip Rev</dt><dd>{1}</dd>";
  const char HTTP_INFO_lastreset[]    PROGMEM = "<dt>Last reset reason</dt><dd>CPU0: {1}<br/>CPU1: {2}</dd>";
  const char HTTP_INFO_aphost[]       PROGMEM = "<dt>Acccess Point Hostname</dt><dd>{1}</dd>";  
#else 
  const char HTTP_INFO_esphead[]    PROGMEM = "<h3>esp8266</h3><hr><dl>";
  const char HTTP_INFO_flashsize[]  PROGMEM = "<dt>Real Flash Size</dt><dd>{1} bytes</dd>";
  const char HTTP_INFO_fchipid[]    PROGMEM = "<dt>Flash Chip ID</dt><dd>{1}</dd>";
  const char HTTP_INFO_corever[]    PROGMEM = "<dt>Core Version</dt><dd>{1}</dd>";
  const char HTTP_INFO_bootver[]    PROGMEM = "<dt>Boot Version</dt><dd>{1}</dd>";
  const char HTTP_INFO_memsketch[]  PROGMEM = "<dt>Memory - Sketch Size</dt><dd>Used / Total bytes<br/>{1} / {2}";
  const char HTTP_INFO_memsmeter[]  PROGMEM = "<br/><progress value='{1}' max='{2}'></progress></dd>";
  const char HTTP_INFO_lastreset[]  PROGMEM = "<dt>Last reset reason</dt><dd>{1}</dd>";
#endif

const char HTTP_INFO_freeheap[]   PROGMEM = "<dt>Memory - Free Heap</dt><dd>{1} bytes available</dd>"; 
const char HTTP_INFO_wifihead[]   PROGMEM = "<br/><h3>WiFi</h3><hr>";
const char HTTP_INFO_uptime[]     PROGMEM = "<dt>Uptime</dt><dd>{1} Mins {2} Secs</dd>";
const char HTTP_INFO_chipid[]     PROGMEM = "<dt>Chip ID</dt><dd>{1}</dd>";
const char HTTP_INFO_idesize[]    PROGMEM = "<dt>Flash Size</dt><dd>{1} bytes</dd>";
const char HTTP_INFO_sdkver[]     PROGMEM = "<dt>SDK Version</dt><dd>{1}</dd>";
const char HTTP_INFO_cpufreq[]    PROGMEM = "<dt>CPU Frequency</dt><dd>{1}MHz</dd>";
const char HTTP_INFO_apip[]       PROGMEM = "<dt>Access Point IP</dt><dd>{1}</dd>";
const char HTTP_INFO_apmac[]      PROGMEM = "<dt>Access Point MAC</dt><dd>{1}</dd>";
const char HTTP_INFO_apssid[]     PROGMEM = "<dt>SSID</dt><dd>{1}</dd>";
const char HTTP_INFO_apbssid[]    PROGMEM = "<dt>BSSID</dt><dd>{1}</dd>";
const char HTTP_INFO_staip[]      PROGMEM = "<dt>Station IP</dt><dd>{1}</dd>";
const char HTTP_INFO_stagw[]      PROGMEM = "<dt>Station Gateway</dt><dd>{1}</dd>";
const char HTTP_INFO_stasub[]     PROGMEM = "<dt>Station Subnet</dt><dd>{1}</dd>";
const char HTTP_INFO_dnss[]       PROGMEM = "<dt>DNS Server</dt><dd>{1}</dd>";
const char HTTP_INFO_host[]       PROGMEM = "<dt>Hostname</dt><dd>{1}</dd>";
const char HTTP_INFO_stamac[]     PROGMEM = "<dt>Station MAC</dt><dd>{1}</dd>";
const char HTTP_INFO_conx[]       PROGMEM = "<dt>Connected</dt><dd>{1}</dd>";
const char HTTP_INFO_autoconx[]   PROGMEM = "<dt>Autoconnect</dt><dd>{1}</dd>";
const char HTTP_INFO_temp[]       PROGMEM = "<dt>Temperature</dt><dd>{1} C&deg; / {2} F&deg;</dd>";

// Strings
const char S_y[]                  PROGMEM = "Yes";
const char S_n[]                  PROGMEM = "No";
const char S_enable[]             PROGMEM = "Enabled";
const char S_disable[]            PROGMEM = "Disabled";
const char S_GET[]                PROGMEM = "GET";
const char S_POST[]               PROGMEM = "POST";
const char S_NA[]                 PROGMEM = "Unknown";

const char S_titlewifisaved[]     PROGMEM = "Credentials Saved";
const char S_titlewifi[]          PROGMEM = "Config ESP";
const char S_titleinfo[]          PROGMEM = "Info";
const char S_titleparam[]         PROGMEM = "Setup";
const char S_titleparamsaved[]    PROGMEM = "Setup Saved";
const char S_titleexit[]          PROGMEM = "Exit";
const char S_titlereset[]         PROGMEM = "Reset";
const char S_titleerase[]         PROGMEM = "Erase";
const char S_titleclose[]         PROGMEM = "Close";
const char S_options[]            PROGMEM = "options";
const char S_nonetworks[]         PROGMEM = "No networks found. Refresh to scan again.";
const char S_staticip[]           PROGMEM = "Static IP";
const char S_staticgw[]           PROGMEM = "Static Gateway";
const char S_staticdns[]          PROGMEM = "Static DNS";
const char S_subnet[]             PROGMEM = "Subnet";
const char S_exiting[]            PROGMEM = "Exiting";
const char S_resetting[]          PROGMEM = "Module will reset in a few seconds.";
const char S_closing[]            PROGMEM = "You can close the page, portal will continue to run";
const char S_error[]              PROGMEM = "An Error Occured";
const char S_notfound[]           PROGMEM = "File Not Found\n\n";
const char S_uri[]                PROGMEM = "URI: ";
const char S_method[]             PROGMEM = "\nMethod: ";
const char S_args[]               PROGMEM = "\nArguments: ";
const char S_parampre[]           PROGMEM = "param_";

// debug strings
const char D_HR[]                 PROGMEM = "--------------------";

// END WIFI_MANAGER_OVERRIDE_STRINGS
#endif

// -----------------------------------------------------------------------------------------------
// DO NOT EDIT BELOW THIS LINE

const uint8_t _nummenutokens = 9;
const char * const _menutokens[9] PROGMEM = {
    "wifi",
    "wifinoscan",
    "info",
    "param",
    "close",
    "restart",
    "exit",
    "erase",
    "sep"
};

const char R_root[]               PROGMEM = "/";
const char R_wifi[]               PROGMEM = "/wifi";
const char R_wifinoscan[]         PROGMEM = "/0wifi";
const char R_wifisave[]           PROGMEM = "/wifisave";
const char R_info[]               PROGMEM = "/info";
const char R_param[]              PROGMEM = "/param";
const char R_paramsave[]          PROGMEM = "/paramsave";
const char R_restart[]            PROGMEM = "/restart";
const char R_exit[]               PROGMEM = "/exit";
const char R_close[]              PROGMEM = "/close";
const char R_erase[]              PROGMEM = "/erase"; 
const char R_status[]             PROGMEM = "/status";


//Strings
const char S_ip[]                 PROGMEM = "ip";
const char S_gw[]                 PROGMEM = "gw";
const char S_sn[]                 PROGMEM = "sn";
const char S_dns[]                PROGMEM = "dns";

// softap ssid default prefix
#ifdef ESP8266
  const char S_ssidpre[]        PROGMEM = "ESP";
#elif defined(ESP32)
  const char S_ssidpre[]        PROGMEM = "ESP32";
#else
  const char S_ssidpre[]        PROGMEM = "WM";
#endif

//Tokens
//@todo consolidate and reduce
const char T_ss[]                 PROGMEM = "{"; // token start sentinel
const char T_es[]                 PROGMEM = "}"; // token end sentinel
const char T_1[]                  PROGMEM = "{1}"; // @token 1
const char T_2[]                  PROGMEM = "{2}"; // @token 2
const char T_v[]                  PROGMEM = "{v}"; // @token v
const char T_I[]                  PROGMEM = "{I}"; // @token I
const char T_i[]                  PROGMEM = "{i}"; // @token i
const char T_n[]                  PROGMEM = "{n}"; // @token n
const char T_p[]                  PROGMEM = "{p}"; // @token p
const char T_t[]                  PROGMEM = "{t}"; // @token t
const char T_l[]                  PROGMEM = "{l}"; // @token l
const char T_c[]                  PROGMEM = "{c}"; // @token c
const char T_e[]                  PROGMEM = "{e}"; // @token e
const char T_q[]                  PROGMEM = "{q}"; // @token q
const char T_r[]                  PROGMEM = "{r}"; // @token r
const char T_R[]                  PROGMEM = "{R}"; // @token R
const char T_h[]                  PROGMEM = "{h}"; // @token h

// http
const char HTTP_HEAD_CL[]         PROGMEM = "Content-Length";
const char HTTP_HEAD_CT[]         PROGMEM = "text/html";
const char HTTP_HEAD_CT2[]        PROGMEM = "text/plain";
const char HTTP_HEAD_CORS[]       PROGMEM = "Access-Control-Allow-Origin";
const char HTTP_HEAD_CORS_ALLOW_ALL[]  PROGMEM = "*";

const char * const WIFI_STA_STATUS[] PROGMEM
{
  "WL_IDLE_STATUS",     // 0 STATION_IDLE
  "WL_NO_SSID_AVAIL",   // 1 STATION_NO_AP_FOUND
  "WL_SCAN_COMPLETED",  // 2
  "WL_CONNECTED",       // 3 STATION_GOT_IP
  "WL_CONNECT_FAILED",  // 4 STATION_CONNECT_FAIL, STATION_WRONG_PASSWORD(NI)
  "WL_CONNECTION_LOST", // 5
  "WL_DISCONNECTED",    // 6 
  "WL_STATION_WRONG_PASSWORD" // 7 KLUDGE 
};

#ifdef ESP32
const char * const AUTH_MODE_NAMES[] PROGMEM
{
    "OPEN",
    "WEP",             
    "WPA_PSK",         
    "WPA2_PSK",        
    "WPA_WPA2_PSK",    
    "WPA2_ENTERPRISE", 
    "MAX"
};
#elif defined(ESP8266)
const char * const AUTH_MODE_NAMES[] PROGMEM
{
    "",
    "",
    "WPA_PSK",      // 2 ENC_TYPE_TKIP
    "",
    "WPA2_PSK",     // 4 ENC_TYPE_CCMP
    "WEP",          // 5 ENC_TYPE_WEP
    "",
    "OPEN",         //7 ENC_TYPE_NONE
    "WPA_WPA2_PSK", // 8 ENC_TYPE_AUTO
};
#endif

const char* const WIFI_MODES[] PROGMEM = { "NULL", "STA", "AP", "STA+AP" };

#ifdef ESP32
const wifi_country_t WM_COUNTRY_US{"US",1,11,WIFI_COUNTRY_POLICY_AUTO};
const wifi_country_t WM_COUNTRY_CN{"CN",1,13,WIFI_COUNTRY_POLICY_AUTO};
const wifi_country_t WM_COUNTRY_JP{"JP",1,14,WIFI_COUNTRY_POLICY_AUTO};
#endif
