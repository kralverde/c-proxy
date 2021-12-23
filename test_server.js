const http = require('http'); // Import Node.js core module
const fs = require('fs');

// set response content
fs.readFile('test.txt', 'utf-8', (err, data) => {
    if (err) {
        data = 'ERROR';
    }
    data = data.split('\n').map(x => x.trim()).join(' ');
    var server = http.createServer(function (req, res) {   //create web server
        if (req.url == '/') { //check the URL of the current request
            
            // set response header
            res.writeHead(200, { 'Content-Type': 'text/html' }); 
            res.write(data);
            res.end();
        }
        else
            res.end('Invalid Request!');
    });
    server.listen(5000); //6 - listen for any incoming requests
    console.log('Node.js web server at port 5000 is running..')
});

