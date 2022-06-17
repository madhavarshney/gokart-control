const { sendToArduino } = require('./arduino')

let recordingMode = 'OFF' // OFF | RECORD | PLAY
let recordedStates = []
let curRecordingFrame = -1

function startRecording(port) {
  recordingMode = 'RECORD'
  recordedStates = []
}

function addRecordingFrame(state) {
  recordedStates.push(state)
}

function stopRecording(port) {
  if (recordingMode === 'RECORD') {
    const isNotEmpty = (state) => {
      return state.throttleSpeed > 0 || state.steeringTargetPos !== state.steeringCenterPos
    }

    // delete empty frames at the start of the recording
    for (var i = 0; i < recordedStates.length; i++)
      if (isNotEmpty(recordedStates[i]))
        break

    recordedStates.splice(0, i)


    // delete empty frames at the end of the recording
    for (var i = recordedStates.length - 1; i >= 0; i--)
      if (isNotEmpty(recordedStates[i]))
        break

    recordedStates.splice(i + 2)
  } else if (recordingMode === 'PLAY') {
    sendToArduino(port, { speed: 0, steering: 0 })
  }

  recordingMode = 'OFF'
}

function playRecording(port) {
  if (recordingMode === 'RECORD')
    stopRecording()

  recordingMode = 'PLAY'
  curRecordingFrame = 0

  function sendState() {
    if (recordingMode !== 'PLAY') return

    const curState = recordedStates[curRecordingFrame]

    sendToArduino(port, {
      speed: curState.throttleSpeed,
      steering: curState.steeringTargetPos - curState.steeringCenterPos,
    })

    if (recordedStates.length - 1 > curRecordingFrame) {
      setTimeout(sendState, recordedStates[++curRecordingFrame].time - curState.time)
    } else {
      curRecordingFrame = -1
      recordingMode = 'OFF'
      sendToArduino(port, { speed: 0, steering: 0 })
    }
  }

  sendState()
}

module.exports = {
  recordingMode,
  recordedStates,
  curRecordingFrame,
  startRecording,
  addRecordingFrame,
  stopRecording,
  playRecording,
}
