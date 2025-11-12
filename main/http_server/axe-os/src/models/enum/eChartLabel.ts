export enum eChartLabel {
    hashrate = 'Hashrate',
    asicTemp = 'ASIC Temp (Avg)',
    asicTemp1 = 'ASIC Temp 1',
    asicTemp2 = 'ASIC Temp 2',
    errorPercentage = 'Error Percentage',
    vrTemp = 'VR Temp',
    asicVoltage = 'ASIC Voltage',
    voltage = 'Voltage',
    power = 'Power',
    current = 'Current',
    fanSpeed = 'Fan Speed',
    fanRpm = 'Fan RPM',
    fan2Rpm = 'Fan 2 RPM',
    wifiRssi = 'Wi-Fi RSSI',
    freeHeap = 'Free Heap',
    none = 'None'
}

export function chartLabelValue(enumKey: string) {
  return Object.entries(eChartLabel).find(([key, val]) => key === enumKey)?.[1];
}

export function chartLabelKey(value: eChartLabel): string {
  return Object.keys(eChartLabel)[Object.values(eChartLabel).indexOf(value)];
}
