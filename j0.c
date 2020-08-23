#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdint.h>

/*++
 * dummystruct below resolves to 'nothing' and at -W4 produces:
 * warning C4201: nonstandard extension used: nameless struct/union
 */
typedef union _LARGE_INTEGER128
{
    struct {
        uint64_t LowPart;
        int64_t HighPart;
    } DUMMYSTRUCTNAME;
    struct {
        uint64_t LowPart;
        int64_t HighPart;
    } u;
    uint8_t OctoPart[16];
} LARGE_INTEGER128, *PLARGE_INTEGER128;

/*++
 * dummystruct below resolves to 'nothing' and at -W4 produces:
 * warning C4201: nonstandard extension used: nameless struct/union
 */
typedef union _ULARGE_INTEGER128 
{
    struct {
        uint64_t LowPart;
        uint64_t HighPart;
    } DUMMYSTRUCTNAME;
    struct {
        uint64_t LowPart;
        uint64_t HighPart;
    } u;
    uint8_t OctoPart[16];
} ULARGE_INTEGER128, *PULARGE_INTEGER128;

/*++ do a private arraysize macro and avoid including the stdlib.h header ... */
#ifndef _countof
 #define _countof(__arr)        (sizeof((__arr)) / sizeof((__arr)[0]))
#endif  /* _countof */

/*++ buffer size for usn records ... */
#define _USN_BUFFER_SIZE        (USN_PAGE_SIZE * 2)

/*++ 
 * mask for 'all' change reasons. the current reasons mask is X'81FFFF77, but
 * existing examples use X'FFFFFFFF for 'all' ...
 */
#define _USN_REASON_ALL         0xFFFFFFFF

/*++
 * compile: cl -W4 [-Zi] j0.c
 * 
 * reading usn changes
 * the code here reads and displays usn change records. this can be useful for
 * applications that track file changes. note that usn change records indicate 
 * changes have been made to a file. they do not show what was changed.
 * 
 * there are 24 change reasons listed in the winioctl.h header. the microsoft 
 * journal documentation indicates changes are cumulative as long as a file is
 * open. the 'close' reason in a change shows all of the changes that happened
 * while the file was open.
 * 
 * more information here:
 *   https://docs.microsoft.com/en-us/windows/win32/fileio/change-journals
 *   https://docs.microsoft.com/en-us/windows/win32/fileio/change-journal-records
 * 
 * this is the general flow of this code:
 * 
 * _UsnpReadJournalData                     open volume, get journal data
 *   + _UsnpFormatJournalData               print the journal data
 *   + _UsnpReadJournalRecords              get usn change records
 *     + _UsnpFormatRecord                  print records based on version
 *       | _UsnpFormatRecordV2              print v2 (not implemented)
 *       | _UsnpFormatRecordV3              print v3
 *       | _UsnpFormatRecordV4              print v4 (not implemented)
 *          + _UsnpGetFilenameFromFileId    get name from a FILE_ID_128
 *          + _UsnpFormatTimestamp          format an nt timestamp
 *          + _UsnpDump                     hex-dump
 * 
 * _UsnpGetFileIdFromFilename               get fild_id_128 from filename
 * _UsnpGetFileIdFromHandle                 get fild_id_128 from handle
 * 
 * usn change records contain the file-reference-number of the file that has
 * changed, along with a parent-file-reference-number, the directory where the
 * file is/was. the openfilebyid function can be used to get the name of the
 * directory and then compare that name with locations of interest. another way
 * to filter would be to open a handle to a directory of interest, then use the
 * getfileinformationbyhandle function and then save the fileindex, then while 
 * enumerating change records, compare with the parent-file-reference-number.
 * 
 * example usn change record:
 * >>>>>>>>
 *  FRN          0000000000000000002400000009D875
 *  Parent FRN   0000000000000000000D0000000A0CD9 - \\?\C:\Users\me\AppData\Local\Temp
 *  USN          0000000498241F98
 *  Reason       80000102
 *  Attributes   00000120
 *  FileName     _CL_3817d218in
 *  TimeStamp    01D63C5D4ABBA32B - 2020-06-06 23:50:43.744
 * 
 * example getfileinformationbyhandle fileindex:
 * >>>>>>>>
 * 000D0000000A0CD9 C:\Users\me\AppData\Local\Temp
 * 
 * the lowpart of the file-reference-number is the fileindex:
 * 
 * 0000000000000000000D0000000A0CD9 - file-reference-number (FILE_ID_128)
 *                 ^^^^^^^^^^^^^^^^
 *                 000D0000000A0CD9 - fileindex
 * 
 * the _UsnpGetFileIdFromFilename and _UsnpGetFileIdFromHandle functions can be
 * used to get the file index for a file or directory.
 * 
 */

