var http = require('http');
var events = require('events');
var emitter = new events.EventEmitter();

var handler = function() {
  console.log("Loaded!");
}

emitter.on('fire', handler);

http.createServer(function(req, res) {
  res.writeHead(200, {'Content-Type': 'text/html'});
  emitter.emit('fire');
  res.end();
}).listen(8080);
