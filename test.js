const PicoTTS		= require('./lib/')
const {WaveFile}	= require('wavefile')
const File			= require('fs')

const c = new PicoTTS({
	ta_file: 'lang/fr-FR_ta.bin',
	sg_file: 'lang/fr-FR_nk0_sg.bin',
	volume: 90,
	speed: 95,
	pitch: 105,
	debug: false
})

const samples = []

c
	.on('error', err => {
		console.error(err)
	})
	.on('data', (chunk, isLastChunk) => {
		// console.log('CHUNK', isLastChunk)
		for ( let i = 0 ; i < chunk.byteLength ; i += 2 ) {
			const s = chunk.readInt16LE(i, i + 2)
			samples.push(s)
		}
	})
	.on('end', () => {
		const wav = new WaveFile()
		wav.fromScratch(1, 16000, 16, samples)
		File.writeFileSync('test.wav', wav.toBuffer())
	})

// const text = 'Lecture des meilleures chansons de Céline Dion dans le salon.'
const text = 'Bonjour à tous'

console.log(c.speak(text))