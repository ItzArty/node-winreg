# node-winreg
A nodejs module for reading from the Microsoft Windows registry

Will soon get back to this.

You can expect to see editing and watching methods to be added in the future with an asynchronous wrapper on top of that.

Not so much a bug: Often reads may result in error 0x2 which is referred to as `ERROR_FILE_NOT_FOUND`, even though the path could exist.

`getKeyValue( ... ) : [ Array, String, Number, BigInt ]`

  Accepts from 1 up to 3 arguments in the following combinations:
  1. `HIVE`, `PATH`, `KEY`
  2. `HIVE`, `PATH` / `KEY`
  3. `HIVE` : `PATH` / `KEY`
  
  Returns an array, string, number or a bigint depending on what datatype the request key is
  * REG_QWORD (unsigned 64bit integer): BigInt
  * REG_DWORD (unsigned 32bit integer): Number
  * REG_SZ (string): String
  * REG_MULTI_SZ (null-terminated string array): Array
    
  **Support for other datatypes is on the way**
