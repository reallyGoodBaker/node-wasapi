const {
    _init,
    _initDevice: initDevice,
    _release,
    _getDevices: lookupDevices,
    _getFormat,
    _getFormatEx,
    _initRenderService: initRenderService,
    _initCaptureService: initCaptureService,
    _start: start,
    _stop: stop,
    _service: service,
    _stopService: stopService,
    _setNeedDataHandler: setNeedDataHandler,
    _setRecvDataHandler: setRecvDataHandler,
} = require('./build/Release/wasapi.node')

const { FORMAT_TAGS } = require('./format-tags')

_init()

function release() {
    _release()
}

const DeviceType = {
    Output: 1,
    Input: 2,
}

function getFormat(type) {
    const obj = _getFormat(type)
    obj.wFormatTagHelp = FORMAT_TAGS[obj.wFormatTag]
    obj.isExtensible = obj.wFormatTag === 65534
    return obj
}

const ShareMode = {
    Shared: 0,
    Exclusive: 1,
}

const channelPositions = {
    0x1: 'FrontLeft',
    0x2: 'FrontRight',
    0x4: 'FrontCenter',
    0x8: 'LowFrequency',
    0x10: 'BackLeft',
    0x20: 'BackRight',
    0x40: 'FrontLeftOfCenter',
    0x80: 'FrontRightOfCenter',
    0x100: 'BackCenter',
    0x200: 'SideLeft',
    0x400: 'SideRight',
    0x800: 'TopCenter',
    0x1000: 'TopFrontLeft',
    0x2000: 'TopFrontCenter',
    0x4000: 'TopFrontRight',
    0x8000: 'TopBackLeft',
    0x10000: 'TopBackCenter',
    0x20000: 'TopBackRight',
}

function getChannels() {
    const channelMask = this.dwChannelMask
    const channels = []
    for (let i = 0; i < 32; i++) {
        if (channelMask & (1 << i)) {
            channels.push(channelPositions[1 << i])
        }
    }
    return channels
}

function getFormatEx(type) {
    const obj = _getFormatEx(type)
    obj.getChannels = getChannels.bind(obj)
    return obj
}

module.exports = {
    DeviceType,
    initDevice,
    release,
    lookupDevices,
    getFormat,
    getFormatEx,
    ShareMode,
    initCaptureService,
    initRenderService,
    start,
    stop,
    service,
    stopService,
    setNeedDataHandler,
    setRecvDataHandler,
}