const fs = require('fs/promises')
const {
    initDevice,
    release,
    lookupDevices,
    DeviceType: { Output },
    getFormat,
    getFormatEx,
    ShareMode: { Shared, Exclusive },
    initRenderService,
    start,
    stop,
    service,
    stopService,
    setNeedDataHandler,
} = require('../index')

console.log(1, lookupDevices(Output))
console.log(2, initDevice(Output))
let format = getFormat(Output)
/**
 * @type {import('../index').WavFormatEx}
 */
let formatEx = null
console.log(3, format.isExtensible ? (formatEx = getFormatEx(Output)) : format)

const hns = Math.ceil(2048 * (10 ** 7) / format.nSamplesPerSec)
console.log(4, initRenderService(Shared, hns))
console.log(5, formatEx ? formatEx.subFormat.string() : 'Unsupported')
console.log(6, formatEx ? formatEx.getChannels() : 'Unsupported')

async function readWav() {
    const wav = new Float32Array(await fs.readFile('./test/test.pcm'))
    const framesRequest = 1024
    console.log(wav.length)

    let index = 0
    console.log(7, setNeedDataHandler((size, write) => {
        write(wav.slice(index, index += framesRequest).buffer)
        
        process.stdout.clearLine(-1)
        process.stdout.write((index / wav.length * 100).toFixed(2) + '%\r')

        if (index >= wav.length) {
            process.stdout.clearLine(-1)
            console.log('Done')
            stopService()
            return 0
        }

        return framesRequest
    }))

    start(Output)
    console.log('Playing')
    service(framesRequest)
    stop(Output)

    release()
}


readWav()