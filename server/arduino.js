const SerialPort = require('serialport')
const Readline = require('@serialport/parser-readline')

function createSerialPort({ path, baudRate, onUpdate, onClose }) {
  let disconnectPort = false

  const port = new SerialPort(path, { baudRate, autoOpen: false })
  const parser = port.pipe(new Readline({ delimiter: '\n' }))

  port.on('open', () => {
    console.log(`Serial port ${path} opened`)
  })

  port.on('close', () => {
    console.log(`Serial port ${path} closed`)
    onClose()
    if (!disconnectPort) {
      openSerialPort(port)
    }
  })

  parser.on('data', dataString => {
    let data

    try {
      data = JSON.parse(dataString)
    } catch (err) {
      console.log(`Arduino: ${dataString}`)
      return
    }

    onUpdate(data)
  })

  const setDisconnectPort = (_disconnectPort) => {
    if (disconnectPort !== _disconnectPort) {
      disconnectPort = _disconnectPort
  
      if (disconnectPort) {
        port.close()
      } else {
        openSerialPort(port)
      }
    }
  }

  return { port, setDisconnectPort }
}

function openSerialPort(port) {
  console.log(`Opening serial port ${port.path}`)
  port.open((err) => {
    if (err) {
      console.log('Error opening port: ', err.message)
      setTimeout(() => openSerialPort(port), 800)
    }
  })
}

function sendToArduino(port, params, callback) {
  const { speed, steering } = params

  port.write(`2 ${speed} ${steering}\n`, (err) => {
    if (err) console.log(`Error writing ${params} to Arduino:`, err.message)
    if (callback) callback(err)
  })
}

module.exports = { createSerialPort, openSerialPort, sendToArduino }
