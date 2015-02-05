# librsync
Visual Studio 2013 version of librsync as found on: https://github.com/librsync/librsync

# Changes
* Included libpopt, as found on: https://github.com/dropbox/librsync
* Generated ```prototab.h``` and ```prototab.c```
* Added a ```VISUALSTUDIO``` Preprocessor Definition which takes care of the ```inline``` to ```__inline``` transition.
 * Note: The Blake2 implementation doesn't like this trick and is therefor copied and modified appropriately.
* Added slightly modified ```rdiff.c``` which will export functions to a DLL if the ```BUILDDLL``` Preprocessor Definition is defined.
* Seperated projects into librsync and rdiff
 * librsync outputs to a Static Library
 * rdiff outputs to an Executable or a DLL, depending on the selected Build Configuration.

No files outside of the ```PCbuild```-folder have been modified. This should technically allow for easy upgrading.

# rdiff API
When you choose to build rdiff as a DLL, the following functions will be available to you.

```
rs_result rdiff_sig(const char *baseFile, const char *sigFile);
rs_result rdiff_delta(const char *sigFile, const char *newFile, const char *deltaFile);
rs_result rdiff_patch(const char *baseFile, const char *deltaFile, const char *newFile);
```

By just exposing these functions, you can implement the rsync-algorithm into your applications as if you were directly running the rdiff-Executable, without having to worry about the full librsync-API.

On top of that, the much simpler function-signatures allow for Pinvoking from .NET as well. An example for that can be found in ```PCbuild\RDiff.cs```.