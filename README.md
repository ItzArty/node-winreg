# node-winreg
A nodejs module for reading from the Microsoft Windows registry

Will soon get back to this.

You can expect to see editing and watching methods to be added in the future with an asynchronous wrapper on top of that.

Not so much a bug: Often reads may result in error 0x2 which is referred to as `ERROR_FILE_NOT_FOUND`, even though the path could exist.

Basic usage:

`winreg.getKeyValue( hive, path, key )`
`winreg.getKeyValue( hive, path + key )`
`winreg.getKeyValue( hive : path + key )`
