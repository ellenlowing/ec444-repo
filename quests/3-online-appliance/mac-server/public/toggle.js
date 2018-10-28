var buttonState = false;
var button = document.getElementById('button');
var url = 'http://smartsystems.ddns.net:1111/';

// lights on/off? --- buttonState --- textToDisplay
// on             --- true        --- OFF
// off            --- false       --- ON


button.addEventListener('click', (e) => {
  if(buttonState) {
    buttonState = false;
    button.innerHTML = "ON";
  } else {
    buttonState = true;
    button.innerHTML = "OFF";
  }
  var xhr = new XMLHttpRequest();
  xhr.open("POST", url, true);
  xhr.setRequestHeader('Content-Type', 'application/json');
  xhr.send(JSON.stringify({
      buttonState: buttonState
  }));
});
