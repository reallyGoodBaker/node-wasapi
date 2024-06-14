const { initDevice, release, lookupDevices } = require('./index')

console.log(lookupDevices('output'))
release()