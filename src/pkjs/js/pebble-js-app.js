var initialised = false;
var CityID = 0, posLat = "0", posLon = "0", lang = "en";
var calendarPending = false , weatherPending = false, messageData = {};
var weatherIcon = {
    "01d" : 'I',	//clear sky (day)
    "02d" : '"',	//few clouds (day)
    "03d" : '!',	//scattered clouds
    "04d" : 'k',	//broken clouds
    "09d" : '$',	//shower rain
    "10d" : '+',	//rain (day)
    "11d" : 'F',	//thunderstorm
    "13d" : '9',	//snow
    "50d" : '=',	//mist (day)
    "01n" : 'N',	//clear sky (night)
    "02n" : '#',	//few clouds (night)
    "03n" : '!',	//scattered clouds
    "04n" : 'k',	//broken clouds
    "09n" : '$',	//shower rain
    "10n" : ',',	//rain (night)
    "11n" : 'F',	//thunderstorm
    "13n" : '9',	//snow
    "50n" : '>'		//mist (night)
};

//-----------------------------------------------------------------------------------------------------------------------
Pebble.addEventListener("ready", function() {
    initialised = true;
	var p_lang = "en_US";

	//Get pebble language
	if(Pebble.getActiveWatchInfo) {
		try {
			var watch = Pebble.getActiveWatchInfo();
			p_lang = watch.language;
		} catch(err) {
			console.log("Pebble.getActiveWatchInfo(); Error!");
		}
	} 

	//Choose language
	var sub = p_lang.substring(0, 2);
	if (sub === "de")
		lang = "de";
	else  if (sub === "es")
		lang = "es";
	else if (sub === "fr")
		lang = "fr";
	else if (sub === "it")
		lang = "it";
	else
		lang = "en";

	console.log("JavaScript app ready and running! Pebble lang: " + p_lang + ", using for Weather: " + lang);
	sendMessageToPebble({"JS_READY": 1});		
});
//-----------------------------------------------------------------------------------------------------------------------
function sendMessageToPebble(payload) {
	Pebble.sendAppMessage(payload, 
		function(e) {
			console.log('Successfully delivered message (' + e.payload + ') with transactionId='+ e.data.transactionId);
		},
		function(e) {
			console.log('Unable to deliver message with transactionId=' + e.data.transactionId + ' Error is: ' + e.error.message);
		});
}
//-----------------------------------------------------------------------------------------------------------------------
//-- Get current location: http://forums.getpebble.com/discussion/21755/pebble-js-location-to-url
var locationOptions = {
	enableHighAccuracy: true, 
	maximumAge: 10000, 
	timeout: 10000
};
//-----------------------------------------------------------------------------------------------------------------------
function locationSuccess(pos) {
	console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);
	posLat = (pos.coords.latitude).toFixed(3);
	posLon = (pos.coords.longitude).toFixed(3);
	
	updateWeather();
}
//-----------------------------------------------------------------------------------------------------------------------
function locationError(err) {
	posLat = "0";
	posLon = "0";
	console.log('location error (' + err.code + '): ' + err.message);
}
//-----------------------------------------------------------------------------------------------------------------------
Pebble.addEventListener('appmessage', function(e) {
	console.log("Got message: " + JSON.stringify(e));
	if ('cityid' in e.payload) {	//Weather Download
		CityID = e.payload.cityid;
		if (CityID === 0)
			navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
		else
			updateWeather();
	}else if ('calendar' in e.payload){
    updateCalendar();  
  }
});
//-----------------------------------------------------------------------------------------------------------------------
function checkSendMessage(){
  console.log("checkSendMessage");
  if(!calendarPending && !weatherPending){
    console.log("sendMessageToPebble");
     sendMessageToPebble(messageData);   
  }
}
//-----------------------------------------------------------------------------------------------------------------------
function updateWeather() {  
	console.log("Updating weather");
  updateCalendar(); 
	var req = new XMLHttpRequest();
	var URL = "http://api.openweathermap.org/data/2.5/weather?APPID=a9423f401fa1a50cbef532db45360841&";
	
	if (CityID !== 0)
		URL += "id="+CityID.toString();
	else if (posLat != "0" && posLon != "0")
		URL += "lat=" + posLat + "&lon=" + posLon;
	else
		return; //Error, no position data
	
	URL += "&units=metric&lang=" + lang + "&type=accurate";
	console.log("UpdateURL: " + URL);
  weatherPending = true;
	req.open("GET", URL, true);
	req.onload = function(e) {
		if (req.readyState == 4) {
      weatherPending = false;
			if (req.status == 200) {
				var response = JSON.parse(req.responseText);
				var temp = Math.round(response.main.temp);//-273.15
				var icon = response.weather[0].icon;
				var cond = response.weather[0].description;
				var name = response.name;
        console.log("Got Weather Data for City: " + name + ", Temp: " + temp + ", Icon:" + icon + weatherIcon[icon]+", Cond:"+cond);
        messageData["w_temp"]= temp;
        messageData["w_icon"]= weatherIcon[icon];
        messageData["w_cond"]= cond;
        checkSendMessage();				
			}
		}
	};
	req.send(null);
}
//-----------------------------------------------------------------------------------------------------------------------
function updateCalendar() {
	console.log("Updating calendar");
	var req = new XMLHttpRequest();
	var URL = "https://calout.azurewebsites.net/calendar";
  calendarPending = true;
	req.open("GET", URL, true);
  req.responseType = 'json';
	req.onload = function(e) {
		if (req.readyState == 4) {
      calendarPending = false;
			if (req.status == 200) {
				var calendarResponse = req.response;
				var hours = ''+calendarResponse.hours;
        var subject = ''+calendarResponse.subject;
        console.log("Got Calendar Data: " + subject + ", Due hours: " + hours);
        messageData["c_subj"]= subject;
        checkSendMessage();				
			}
		}
	};
	req.send(null);
}
//-----------------------------------------------------------------------------------------------------------------------
Pebble.addEventListener("showConfiguration", function() {
    var options = JSON.parse(localStorage.getItem('cas_wv_28de_opt'));
    console.log("read options: " + JSON.stringify(options));
    console.log("showing configuration");
	var uri = 'https://pfxmobile.github.io/config_casiowv58de.html?title=Casio%20WV-58DE%20v2.10';
    if (options !== null) {
        uri +=
			'&inv=' + encodeURIComponent(options.inv) + 
			'&showsec=' + encodeURIComponent(options.showsec) + 
			'&battdgt=' + encodeURIComponent(options.battdgt) + 
			'&showbatt=' + encodeURIComponent(options.showbatt) + 
			'&vibr=' + encodeURIComponent(options.vibr) + 
			'&vibr_bt=' + encodeURIComponent(options.vibr_bt) + 
			'&datefmt=' + encodeURIComponent(options.datefmt) + 
			'&weather=' + encodeURIComponent(options.weather) + 
			'&showcond=' + encodeURIComponent(options.showcond) + 
			'&units=' + encodeURIComponent(options.units) + 
			'&cityid=' + encodeURIComponent(options.cityid);
    }
	console.log("Uri: "+uri);
    Pebble.openURL(uri);
});
//-----------------------------------------------------------------------------------------------------------------------
Pebble.addEventListener("webviewclosed", function(e) {
    console.log("configuration closed");
    if (e.response !== "") {
        var options = JSON.parse(decodeURIComponent(e.response));
        console.log("storing options: " + JSON.stringify(options));
        localStorage.setItem('cas_wv_28de_opt', JSON.stringify(options));
        sendMessageToPebble(options);
    } else {
        console.log("no options received");
    }
});
//-----------------------------------------------------------------------------------------------------------------------