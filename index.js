const {
    _init, _initDevice, _release, _getDevices
} = require('./build/Release/wasapi.node')

_init()

function initDevice(sout='', sin='', a=0) {
    return _initDevice(sout, sin, a)
}

function release() {
    _release()
}

function lookupDevices(type='all') {
    return _getDevices(type)
}

module.exports = {
    initDevice,
    release,
    lookupDevices,
}