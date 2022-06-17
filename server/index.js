const { createServer } = require('http')
const { Server } = require('socket.io')
const express = require('express')

const { createSerialPort, openSerialPort, sendToArduino } = require('./arduino')
const {
  recordingMode,
  recordedStates,
  curRecordingFrame,
  startRecording,
  addRecordingFrame,
  stopRecording,
  playRecording,
} = require('./recording')
const {
  enableRestControl,
  httpPort,
  serialPort,
  serialBaudRate
} = require('./config')

const app = express()
const httpServer = createServer(app)
const io = new Server(httpServer, {})

let forceDisconnectFromArduino = false
let lastArduinoState = null
// let remoteIsConnected = false

const { port, setDisconnectPort } = createSerialPort({
  path: serialPort,
  baudRate: serialBaudRate,
  onUpdate: (data) => {
    lastArduinoState = data

    if (recordingMode === 'RECORD') {
      addRecordingFrame({
        time: data.general.millis,
        throttleSpeed: data.throttle.speed,
        steeringTargetPos: data.steering.targetPos,
        steeringCurrentPos: data.steering.currentPos,
        steeringSpeed: data.steering.speed,
        steeringCenterPos: data.steering.centerPos,
      })
    }

    sendState(io)
  },
  onClose: () => {
    lastArduinoState = null
    sendState(io)
  }
})

function sendState(socketOrIo) {
  socketOrIo.emit('state', {
    connectedToArduino: port.isOpen,
    hasArduinoState: !!lastArduinoState,
    forceDisconnectFromArduino,
    recording: {
      mode: recordingMode,
      hasRecorded: recordedStates.length > 0,
      currentFrame: curRecordingFrame,
    },
    ...(lastArduinoState || {}),
  })
}

app.use(express.static('public'))

io.on('connection', (socket) => {
  // if (remoteIsConnected)
  //   return socket.disconnect()

  // remoteIsConnected = true

  sendState(socket)

  socket.on('set', (speed, steering) => {
    if (port.isOpen) {
      sendToArduino(port, { speed, steering })
    }
  })

  socket.on('set-disconnect-arduino', (shouldDisconnect) => {
    setDisconnectPort(shouldDisconnect)
    forceDisconnectFromArduino = shouldDisconnect
  })

  socket.on('start-recording', () => startRecording(port))
  socket.on('stop-recording', () => stopRecording(port))
  socket.on('play-recording', () => playRecording(port))

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
        return res.status(502).send({ error: 'Arduino not connected' })
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
