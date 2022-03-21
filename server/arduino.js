const SerialPort = require('serialport')
const Readline = require('@serialport/parser-readline')

function openSerialPort({ path, baudRate }) {
  const port = new SerialPort(path, { baudRate })
  const parser = port.pipe(new Readline({ delimiter: '\n' }))

  port.on('open', () => {
    console.log(`Serial port ${path} opened`)
  })

  parser.on('data', data => {
    console.log('[Arduino]', data)
  })

  return port
}

function sendToArduino(port, params, callback) {
  const { speed, steering } = params

  port.write(`2 ${speed} ${steering}\n`, (err) => {
    if (err) console.log(`Error writing ${params} to Arduino:`, err.message)
    if (callback) callback(err)
  })
}

module.exports = { openSerialPort, sendToArduino }