/*++
 */
BOOL
_UsnpReadJournalData (
    __in wchar_t* pathname,
    __in DWORD Reason
    );

/*++
 */
BOOL
_UsnpReadJournalRecords (
    __in HANDLE osh,
    __in USN_JOURNAL_DATA* pJournalData,
    __in USN StartUsn,
    __in DWORD Reason
    );

/*++
 */
BOOL
_UsnpFormatJournalData (
    __in wchar_t* pathname,
    __in PUSN_JOURNAL_DATA pJournalData
    );

/*++
 */
BOOL
_UsnpFormatRecord (
    __in HANDLE osh,
    __in USN_RECORD_UNION* pUsn 
    );

/*++
 */
BOOL
_UsnpFormatRecordV2 (
    __in HANDLE osh,
    __in USN_RECORD_V2* pRecord 
    );

/*++
 */
BOOL
_UsnpFormatRecordV3 (
    __in HANDLE osh,
    __in USN_RECORD_V3* pRecord 
    );

/*++
 */
BOOL
_UsnpFormatRecordV4 (
    __in HANDLE osh,
    __in USN_RECORD_V4* pRecord 
    );

/*++
 */
BOOL
_UsnpIsEqualFileReference (
    FILE_ID_128* pfid1,
    FILE_ID_128* pfid2
    );

/*++
 */
BOOL
_UsnpGetFileIdFromFilename (
    __in wchar_t* filename,
    __out FILE_ID_128* pFileId 
    );

/*++
 */
BOOL
_UsnpGetFileIdFromHandle (
    __in HANDLE osh,
    __out FILE_ID_128* pFileId 
    );

/*++
 */
BOOL
_UsnpGetFilenameFromFileId (
    __in HANDLE osh,
    __in FILE_ID_128* pFileId,
    __out_ecount(cchbuffer) wchar_t* buffer,
    __in size_t cchbuffer 
    );

/*++
 */
BOOL
_UsnpFormatTimestamp (
    __in LARGE_INTEGER* pTimeStamp,
    __out_ecount(cchbuffer) wchar_t* buffer,
    __in size_t cchbuffer 
    );

/*++
 */
BOOL
_UsnpDump (
    __in_ecount(buffersize) uint8_t* buffer,
    __in int buffersize 
    );

/*++
 */
int
__wtoi (
    __in const wchar_t* str 
    );

/*++ ... */
int g_dump = 0;
int g_count = 23;
wchar_t* argv0 = NULL;

wchar_t* monitor_dir = L"C:\\Temp\\ar\\bld\\nt-usn";
FILE_ID_128 monitor_fid = {0};

/*++
 */
int 
wmain ( 
    int argc, 
    wchar_t** argv )
{
    DWORD reason;
    wchar_t* pathname = NULL;

    UNREFERENCED_PARAMETER(argc);

    /*++ ... */
    pathname = L"C:\\";
    reason = USN_REASON_CLOSE;

    argv0 = *argv++;
    while(*argv)
    {
        wchar_t* arg = *argv++;
        if(arg[0] == L'-')
        {
            if( (arg[1] == L'd') || (arg[1] == L'D'))
            {
                g_dump++;
            }
        }
        else
        {
            g_count = __wtoi(arg);
        }
    }

    if( _UsnpGetFileIdFromFilename(monitor_dir, &monitor_fid) == FALSE)
    {
        fwprintf(stderr, L"get directory fid failed, status(%X)\n", GetLastError());
    }
    else
    {
        LARGE_INTEGER128 st_fid = {0};
        RtlMoveMemory(&st_fid, &monitor_fid, sizeof(LARGE_INTEGER128));
        fwprintf(stdout, L"monitor fid %016I64X - %s\n", st_fid.LowPart, monitor_dir);
    }

    fwprintf(stdout, L"dump(%s), count(%d)\n", ((g_dump) ? L"on" : L"off"), g_count);

    /*++
     */

    if( _UsnpReadJournalData(pathname, reason) == FALSE)
    {
        long w32error = GetLastError();
        if(w32error == ERROR_JOURNAL_NOT_ACTIVE)
        {
            fwprintf(stderr, L"journal has not been activated\n");
        }
        else
        {
            fwprintf(stderr, L"get journal data failed, status(%X)\n", w32error);
        }
    }
    return 0;
}

