var express = require('express');
var bodyParser = require('body-parser');
var app = express();
var http = require('http');

var buttonState = false;

// serve files from the public directory
app.use(express.static('public'));

// configure body-parser for express
app.use(bodyParser.urlencoded({extended: false}));
app.use(bodyParser.json());

// serve the homepage
app.get('/', (req, res) => {
  res.sendFile(__dirname + '/index.html');
});

// handle POST request
app.post('/', (req, res) => {
  response = {
    buttonState: req.body.buttonState
  };
  buttonState = response.buttonState;
  res.end();
});

app.get('/ctrl', (req, res) => {
  if(buttonState) res.end("1")
  else res.end("0");
});

// start the express web server listening on 1111
app.listen(1111, () => {
  console.log('listening on 1111');
});
