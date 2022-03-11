const path = require('path');
const express = require('express')
const SerialPort = require('serialport');
const Readline = require('@serialport/parser-readline');

const app = express()
const port = new SerialPort('/dev/cu.usbmodem144101', { baudRate: 9600 });
const parser = port.pipe(new Readline({ delimiter: '\n' }));

// Read the port data
port.on('open', () => {
  console.log('Serial port opened');
});

parser.on('data', data => {
  console.log('Got word from arduino:', data);
});

app.get('/', function(req, res) {
  res.sendFile(path.join(__dirname, '/index.html'));
});

app.get('/api/set/:speed/:steering', (req, res) => {
  try {
    const speed = parseInt(req.params.speed, 10)
    const steering = parseInt(req.params.steering, 10)

    console.log(req.params)

    port.write(`0 ${speed}\n`, (err) => {
      if (err) {
        console.log('Error on write: ', err.message);
        res.status(500).send({ error: err.message })
      }

      console.log(`Speed set to ${speed}`);

      port.write(`1 ${steering}\n`, (err) => {
        if (err) {
          console.log('Error on write: ', err.message);
          res.status(500).send({ error: err.message })
        }

        console.log(`Steering set to ${steering}`);
        res.status(200).send({ status: 'success' })
      });
    });
  } catch (err) {
    res.status(500).send({ error: err.message })
  }
})

app.listen(3100, () => {
  console.log(`Go-kart controller listening on port 3100`)
})