/*++
 */
BOOL
_UsnpReadJournalData (
    __in wchar_t* pathname,
    __in DWORD Reason )
{
    BOOL status;
    DWORD bytes = 0;
    HANDLE osh;
    wchar_t diskname[8] = L"\\\\.\\?:";
    USN_JOURNAL_DATA JournalData = {0};

    if(pathname == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    diskname[4] = *pathname;

    osh = CreateFileW(
     diskname,
     (GENERIC_READ | GENERIC_WRITE),
     (FILE_SHARE_READ | FILE_SHARE_WRITE),
     NULL,
     OPEN_EXISTING,
     0,
     NULL
     );

    if(osh == INVALID_HANDLE_VALUE)
    {
        /*++ last error set by call ... */
        fwprintf(stderr, L"open failed, status(%X)\n", GetLastError());
        return FALSE;
    }

    /*++
     * this code expects to build against _NTDDI_WIN8 and above because there
     * are three versions of USN_JOURNAL_DATA,
     * 
     *   USN_JOURNAL_DATA_V0
     *   USN_JOURNAL_DATA_V1
     *   USN_JOURNAL_DATA_V2
     * 
     * the V1 and V2 structures both contain supported usn versions,
     * 
     *   MinSupportedMajorVersion
     *   MaxSupportedMajorVersion
     *
     * these values are needed when getting the usn records.
     */

    status = DeviceIoControl(
     osh,
     FSCTL_QUERY_USN_JOURNAL,
     NULL,
     0,
     &JournalData,
     sizeof(USN_JOURNAL_DATA),
     &bytes,
     NULL
     );
                            
    if(status == FALSE)
    {
        /*++ last error set by call. might be ERROR_JOURNAL_NOT_ACTIVE ... */
        fwprintf(stderr, L"ioctl failed, status(%X)\n", GetLastError());
        return FALSE;
    }

    status = _UsnpFormatJournalData(diskname, &JournalData);
    if(status == FALSE)
    {
        /*++ last error set by call ... */
        fwprintf(stderr, L"format journal data failed, status(%X)\n", GetLastError());
        CloseHandle(osh);
        return FALSE;
    }

    /*++ 
     * using start usn of zero; always start at beginning ... a useful thing to
     * do would be to save the last usn read for a given run and then use that
     * value as the starting point of a subsequent run ...
     */
    status = _UsnpReadJournalRecords(osh, &JournalData, 0LL, Reason);
    if(status == FALSE)
    {
        /*++ last error set by call ... */
        fwprintf(stderr, L"read journal records failed, status(%X)\n", GetLastError());
        CloseHandle(osh);
        return FALSE;
    }

    CloseHandle(osh);
    return TRUE;
}

/*++
 */
BOOL
_UsnpReadJournalRecords (
    __in HANDLE osh,
    __in USN_JOURNAL_DATA* pJournalData,
    __in USN StartUsn,
    __in DWORD Reason )
{
    int count = 0;
    BOOL status;
    char buffer[_USN_BUFFER_SIZE] = {0};
    DWORD bytes = 0;
    PUSN_RECORD_COMMON_HEADER pRecord = NULL;
    READ_USN_JOURNAL_DATA ReadData = {0};

    /*++ check ptr ... */
    if(pJournalData == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ReadData.UsnJournalID    = pJournalData->UsnJournalID;
    ReadData.StartUsn        = StartUsn;
    ReadData.ReasonMask      = Reason;

    /*++ these values require NTDDI_WIN8 and above ... */
    ReadData.MinMajorVersion = pJournalData->MinSupportedMajorVersion;
    ReadData.MaxMajorVersion = pJournalData->MaxSupportedMajorVersion;

    while(1)
    {
        RtlZeroMemory(buffer, sizeof(buffer));

        status = DeviceIoControl (    
         osh,
         FSCTL_READ_USN_JOURNAL,
         &ReadData,
         sizeof(ReadData),
         buffer,
         sizeof(buffer),
         &bytes,
         NULL
         );

        if(status == FALSE)
        {
            /*++ last error set by call ... */
            return FALSE;
        }

        bytes -= sizeof(USN);

        /*++ 
         * the returned buffer starts with the next usn after those in
         * the buffer. after looping on this buffer, set the startusn
         * to the value at the beginning of this buffer and ask more ...
         */
        pRecord = (PUSN_RECORD_COMMON_HEADER)(((uint8_t*)buffer) + sizeof(USN));

        while(bytes > 0)
        {
            if( _UsnpFormatRecord(osh, (USN_RECORD_UNION*)pRecord) == FALSE)
            {
                fwprintf(stderr, L"format usn record failed, status(%X)\n", GetLastError());
                /*++return FALSE;*/
            }

            /*++LIMITLIMIT: ... */
            if(count++ > g_count)
            {
                SetLastError(ERROR_IMPLEMENTATION_LIMIT);
                return TRUE;
            }
            /*++LIMITLIMIT: ... */

            bytes -= pRecord->RecordLength;

            pRecord = (PUSN_RECORD_COMMON_HEADER)(((uint8_t*)pRecord) + pRecord->RecordLength);
        }
        /*++ get the next starting usn ... */
        ReadData.StartUsn = *(USN*)&buffer;
    }
    return TRUE;
}

/*++
 */
BOOL
_UsnpFormatJournalData (
    __in wchar_t* pathname,
    __in PUSN_JOURNAL_DATA pJournalData )
{
    if((pathname == NULL) || (pJournalData == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    fwprintf(stdout,
     L"JOURNAL DATA PATH(%s)\n"
     L"  UsnJournalID        %016I64X\n"
     L"  FirstUsn            %016I64X\n"
     L"  NextUsn             %016I64X\n"
     L"  LowestValidUsn      %016I64X\n"
     L"  MaxUsn              %016I64X\n"
     L"  MaximumSize         %016I64X\n"
     L"  AllocationDelta     %016I64X\n"
     L"  Supported Versions  %u.x - %u.x\n",
     pathname,
     pJournalData->UsnJournalID,
     pJournalData->FirstUsn,
     pJournalData->NextUsn,
     pJournalData->LowestValidUsn,
     pJournalData->MaxUsn,
     pJournalData->MaximumSize,
     pJournalData->AllocationDelta,
     pJournalData->MinSupportedMajorVersion,
     pJournalData->MaxSupportedMajorVersion
     );
    return TRUE;
}

/*++
 */
BOOL
_UsnpFormatRecord (
    __in HANDLE osh,
    __in USN_RECORD_UNION* pRecord )
{
    /*++ check ptr ... */
    if(pRecord == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch(pRecord->Header.MajorVersion)
    {
    case 2: return _UsnpFormatRecordV2(osh, &(pRecord->V2));
    case 3: return _UsnpFormatRecordV3(osh, &(pRecord->V3));
    case 4: return _UsnpFormatRecordV4(osh, &(pRecord->V4));
    }

    SetLastError(ERROR_INVALID_DATA);
    return FALSE;
}

/*++
 */
BOOL
_UsnpFormatRecordV2 (
    __in HANDLE osh,
    __in USN_RECORD_V2* pRecord )
{
    /*++ check ptr ... */
    if(pRecord == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    UNREFERENCED_PARAMETER(osh);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*++
 */
BOOL
_UsnpFormatRecordV3 (
    __in HANDLE osh,
    __in USN_RECORD_V3* pRecord )
{
    wchar_t buffer[MAX_PATH] = {0};
    size_t cchbuffer;
    wchar_t timestamp[MAX_PATH] = {0};
    size_t cchtimestamp;
    wchar_t* filename = NULL;
    WORD cchfilename = 0;
    ULARGE_INTEGER128* refnum = NULL;
    ULARGE_INTEGER128* parent = NULL;

    /*++ check ptr ... */
    if(pRecord == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /*++
     * filtering could be done here using the file index of the parent ...
     */
    if( _UsnpIsEqualFileReference(&(pRecord->ParentFileReferenceNumber), &monitor_fid) == FALSE)
    {
        /*++ fid's don't match ... */
    }

    refnum = (ULARGE_INTEGER128*)&(pRecord->FileReferenceNumber);
    parent = (ULARGE_INTEGER128*)&(pRecord->ParentFileReferenceNumber);

    filename = (wchar_t*)((char*)pRecord + pRecord->FileNameOffset);
    cchfilename = (pRecord->FileNameLength / sizeof(wchar_t));

    cchtimestamp = _countof(timestamp);
    if( _UsnpFormatTimestamp(&(pRecord->TimeStamp), timestamp, cchtimestamp) == FALSE)
    {
        _snwprintf_s(timestamp, cchtimestamp, cchtimestamp, L"[error(%X)]", GetLastError());
    }

    cchbuffer = _countof(buffer);
    if( _UsnpGetFilenameFromFileId(osh, &(pRecord->ParentFileReferenceNumber), buffer, cchbuffer) == FALSE)
    {
        _snwprintf_s(buffer, cchbuffer, cchbuffer, L"[error(%X)]", GetLastError());
    }

    wprintf (
     L">>>>>>>>\n"
     L"  FRN                 %016I64X%016I64X\n"
     L"  Parent FRN          %016I64X%016I64X - %s\n"
     L"  USN                 %016I64X\n"
     L"  Reason              %08X\n"
     L"  Attributes          %08X\n"
     L"  FileName            %.*s\n"
     L"  TimeStamp           %016I64X - %s\n",
     refnum->HighPart, refnum->LowPart,
     parent->HighPart, parent->LowPart,
     buffer,
     pRecord->Usn,
     pRecord->Reason,
     pRecord->FileAttributes,
     cchfilename,
     filename,
     pRecord->TimeStamp.QuadPart,
     timestamp
     );

    if(g_dump)
    {
        /*++ hex-dump record, if desired ... */
        _UsnpDump((uint8_t*)pRecord, pRecord->RecordLength);
    }

    return TRUE;
}

/*++
 */
BOOL
_UsnpFormatRecordV4 (
    __in HANDLE osh,
    __in USN_RECORD_V4* pRecord )
{
    /*++ check ptr ... */
    if(pRecord == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    UNREFERENCED_PARAMETER(osh);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*++
 */
BOOL
_UsnpIsEqualFileReference (
    FILE_ID_128* pfid1,
    FILE_ID_128* pfid2 )
{
    if((pfid1 == NULL) || (pfid2 == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if(memcmp((void*)pfid1, (void*)pfid2, sizeof(FILE_ID_128)) == 0)
    {
        return TRUE;
    }
    return FALSE;
}

/*++
 */
BOOL
_UsnpGetFileIdFromFilename (
    __in wchar_t* filename,
    __out FILE_ID_128* pFileId )
{
    BOOL status = FALSE;
    HANDLE osfh = NULL;

    if((filename == NULL) || (pFileId == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    osfh = CreateFileW(
     filename, 
     (GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES),
     (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE),
     NULL, 
     OPEN_EXISTING, 
     (FILE_FLAG_NO_BUFFERING | FILE_FLAG_BACKUP_SEMANTICS),
     NULL
     );

    if(osfh != NULL)
    {
        status = _UsnpGetFileIdFromHandle(osfh, pFileId);
        CloseHandle(osfh);
    }
    return status;
}

/*++
 */
BOOL
_UsnpGetFileIdFromHandle (
    __in HANDLE osfh,
    __out FILE_ID_128* pFileId )
{
    BY_HANDLE_FILE_INFORMATION FileInfo = {0};

    if((osfh == NULL) || (pFileId == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if( GetFileInformationByHandle(osfh, &FileInfo) == FALSE)
    {
        /*++ last error set by call ... */
        return FALSE;
    }

    LARGE_INTEGER st_index = {0};
    st_index.LowPart = FileInfo.nFileIndexLow;
    st_index.HighPart = FileInfo.nFileIndexHigh;

    LARGE_INTEGER128 st_fid = {0};
    st_fid.LowPart = st_index.QuadPart;

    RtlMoveMemory(pFileId, &st_fid, sizeof(LARGE_INTEGER128));

    return TRUE;
}

/*++
 */
BOOL
_UsnpGetFilenameFromFileId (
    __in HANDLE osh,
    __in FILE_ID_128* pFileId,
    __out_ecount(cchbuffer) wchar_t* buffer,
    __in size_t cchbuffer )
{
    DWORD length;
    HANDLE osfh;
    FILE_ID_DESCRIPTOR id = {0};

    /*++ check ptr and buffer ... */
    if((pFileId == NULL) || (buffer == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    id.dwSize = sizeof(id);
    id.Type = ExtendedFileIdType;
    RtlMoveMemory(&(id.ExtendedFileId), pFileId, sizeof(FILE_ID_128));

    osfh = OpenFileById (
     osh,
     &id,
     0,
     (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE),
     NULL,
     (FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT)
     );

    if(osfh == NULL)
    {
        /*++ last error set by call ... */
        return FALSE;
    }

    length = GetFinalPathNameByHandleW(osfh, buffer, (DWORD)cchbuffer, (FILE_NAME_NORMALIZED | VOLUME_NAME_DOS));

    CloseHandle(osfh);
    
    if((length == 0) || (length > cchbuffer))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    return TRUE;
}

/*++
 */
BOOL
_UsnpFormatTimestamp (
    __in LARGE_INTEGER* pTimeStamp,
    __out_ecount(cchbuffer) wchar_t* buffer,
    __in size_t cchbuffer )
{
    int length;
    FILETIME rectime = {0};
    SYSTEMTIME systime = {0};

    /*++
     * nt timestamps are 100-nanosecond intervals since 1601-01-01. this
     * code formats such a time in the typical way. if desired, the time
     * can be converted to a unix epoch and formatted using a c-runtime
     * function such as ctime() ...
     */

    if((pTimeStamp == NULL) || (buffer == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    rectime.dwLowDateTime  = pTimeStamp->LowPart;
    rectime.dwHighDateTime = pTimeStamp->HighPart;

    if( FileTimeToSystemTime(&rectime, &systime) == FALSE)
    {
        /*++ last error set by call ... */
        return FALSE;
    }

    length = _snwprintf_s (
     buffer,
     cchbuffer,
     cchbuffer,
     L"%u-%02u-%02u %02u:%02u:%02u.%03u",
     systime.wYear, systime.wMonth, systime.wDay,
     systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds
     );

    if((size_t)length >= cchbuffer)
    {
        /*++ the call null-term's at buf[cch-1] ... */
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    return TRUE;
}

/*++
 */
BOOL
_UsnpDump (
    __in_ecount(buffersize) uint8_t* buffer,
    __in int buffersize )
{
    if(buffer == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    fwprintf(stdout, L"DUMP %d BYTES AT =A'%p\n", buffersize, buffer);
    fwprintf(stdout, L"ADDRESS   OFFSET     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F | 0123456789ABCDEF |\n");

    for(int index=0; index<buffersize; index += 16)
    {
        fwprintf(stdout, L"%p  %08X  ", (buffer + index), index);
        for(int jndex=0; jndex<16; jndex++)
        {
            if((index + jndex) < buffersize)
            {
                fwprintf(stdout, L"%02X ", (uint8_t)buffer[(index + jndex)]);
            }
            else
            {
                fwprintf(stdout, L"%*C ", (int)(sizeof(uint8_t) * 2), ' ');
            }
        }

        fwprintf(stdout, L"| ");
        for(int kndex=0; kndex<16; kndex++ )
        {
            uint8_t ch = buffer[(index + kndex)];
            if((index + kndex) < buffersize)
            {
                fwprintf(stdout, L"%C", (((ch < 0x20) || (ch > 0x7F)) ? '.' : ch));
            }
            else
            {
                fwprintf(stdout, L" ");
            }
        }
        fwprintf(stdout, L" |\n");
    }
    return TRUE;
}

/*++
 */
int
__wtoi (
    __in const wchar_t* str )
{
    int n = 0;
    int signum = 0;
    for(;; str++)
    {
        switch(*str)
        {
        case L' ':
        case L'\t': continue;
        case L'-':  signum++;
        case L'+':  str++;
        }
        break;
    }
    while((*str >= L'0') && (*str <= L'9'))
    {
        n = n * 10 + *str++ - L'0';
    }
    return ((signum != 0) ? -n : n);
}
