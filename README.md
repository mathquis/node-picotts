# node-picotts

## Usage

```javascript
const PicoTTS = require('node-picotts')

const engine = new PicoTTS({
	ta_file: 'lang/fr-FR_ta.bin',
	sg_file: 'lang/fr-FR_nk0_sg.bin',
	volume: 90, // default 100
	speed: 95, // default 100
	pitch: 105, // default 100
	debug: false // If true, wil print logs to stdout
})

engine
	.on('error', err => {
		// Handle error during synthesis
	})
	.on('data', (chunk, isLastChunk) => {
		// 16 bits samples buffer
	})
	.on('end', () => {
		// Synthesis is done
	})

engine.speak('Bonjour', {
	volume: 95,
	pitch: 102,
	speed: 100
})
```