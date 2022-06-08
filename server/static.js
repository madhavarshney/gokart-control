const { createServer } = require('http');
const express = require('express');
const proxy = require('express-http-proxy');

const { httpPort } = require('./config')

const app = express()
const httpServer = createServer(app)

app.use(express.static('public'));
app.use('/api', proxy('172.20.10.5'));

httpServer.listen(httpPort, () => {
  console.log(`Go-kart controller listening on port ${httpPort}`)
})
