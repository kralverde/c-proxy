var http = require('http'); // Import Node.js core module

var server = http.createServer(function (req, res) {   //create web server
    if (req.url == '/') { //check the URL of the current request
        
        // set response header
        res.writeHead(200, { 'Content-Type': 'text/html' }); 
        
        // set response content    
        res.write('<html><body><p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer placerat mi urna, nec imperdiet justo interdum quis. Morbi metus elit, dignissim non ex at, ullamcorper feugiat justo. Nam erat lectus, interdum in condimentum eu, sagittis a eros. Vivamus cursus euismod nisi id egestas. In pharetra orci erat, et ultrices diam pulvinar quis. Fusce dignissim rhoncus dui, et pharetra tortor tempus aliquam. Nam ac maximus leo. Mauris a aliquet ipsum. Nulla non auctor sapien, eu congue mi. Fusce sollicitudin erat non posuere condimentum. Sed sed congue tellus. Fusce vulputate a elit et interdum. Nulla elit nibh, consectetur finibus lorem ut, malesuada gravida dolor. Sed accumsan ipsum at felis tempor pharetra. Morbi ac augue erat. Sed tristique, arcu sit amet vestibulum bibendum, neque ex aliquam quam, sagittis lacinia quam nibh non ligula.</p><p>Maecenas vestibulum libero a fringilla gravida. Curabitur nec lorem tincidunt, scelerisque lectus et, consectetur libero. Duis hendrerit turpis in congue posuere. Aenean sit amet tortor tincidunt, commodo tellus in, vestibulum leo. Donec et ultricies magna. Ut in dictum orci. Sed et sapien ut nibh bibendum porta non vitae ligula. Sed sit amet velit et leo viverra imperdiet. Nulla efficitur ex ornare, condimentum odio eu, finibus nisi. Suspendisse fringilla odio ac fringilla tempus. Nulla gravida lorem vel placerat placerat. Sed vitae nisi at enim lobortis commodo. Nulla venenatis quis mauris vel vehicula</p><p>Mauris sed viverra nisl. Phasellus porttitor justo quis pharetra iaculis. Morbi eget scelerisque diam, in blandit tellus. Integer iaculis urna orci, bibendum maximus enim ornare at. Phasellus augue augue, interdum sit amet sollicitudin at, suscipit eu nibh. Vestibulum eget massa nec mauris laoreet hendrerit. Curabitur ultrices, felis ac faucibus consectetur, lacus risus congue mauris, sed placerat nulla nibh et ex. Praesent malesuada vestibulum lacus, quis consequat nunc scelerisque lobortis. Nam scelerisque, dolor nec laoreet bibendum, purus mi condimentum nulla, eget viverra arcu nibh et libero. In vitae porta nisl.</p><p>Ut convallis nibh ac eros cursus varius. Quisque commodo orci risus, blandit vestibulum velit hendrerit rhoncus. In tristique semper ultrices. Fusce ullamcorper egestas velit, id condimentum sem porta nec. Quisque nec arcu quis sapien molestie tempus sodales sed diam. Nulla tempus venenatis augue nec cursus. Pellentesque id commodo arcu. Fusce id turpis vel sem rhoncus dignissim. Cras dignissim tincidunt consectetur. Aliquam tempus ipsum quis pulvinar ornare. Praesent sagittis auctor purus sollicitudin rhoncus.</p><p>Nullam sit amet turpis eu ex malesuada congue. Nam feugiat ultricies felis ac elementum. Suspendisse eleifend volutpat dignissim. Curabitur congue ante at lectus rutrum vulputate quis eu est. Cras et accumsan nunc. Vivamus eu tempus nulla. In quis lectus efficitur, dignissim massa et, malesuada justo. Cras eu augue eu nisl placerat vestibulum. Quisque vel ipsum ultricies, hendrerit lectus eget, maximus nibh. Nullam dapibus facilisis quam in suscipit.</p></body></html>');
        res.end();
    
    }
    else if (req.url == "/student") {
        
        res.writeHead(200, { 'Content-Type': 'text/html' });
        res.write('<html><body><p>This is student Page.</p></body></html>');
        res.end();
    
    }
    else if (req.url == "/admin") {
        
        res.writeHead(200, { 'Content-Type': 'text/html' });
        res.write('<html><body><p>This is admin Page.</p></body></html>');
        res.end();
    
    }
    else
        res.end('Invalid Request!');

});

server.listen(5000); //6 - listen for any incoming requests

console.log('Node.js web server at port 5000 is running..')

