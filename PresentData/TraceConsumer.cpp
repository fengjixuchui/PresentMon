/*
Copyright 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <windows.h>
#include <tdh.h> // must include after windows.h
#include <assert.h>

#include "TraceConsumer.hpp"

namespace {

void PrintIndent(FILE* fp, uint32_t indent)
{
    for (uint32_t i = 0; i < indent; ++i) {
        fprintf(fp, "    ");
    }
}

char const* InType(USHORT intype) {
#define RETURN_INTYPE(_Type) if (intype == TDH_INTYPE_##_Type) return #_Type
    RETURN_INTYPE(NULL);
    RETURN_INTYPE(UNICODESTRING);
    RETURN_INTYPE(ANSISTRING);
    RETURN_INTYPE(INT8);
    RETURN_INTYPE(UINT8);
    RETURN_INTYPE(INT16);
    RETURN_INTYPE(UINT16);
    RETURN_INTYPE(INT32);
    RETURN_INTYPE(UINT32);
    RETURN_INTYPE(INT64);
    RETURN_INTYPE(UINT64);
    RETURN_INTYPE(FLOAT);
    RETURN_INTYPE(DOUBLE);
    RETURN_INTYPE(BOOLEAN);
    RETURN_INTYPE(BINARY);
    RETURN_INTYPE(GUID);
    RETURN_INTYPE(POINTER);
    RETURN_INTYPE(FILETIME);
    RETURN_INTYPE(SYSTEMTIME);
    RETURN_INTYPE(SID);
    RETURN_INTYPE(HEXINT32);
    RETURN_INTYPE(HEXINT64);
    return "Unknown intype";
}

char const* OutType(USHORT outtype) {
#define RETURN_OUTTYPE(_Type) if (outtype == TDH_OUTTYPE_##_Type) return #_Type
    RETURN_OUTTYPE(NULL);
    RETURN_OUTTYPE(STRING);
    RETURN_OUTTYPE(DATETIME);
    RETURN_OUTTYPE(BYTE);
    RETURN_OUTTYPE(UNSIGNEDBYTE);
    RETURN_OUTTYPE(SHORT);
    RETURN_OUTTYPE(UNSIGNEDSHORT);
    RETURN_OUTTYPE(INT);
    RETURN_OUTTYPE(UNSIGNEDINT);
    RETURN_OUTTYPE(LONG);
    RETURN_OUTTYPE(UNSIGNEDLONG);
    RETURN_OUTTYPE(FLOAT);
    RETURN_OUTTYPE(DOUBLE);
    RETURN_OUTTYPE(BOOLEAN);
    RETURN_OUTTYPE(GUID);
    RETURN_OUTTYPE(HEXBINARY);
    RETURN_OUTTYPE(HEXINT8);
    RETURN_OUTTYPE(HEXINT16);
    RETURN_OUTTYPE(HEXINT32);
    RETURN_OUTTYPE(HEXINT64);
    RETURN_OUTTYPE(PID);
    RETURN_OUTTYPE(TID);
    RETURN_OUTTYPE(PORT);
    RETURN_OUTTYPE(IPV4);
    RETURN_OUTTYPE(IPV6);
    RETURN_OUTTYPE(SOCKETADDRESS);
    RETURN_OUTTYPE(CIMDATETIME);
    RETURN_OUTTYPE(ETWTIME);
    RETURN_OUTTYPE(XML);
    RETURN_OUTTYPE(ERRORCODE);
    RETURN_OUTTYPE(WIN32ERROR);
    RETURN_OUTTYPE(NTSTATUS);
    RETURN_OUTTYPE(HRESULT);
    RETURN_OUTTYPE(CULTURE_INSENSITIVE_DATETIME);
    RETURN_OUTTYPE(JSON);
  //RETURN_OUTTYPE(UTF8);
 // RETURN_OUTTYPE(PKCS7_WITH_TYPE_INFO);
    return "Unknown outtype";
}

void PrintEventProperty(FILE* fp, uintptr_t bufferAddr, EVENT_PROPERTY_INFO const& prop, uint32_t indent)
{
    PrintIndent(fp, indent); fprintf(fp, "%ls\n", (wchar_t*)(bufferAddr + prop.NameOffset));
    ++indent;

  //PrintIndent(fp, indent); fprintf(fp, "Flags    = %08x\n", prop.Flags);
    if (prop.Flags & PropertyStruct) {
        for (ULONG i = 0; i < prop.structType.NumOfStructMembers; ++i) {
            auto info = (TRACE_EVENT_INFO*) bufferAddr;
            PrintEventProperty(fp, bufferAddr, info->EventPropertyInfoArray[prop.structType.StructStartIndex + i], indent);
        }
    } else {
      //PrintIndent(fp, indent); fprintf(fp, "Map     = %ls\n", (wchar_t*)(bufferAddr + prop.nonStructType.MapNameOffset));
        PrintIndent(fp, indent); fprintf(fp, "%s -> %s\n",
            InType(prop.nonStructType.InType),
            OutType(prop.nonStructType.OutType));
    }
}

}

void PrintEventInformationFromTdh(FILE* fp, EVENT_RECORD* pEventRecord)
{
    ULONG bufferSize = 0;
    auto status = TdhGetEventInformation(pEventRecord, 0, nullptr, nullptr, &bufferSize);
    if (status == ERROR_INSUFFICIENT_BUFFER) {
        auto bufferAddr = (uintptr_t) malloc(bufferSize);

        auto info = (TRACE_EVENT_INFO*) bufferAddr;
        status = TdhGetEventInformation(pEventRecord, 0, nullptr, info, &bufferSize);
        if (status == ERROR_SUCCESS) {
            fprintf(fp, "%ls::%ls::%ls\n",
                (wchar_t*)(bufferAddr + info->ProviderNameOffset),
                (wchar_t*)(bufferAddr + info->TaskNameOffset),
                (wchar_t*)(bufferAddr + info->OpcodeNameOffset));
          //printf("Keywords = %ls\n\n", (wchar_t*)(bufferAddr + info->KeywordsNameOffset));
          //printf("TopLevelPropertyCount = %u\n", info->TopLevelPropertyCount);
          //printf("PropertyCount = %u\n", info->PropertyCount);
            for (ULONG i = 0, N = info->TopLevelPropertyCount; i < N; ++i) {
                PrintEventProperty(fp, bufferAddr, info->EventPropertyInfoArray[i], 1);
            }
        }

        free((void*) bufferAddr);
    }
}

std::wstring GetEventTaskNameFromTdh(EVENT_RECORD* pEventRecord)
{
    std::wstring taskName = L"";
    ULONG bufferSize = 0;
    auto status = TdhGetEventInformation(pEventRecord, 0, nullptr, nullptr, &bufferSize);
    if (status == ERROR_INSUFFICIENT_BUFFER) {
        auto bufferAddr = (uintptr_t)malloc(bufferSize);

        auto info = (TRACE_EVENT_INFO*)bufferAddr;
        status = TdhGetEventInformation(pEventRecord, 0, nullptr, info, &bufferSize);
        if (status == ERROR_SUCCESS) {
            taskName = (wchar_t*)(bufferAddr + info->TaskNameOffset);
        }

        free((void*)bufferAddr);
    }

    return taskName;
}

template <>
bool GetEventDataFromTdh<std::string>(EVENT_RECORD* pEventRecord, wchar_t const* name, std::string* out, bool bPrintOnError)
{
    PROPERTY_DATA_DESCRIPTOR descriptor;
    descriptor.PropertyName = (ULONGLONG) name;
    descriptor.ArrayIndex = ULONG_MAX;

    ULONG propertySize = 0;
    auto status = TdhGetPropertySize(pEventRecord, 0, nullptr, 1, &descriptor, &propertySize);
    if (status != ERROR_SUCCESS) {
        if (bPrintOnError) {
            fprintf(stderr, "error: could not get event %ls property's size (error=%u).\n", name, status);
        }
        return false;
    }

    out->resize(propertySize);
    auto buffer = (BYTE*) out->data();
    status = TdhGetProperty(pEventRecord, 0, nullptr, 1, &descriptor, propertySize, buffer);
    if (status != ERROR_SUCCESS) {
        if (bPrintOnError) {
            fprintf(stderr, "error: could not get event %ls property (error=%u).\n", name, status);
        }
        return false;
    }

    return true;
}

void EventMetadataContainer::InsertMetadata(GUID const& Provider, EVENT_DESCRIPTOR const& EventDescriptor, TRACE_EVENT_INFO const* pInfo, SIZE_T TeiSize)
{
    auto& spCachedInfo = mMetadata[Provider][EventDescriptor];
    if (!spCachedInfo)
    {
        spCachedInfo.reset(new byte[TeiSize]);
        memcpy(spCachedInfo.get(), pInfo, TeiSize);
    }
}

static void GetPropertyByIndex(TRACE_EVENT_INFO const& traceEventInfo, EVENT_RECORD const& eventRecord, uint32_t index,
                               EVENT_PROPERTY_INFO const** outPropInfo, uint32_t* outOffset);

static void GetPropertySize(TRACE_EVENT_INFO const& traceEventInfo, EVENT_RECORD const& eventRecord, EVENT_PROPERTY_INFO const& propInfo,
                            uint32_t* outSize, uint32_t* outCount)
{
    // TODO: We don't handle nested structs or variable data lengths yet
    assert((propInfo.Flags & (PropertyParamLength | PropertyStruct)) == 0);

    uint32_t size = propInfo.length;

    // TODO: Not sure this is needed, propInfo.length seems to be correct?
    if (propInfo.nonStructType.InType == TDH_INTYPE_SIZET ||
        propInfo.nonStructType.InType == TDH_INTYPE_POINTER)
    {
        if (eventRecord.EventHeader.Flags & EVENT_HEADER_FLAG_64_BIT_HEADER)
        {
            size = 8;
        }
        else if (eventRecord.EventHeader.Flags & EVENT_HEADER_FLAG_32_BIT_HEADER)
        {
            size = 4;
        }
    }

    uint32_t count = propInfo.count;
    if (propInfo.Flags & PropertyParamCount)
    {
        EVENT_PROPERTY_INFO const* countPropInfo = nullptr;
        uint32_t countPropOffset = 0;
        GetPropertyByIndex(traceEventInfo, eventRecord, count, &countPropInfo, &countPropOffset);

        assert(countPropInfo->Flags == 0);
        assert(countPropInfo->nonStructType.InType == TDH_INTYPE_UINT32); // TODO: handle other int types
        count = *(uint32_t const*) ((uintptr_t) eventRecord.UserData + countPropOffset);
    }

    *outSize = size;
    *outCount = count;
}

static void GetPropertyByIndex(TRACE_EVENT_INFO const& traceEventInfo, EVENT_RECORD const& eventRecord, uint32_t index,
                               EVENT_PROPERTY_INFO const** outPropInfo, uint32_t* outOffset)
{
    assert(index < traceEventInfo.TopLevelPropertyCount);

    uint32_t offset = 0;
    for (uint32_t i = 0;; ++i)
    {
        auto const& propInfo = traceEventInfo.EventPropertyInfoArray[i];

        if (i == index)
        {
            *outPropInfo = &propInfo;
            *outOffset = offset;
            break;
        }

        uint32_t size = 0;
        uint32_t count = 0;
        GetPropertySize(traceEventInfo, eventRecord, propInfo, &size, &count);
        offset += size * count;
    }
}

const void* EventMetadataContainer::GetEventDataImpl(EVENT_RECORD* pEventRecord, wchar_t const* name, SIZE_T* pSize)
{
    auto ProviderIter = mMetadata.find(pEventRecord->EventHeader.ProviderId);
    if (ProviderIter == mMetadata.end())
        return nullptr;

    auto EventIter = ProviderIter->second.find(pEventRecord->EventHeader.EventDescriptor);
    if (EventIter == ProviderIter->second.end())
        return nullptr;

    auto& tei = *reinterpret_cast<TRACE_EVENT_INFO*>(EventIter->second.get());
    uint32_t DataOffset = 0;
    for (uint32_t i = 0; i < tei.TopLevelPropertyCount; ++i)
    {
        auto& prop = tei.EventPropertyInfoArray[i];

        uint32_t size = 0;
        uint32_t count = 0;
        GetPropertySize(tei, *pEventRecord, prop, &size, &count);
        size = size * count;

        if (wcscmp(TEI_PROPERTY_NAME(&tei, &prop), name) == 0)
        {
            *pSize = size;
            return reinterpret_cast<BYTE*>(pEventRecord->UserData) + DataOffset;
        }

        DataOffset += size;
    }
    return nullptr;
}

const void* EventMetadataContainer::GetEventDataFromArrayImpl(EVENT_RECORD* pEventRecord, wchar_t const* name, uint32_t index, SIZE_T* pSize)
{
    auto ProviderIter = mMetadata.find(pEventRecord->EventHeader.ProviderId);
    if (ProviderIter == mMetadata.end())
        return nullptr;

    auto EventIter = ProviderIter->second.find(pEventRecord->EventHeader.EventDescriptor);
    if (EventIter == ProviderIter->second.end())
        return nullptr;

    auto& tei = *reinterpret_cast<TRACE_EVENT_INFO*>(EventIter->second.get());
    uint32_t DataOffset = 0;
    for (uint32_t i = 0; i < tei.TopLevelPropertyCount; ++i)
    {
        auto& prop = tei.EventPropertyInfoArray[i];

        uint32_t size = 0;
        uint32_t count = 0;
        GetPropertySize(tei, *pEventRecord, prop, &size, &count);

        if (wcscmp(TEI_PROPERTY_NAME(&tei, &prop), name) == 0)
        {
            assert(index < count);
            *pSize = size;
            return reinterpret_cast<BYTE*>(pEventRecord->UserData) + DataOffset + index * size;
        }

        DataOffset += size * count;
    }
    return nullptr;
}

template <> bool EventMetadataContainer::GetEventData<std::string>(EVENT_RECORD* pEventRecord, wchar_t const* name, std::string* out)
{
    SIZE_T Size = 0;
    const void* pData = GetEventDataImpl(pEventRecord, name, &Size);
    if (pData != nullptr && Size > 0)
    {
        out->resize(Size + 1);
        memcpy((BYTE*)out->data(), pData, Size);
        return true;
    }
    return GetEventDataFromTdh(pEventRecord, name, out);
}
