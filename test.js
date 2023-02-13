
const winreg = require( './build/Release/cm' );

console.log( winreg.getKeyValue( 'hklm:SYSTEM\\ControlSet001\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000\\HardwareInformation.qwMemorySize' ) );
console.log( winreg.getKeyValue( 'hkcu', 'SOFTWARE\\Valve\\Steam\\ActiveProcess', 'SteamClientDll64' ) );
console.log( winreg.getKeyValue( 'hklm', 'SYSTEM\\ControlSet001\\Control', 'PreshutdownOrder' ) );