interface DeviceInfo {
    readonly deviceId: string
    readonly name: string
    readonly desc: string
}

export const DeviceType: {
    readonly Output: number,
    readonly Input: number,
}

/**
 * @param type
 * `DeviceType.Output`
 * or `DeviceType.Input`
 * or `DeviceType.Output | DeviceType.Input`
 * @param sout 
 * @param sin 
 * @param accessibility 
 */
export function initDevice(type?: number, sout?: string, sin?: string): boolean
export function release(): void
/**
 * @param type
 * `DeviceType.Output`
 * or `DeviceType.Input`
 * or `DeviceType.Output | DeviceType.Input`
 */
export function lookupDevices(type?: number): DeviceInfo[] | null

interface WavFormat {
    readonly wFormatHelp: string;        /* format type string */
    readonly wFormatTag: number;         /* format type */
    readonly nChannels: number;          /* number of channels (i.e. mono, stereo...) */
    readonly nSamplesPerSec: number;     /* sample rate */
    readonly nAvgBytesPerSec: number;    /* for buffer estimation */
    readonly nBlockAlign: number;        /* block size of data */
    readonly wBitsPerSample: number;     /* number of bits per sample of mono data */
    readonly cbSize: number;             /* the count in bytes of the size of */
    readonly isExtensible: boolean;
    readonly wFormatTagHelp: string;
}

interface WavSamples {
    readonly wValidBitsPerSample: number
    readonly wSamplesPerBlock: number
    readonly wReserved: number
}

interface SubFormatGetter {
    arrayBuffer(): ArrayBuffer
    string(): string
}

interface WavFormatEx {
    readonly format: WavFormat
    readonly samples: WavSamples
    readonly dwChannelMask: number
    readonly subFormat: SubFormatGetter
    getChannels(): string[]
}

/**
 * @param type
 * `DeviceType.Output`
 * or `DeviceType.Input`
 */
export function getFormat(type: number): WavFormat
/**
 * @param type
 * `DeviceType.Output`
 * or `DeviceType.Input`
 */
export function getFormatEx(type: number): WavFormatEx


export const ShareMode: {
    readonly Shared: number,
    readonly Exclusive: number,
}

/**
 * @param shareMode
 * `ShareMode.Shared`
 * or `ShareMode.Exclusive`
 * @param bufferDuration
 * ```js
 * // transfer frame size(int) to buffer duration(int):
 * bufferDuration = Math.ceil(frameSize * 10 ** 7 / sampleRate)
 * ```
 */
export function initCaptureService(shareMode: number, bufferDuration: number): boolean
/**
 * @param shareMode
 * `ShareMode.Shared`
 * or `ShareMode.Exclusive`
 * @param bufferDuration
 * ```js
 * // transfer frame size(int) to buffer duration(int):
 * bufferDuration = Math.ceil(frameSize * 10 ** 7 / sampleRate)
 * ```
 */
export function initRenderService(shareMode: number, bufferDuration: number): boolean

/**
 * @param type 
 * `DeviceType.Output`
 * or `DeviceType.Input`
 */
export function start(type: number): boolean

/**
 * @param type 
 * `DeviceType.Output`
 * or `DeviceType.Input`
 */
export function stop(type: number): boolean

export function service(nframesRender: number): void 
export function stopService(): void

interface NeedDataHandler {
    (
        freeSize: number,
        write: (data: ArrayBuffer) => void
    ): number
}

interface RecvDataHandler {
    (
        data: ArrayBuffer,
        nFramesToRead: number,
        pdwFlags: number,
        devicePosition: BigInt,
        qpcPosition: BigInt
    ): void
}

export function setNeedDataHandler(handler: NeedDataHandler): boolean
export function setRecvDataHandler(handler: RecvDataHandler): boolean