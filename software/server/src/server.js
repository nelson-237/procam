const express = require('express');
const fs = require('fs');
const path = require('path');
const app = express();
const config = require('config.json');


app.use(express.static(path.join(__dirname, 'public')));

app.get('/', function(req, res) {
  res.sendFile(path.join(__dirname + '/index.html'));
  console.log('page');
})

function returnImage(image_path){
  return (req, res) => {
    const stat = fs.statSync(video_path);
    const fileSize = stat.size;
    const range = req.headers.range;

    if (range) {
      const parts = range.replace(/bytes=/, "").split("-");
      const start = parseInt(parts[0], 10);
      const end = parts[1]
        ? parseInt(parts[1], 10)
        : fileSize-1;

      if(start >= fileSize) {
        res.status(416).send('Requested range not satisfiable\n'+start+' >= '+fileSize);
        return;
      }
      
      const chunksize = (end-start)+1;
      const file = fs.createReadStream(video_path, {start, end});
      const head = {
        'Content-Range': `bytes ${start}-${end}/${fileSize}`,
        'Accept-Ranges': 'bytes',
        'Content-Length': chunksize,
        'Content-Type': 'video/mp4',
      };

      res.writeHead(206, head);
      file.pipe(res);
    } else {
      const head = {
        'Content-Length': fileSize,
        'Content-Type': 'video/mp4',
      };
      res.writeHead(200, head);
      fs.createReadStream(video_path).pipe(res);
    }
  }
};

const thermal_image_path = path.resolve(__dirname+'/../images/thermal_sample.jpeg');
app.get('/thermal', returnImage(thermal_image_path));

const visible_image_path = path.resolve(__dirname+'/../images/visible_sample.jpeg');
app.get('/visible', returnImage(visible_image_path));

app.listen(config.network.port, function () {
  console.log(`Listening on port ${config.network.port}!`);
});