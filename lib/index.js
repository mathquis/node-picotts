const {SvoxPicoTTS}	= require('../build/Release/picotts.node')
const EventEmitter	= require('events')
const inherits		= require('util').inherits

inherits(SvoxPicoTTS, EventEmitter)

module.exports = SvoxPicoTTS