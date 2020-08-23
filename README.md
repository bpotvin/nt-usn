# NT-USN

I had to work once with an application that needed to know if files in particular directories had been changed. This wasn't unreasonable, it was part of the basic purpose of the application. The application accomplished the task by starting a supporting program that opened handles to the directories it was interested in and waited for change notifications. This worked out ok, unless the directories involved were on a USB drive intended to move between systems: open handles means the the drive would refuse to eject.

There's a solution on nt: [NTFS change journals](https://docs.microsoft.com/en-us/windows/win32/fileio/change-journals). From the description:


>"The NTFS file system maintains an update sequence number (USN) change journal. When any change is made to a file or directory in a volume, the USN change journal for that volume is updated with a description of the change and the name of the file or directory."

This code reads and displays information from an NTFS USN change journal.

Keep in mind that USN change records indicate changes have been made to a file, but they do not show <i>what</i> was changed. There are 24 change reasons listed in the [winioctl.h](https://docs.microsoft.com/en-us/windows/win32/api/winioctl/) header. The Microsoft journal documentation indicates changes are cumulative as long as a file is open; the 'close' reason in a change shows all of the changes that happened while the file was open.

More information can be found here:
  * https://docs.microsoft.com/en-us/windows/win32/fileio/change-journals
  * https://docs.microsoft.com/en-us/windows/win32/fileio/change-journal-records

## Process
This is the general flow of this code:
```
_UsnpReadJournalData                     open volume, get journal data
  + _UsnpFormatJournalData               print the journal data
  + _UsnpReadJournalRecords              get usn change records
    + _UsnpFormatRecord                  print records based on version
      | _UsnpFormatRecordV2              print v2 (not implemented)
      | _UsnpFormatRecordV3              print v3
      | _UsnpFormatRecordV4              print v4 (not implemented)
         + _UsnpGetFilenameFromFileId    get name from a FILE_ID_128
         + _UsnpFormatTimestamp          format an nt timestamp
         + _UsnpDump                     hex-dump

_UsnpGetFileIdFromFilename               get fild_id_128 from filename
_UsnpGetFileIdFromHandle                 get fild_id_128 from handle
```
USN change records contain the file-reference-number of the file that has
changed, along with a parent-file-reference-number, the directory where the
file is/was. The [OpenFileById](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-openfilebyid) function can be used to get the name of the
directory and then compare that name with locations of interest. Another way
to filter would be to open a handle to a directory of interest, then use the
[GetFileInformationByHandle](https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileinformationbyhandle) function and then save the fileindex, then while enumerating change records, compare with the parent-file-reference-number.

Example usn change record:
```
 FRN          0000000000000000002400000009D875
 Parent FRN   0000000000000000000D0000000A0CD9 - \\?\C:\Users\me\AppData\Local\Temp
 USN          0000000498241F98
 Reason       80000102
 Attributes   00000120
 FileName     _CL_3817d218in
 TimeStamp    01D63C5D4ABBA32B - 2020-06-06 23:50:43.744
```
Here's an example GetFileInformationByHandle fileindex:
```
000D0000000A0CD9 C:\Users\me\AppData\Local\Temp
```
The lowpart of the file-reference-number is the fileindex:
```
0000000000000000000D0000000A0CD9 - file-reference-number (FILE_ID_128)
                ^^^^^^^^^^^^^^^^
                000D0000000A0CD9 - fileindex
```
The _UsnpGetFileIdFromFilename and _UsnpGetFileIdFromHandle functions can be
used to get the file index for a file or directory.

## Build
Open a "vc tools" command prompt, either 32-bit or 64-bit, change to the directory containing the dsw.c file and then:
```
# cl -W4 j0.c
```

## Files
The following files are included:
```
NT-USN
|   j0.c                        source.
\   README.md                   this.
```
That is all.
```
  SR  15,15
  BR  14
```
