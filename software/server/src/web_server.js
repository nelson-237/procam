const express = require('express');
const fs = require('fs');
const path = require('path');
const config = require(path.join(__dirname,'config.json'));
const busboy = require('connect-busboy');

function returnImage(queue){
  return (req, res) => {
    const image_path = queue[queue.length -1];
    // console.debug(`image: ${image_path}`);
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

function replace_with_dummy(queue){
  if(queue.length > 0){
    var file_path = queue[0];
    var copy_path = file_path + '.copy'
    fs.copyFileSync(file_path, copy_path);
    queue[0]= copy_path
  }
}

function main(){
  const app = express();
  app.use(express.static(path.join(__dirname, 'public/')));
  app.use(express.static(path.resolve(__dirname+'/../images')));

  const upload_dir = path.resolve(path.join(__dirname, config.upload_dir));
  var stream_queue = [path.join(upload_dir, 'stream.jpeg')];
  var thermal_queue = [path.join(upload_dir, 'thermal.jpeg')]
  var visible_queue = [path.join(upload_dir, 'visible.jpeg')];
  replace_with_dummy(stream_queue);
  replace_with_dummy(thermal_queue);
  replace_with_dummy(visible_queue);
  
  app.get('/image/stream', returnImage(stream_queue));
  app.get('/image/thermal', returnImage(thermal_queue));
  app.get('/image/visible', returnImage(visible_queue));

  app.get('/', function(req, res) {
    res.sendFile(path.join(__dirname + '/index.html'));
    console.log('index');
  });
  
  
  app.use(busboy()); 
  app.post('/upload', function(req, res) {
      var fstream;
      req.pipe(req.busboy);
      req.busboy.on('file', function (fieldname, file, filename) {
          // console.log("Uploading: " + filename); 
          file_path = path.join(upload_dir, filename);
          fstream = fs.createWriteStream(file_path);
          file.pipe(fstream);
          fstream.on('close', function () {
              res.end();
              stream_queue.push(file_path);
              if(stream_queue.length > config.queue_capacity){
                var to_delete = stream_queue.shift();
                fs.unlink(to_delete, ()=>{});
              }
          });
      });
  });

  app.listen(config.web_server.network.port, function () {
    console.log(`Listening on port ${config.web_server.network.port}!`);
  });
}

main();