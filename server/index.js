const { createServer } = require('http')
const { Server } = require('socket.io')
const express = require('express')

const { createSerialPort, openSerialPort, sendToArduino } = require('./arduino')
const {
  enableRestControl,
  httpPort,
  serialPort,
  serialBaudRate
} = require('./config')

const app = express()
const httpServer = createServer(app)
const io = new Server(httpServer, {})

const port = createSerialPort({
  path: serialPort,
  baudRate: serialBaudRate,
  onUpdate: (data) => io.emit('arduino-state', data)
})

// let remoteIsConnected = false

app.use(express.static('public'))

io.on('connection', (socket) => {
  // if (remoteIsConnected)
  //   return socket.disconnect()

  // remoteIsConnected = true

  socket.on('set', (speed, steering) => {
    if (port.isOpen) {
      sendToArduino(port, { speed, steering })
    }
  })

  socket.on('disconnect', () => {
    // remoteIsConnected = false
    sendToArduino(port, { speed: 0, steering: 0 })
  })
})

if (enableRestControl) {
  app.get('/api/set/:speed/:steering', (req, res) => {
    try {
      const speed = parseInt(req.params.speed, 10)
      const steering = parseInt(req.params.steering, 10)

      console.log(req.params)

      if (!port.isOpen) {
        return res.status(502).send({ error: 'Arduino not connected' });
      }

      sendToArduino(port, { speed, steering }, (err) => {
        if (err)
          return res.status(500).send({ error: err.message })

        res.status(200).send({ status: 'success' })
      })
    } catch (err) {
      res.status(500).send({ error: err.message })
    }
  })
}

openSerialPort(port)
httpServer.listen(httpPort, () => {
  console.log(`Go-kart controller listening on port ${httpPort}`)
})
