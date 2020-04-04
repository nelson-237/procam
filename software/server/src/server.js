const express = require('express');
const fs = require('fs');
const path = require('path');
const app = express();
const config = require(path.join(__dirname,'config.json'));


app.use(express.static(path.join(__dirname, 'public')));
app.use(express.static(path.resolve(__dirname+'/../images')));

function returnImage(image_path){
  return (req, res) => {
    console.debug(`image: ${path.basename(image_path)}`);
    if (! fs.existsSync(image_path)) {
      res.status(403).end('Forbidden');
    }
    var type = 'image/jpeg'
    var s = fs.createReadStream(image_path);
    s.on('open', function () {
        res.set('Content-Type', type);
        s.pipe(res);
    });
    s.on('error', function () {
        res.set('Content-Type', 'text/plain');
        res.status(404).end('Not found');
    });
  };
}

const image_dir = path.resolve(__dirname+'/../images/');
const thermal_image_path = path.resolve(image_dir+'/thermal.jpeg');
app.get('/image/thermal', returnImage(thermal_image_path));
const visible_image_path = path.resolve(image_dir+'/visible.jpeg');
app.get('/image/visible', returnImage(visible_image_path));

app.get('/', function(req, res) {
  res.sendFile(path.join(__dirname + '/index.html'));
  console.log('index');
})

app.listen(config.network.port, function () {
  console.log(`Listening on port ${config.network.port}!`);
});