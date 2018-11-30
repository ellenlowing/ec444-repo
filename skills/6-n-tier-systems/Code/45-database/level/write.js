var level = require('level')
var fs = require('fs')

// create database
var db = level('./sensordb', {valueEncoding: 'json'})

// read text file
var readEnd = false
fs.readFile('smoke.txt', 'utf8', function (err, file) {
  if(err) throw err;
  var rows = file.split(/\r?\n|\r/)
  for(var row = 1; row < rows.length; row++) {
    var data = rows[row].split('\t')
    var time = data[0]
    var id = data[1]
    var smoke = data[2]
    var temp = data[3]
    var key = row
    var value = [{temp: temp, id: id, smoke: smoke, time: time}]
    
    db.put(key, value, function (err) {
      if(err) return console.log('Oooops', err)
    })
  }

})



// 2) Put a key & value
// db.put('name', 'Level', function (err) {
//   if (err) return console.log('Ooops!', err) // some kind of I/O error
//
//   // 3) Fetch by key
//   db.get('name', function (err, value) {
//     if (err) return console.log('Ooops!', err) // likely the key was not found
//
//     // Ta da!
//     console.log('name=' + value)
//   })
// })
