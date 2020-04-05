const express = require('express');
const fs = require('fs');
const path = require('path');
const config = require(path.join(__dirname,'config.json'));

function returnImage(image_dir, image_name){
  const image_path = path.join(image_dir, image_name);
  return (req, res) => {
    console.debug(`image: ${image_name}`);
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

module.exports = function(){
  const app = express();
  app.use(express.static(path.join(__dirname, 'public')));
  app.use(express.static(path.resolve(__dirname+'/../images')));

  const image_dir = path.resolve(path.join(__dirname, config.image_dir));
  app.get('/image/stream', returnImage(image_dir, 'stream.jpeg'));
  app.get('/image/thermal', returnImage(image_dir, 'thermal.jpeg'));
  app.get('/image/visible', returnImage(image_dir, 'visible.jpeg'));

  app.get('/', function(req, res) {
    res.sendFile(path.join(__dirname + '/index.html'));
    console.log('index');
  })

  app.listen(config.web_server.network.port, function () {
    console.log(`Listening on port ${config.web_server.network.port}!`);
  });
}
