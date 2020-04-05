var net = require('net');
var fs = require('fs');
var buffer = require('buffer');
const path = require('path');
const config = require(path.join(__dirname,'config.json'));

module.exports = function(){
    const port = config.image_receiver.network.port;
    const host = config.image_receiver.network.host;
    var server = net.createServer((conn) => {
        console.log('image transfer connected');
        conn.on('data', (data) => {
            console.log('data received');
            console.log('data is: \n' + data);
        });        
    });

    server.listen(
        port, 
        host,
        () => {
            console.log('image receiver bound to ' + port + '\n');
            server.on('connection', () => {
                console.log('connection made...\n')
            });
        }
    );
}

