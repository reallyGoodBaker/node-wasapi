interface DeviceInfo {
    readonly deviceId: string
    readonly name: string
    readonly desc: string
}

/**
 * accessibility:
 * 0 None
 * 1 Output
 * 2 Input
 * 3 Both
 * @param sout 
 * @param sin 
 * @param accessibility 
 */
export function initDevice(sout?: string, sin?: string, accessibility?: 0|1|2|3): boolean
export function release(): void
export function lookupDevices(type?:'all'|'input'|'output'): DeviceInfo[] | null