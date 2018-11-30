var level = require('level')

// create database
var db = level('./sensordb', {valueEncoding: 'json'})

// get command line arguments
const args = require('minimist')(process.argv.slice(2))

var found = false
db.createReadStream()
.on('data', function (data) {
  var key = data.key
  var value = data.value[0]
  var temp = value.temp
  var smoke = value.smoke
  var id = value.id
  var time = value.time
  if(args['t'] == temp && args['s'] == smoke) {
    console.log('Sensor ' + id + ': ')
    if(smoke) console.log('With smoke')
    else console.log('No smoke')
    console.log('Temperature: ' + temp + ' C')
    found = true
  }
})
.on('error', function (err) {
  console.log('Oh my!', err)
})
.on('close', function () {
  // console.log('Stream closed')
})
.on('end', function () {
  if(!found) console.log('No such data could be found')
})
