# CallBackDump-CS
 Lsass memory dump.
 [CallBackDump](https://github.com/seventeenman/CallBackDump) Reflection dll CS plugin, modified on the original basis.

## modified

Supports passing custom encryption key in the beacon command console. If the encryption key is not specified, the default is: **`fuck`**

Dump lsass file path:  get the current cache directory (C:\ProgramData directory is used when the cache directory acquisition fails) + 12-bit random file name + .log

The original dumpXor is slightly modified to decXor, which supports parameter passing decryption key

## Usage:

Load CallBackDump.cna in the bin directory

```sh
1. help callbackdump
2. callbackdump or callbackdump <xor key>
3. decXor <xor encrypt dump file>  <xor key>
4. pypykatz lsa minidump <dump file>
```

**Thanks** 

* https://github.com/seventeenman/CallBackDump
* https://github.com/outflanknl/Zipper

