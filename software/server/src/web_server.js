const express = require('express');
const fs = require('fs');
const path = require('path');
const busboy = require('connect-busboy');

// We load the configuration file
const config = require(path.join(__dirname,'config.json'));

/**
 * Return a request handler 
 * that respond with the most recent 
 * element of the queue
 * @param {Array<string>} queue : a queue that gives the path of the latest uploaded images
 */
function returnImage(queue){
  return (req, res) => {
    // check if the queue is empty;
    if(queue.length == 0){
      res.set('Content-Type', 'text/plain');
      res.status(404).end('Not found');
      return;
    }

    //get the last element in the queue
    const image_path = queue[queue.length -1];
    // console.debug(`image: ${image_path}`);
    // check that the file exists
    if (! fs.existsSync(image_path)) {
      res.status(403).end('Forbidden');
    }
    // return the file as a jpeg
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

/**
 * This is just to avoid deleting the dummies in upload
 * @param {*} queue 
 */
function replace_with_dummy(queue){
  if(queue.length > 0){
    var file_path = queue[0];
    var copy_path = file_path + '.copy'
    fs.copyFileSync(file_path, copy_path);
    queue[0]= copy_path
  }
}

/**
 * Main function, start a webserver
 */
function main(){

  // create the server
  const app = express();

  // static files such as js and css of the webpage
  app.use(express.static(path.join(__dirname, 'public/')));

  //Initialize the queues with dummies
  const upload_dir = path.resolve(path.join(__dirname, config.upload_dir));
  var stream_queue = [path.join(upload_dir, 'stream.jpeg')];
  var thermal_queue = [path.join(upload_dir, 'thermal.jpeg')]
  var visible_queue = [path.join(upload_dir, 'visible.jpeg')];
  replace_with_dummy(stream_queue);
  replace_with_dummy(thermal_queue);
  replace_with_dummy(visible_queue);
  
  // start the image request handler
  app.get('/image/stream', returnImage(stream_queue));
  app.get('/image/thermal', returnImage(thermal_queue));
  app.get('/image/visible', returnImage(visible_queue));

  // returns the main webpage index.html
  app.get('/', function(req, res) {
    res.sendFile(path.join(__dirname + '/index.html'));
    console.log('index');
  });
  
  // This handles the camera uploads and updates the queues
  app.use(busboy()); 
  app.post('/upload', function(req, res) {
      // We pipe the uploaded file to the upload directory
      var fstream;
      req.pipe(req.busboy);
      req.busboy.on('file', function (fieldname, file, filename) {
          // console.log("Uploading: " + filename); 
          file_path = path.join(upload_dir, filename);
          fstream = fs.createWriteStream(file_path);
          file.pipe(fstream);
          // handle what happens after uploading completes
          fstream.on('close', function () {
              res.end();
              // We put the new file in the queue
              stream_queue.push(file_path);
              // if the queue is at max capacity, we remove the oldest file
              if(stream_queue.length > config.queue_capacity){
                var to_delete = stream_queue.shift();
                fs.unlink(to_delete, ()=>{});
              }
          });
      });
  });

  // start listening
  app.listen(config.web_server.network.port, function () {
    console.log(`Listening on port ${config.web_server.network.port}!`);
  });

  process.on('exit', function () {
    console.log('About to exit, waiting for remaining connections to complete');
    app.close();
  });
}

main();