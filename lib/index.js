const Path			= require('path')
const Binary		= require('node-pre-gyp')
const binding_path	= Binary.find(Path.resolve(Path.join(__dirname,'../package.json')))
const {SvoxPicoTTS}	= require(binding_path)
const EventEmitter	= require('events')
const inherits		= require('util').inherits

inherits(SvoxPicoTTS, EventEmitter)

module.exports = SvoxPicoTTS