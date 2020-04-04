const express = require('express');
const fs = require('fs');
const path = require('path');
const app = express();
const config = require(path.join(__dirname,'config.json'));


app.use(express.static(path.join(__dirname, '/public')));
app.use(express.static(path.join(__dirname, '/../images')));

app.get('/', function(req, res) {
  res.sendFile(path.join(__dirname + '/index.html'));
  console.log('page');
})

app.listen(config.network.port, function () {
  console.log(`Listening on port ${config.network.port}!`);
});