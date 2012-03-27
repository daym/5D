/*
5D programming language
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "stdafx.h"
#include <objbase.h>
#include <oleauto.h>
#include "FFIs/RecordPacker"
#include "Numbers/Small"
#include "Numbers/BigUnsigned"
#include "Numbers/Integer"
#include "Evaluators/Evaluators"
#include "Evaluators/FFI"
#include "Evaluators/Builtins"
#include "FFIs/COMWrapper"
#include "Numbers/Ratio"
#include "FFIs/VariantPacker"

//typedef short uint16_t;
// TODO support different endiannesses.
#ifdef WIN32
#include <OaIdl.h> /* VARIANT */
#else
typedef struct  tagCY {
    unsigned long      Lo;
    long               Hi;
} CY;
typedef ULONG SCODE; 
typedef struct  tagDEC {
    USHORT             wReserved;
    BYTE               scale;
    BYTE               sign;
    ULONG              Hi32;
    ULONGLONG          Lo64;
} DECIMAL;
 
typedef struct  tagSAFEARRAYBOUND {
    ULONG              cElements;
    LONG               lLbound;
} SAFEARRAYBOUND;
 
typedef struct  tagSAFEARRAY {
    USHORT             cDims;
    USHORT             fFeatures; /* can be a combination of:
    FADF_AUTO 0x0001 An array that is allocated on the stack.
    FADF_STATIC 0x0002 An array that is statically allocated.
    FADF_EMBEDDED 0x0004 An array that is embedded in a structure.
    FADF_FIXEDSIZE 0x0010 An array that may not be resized or reallocated.
    FADF_RECORD 0x0020 An array that contains records. When set, there will be a pointer to the IRecordInfo interface at negative offset 4 in the array descriptor.
    FADF_HAVEIID 0x0040 An array that has an IID identifying interface. When set, there will be a GUID at negative offset 16 in the safe array descriptor. Flag is set only when FADF_DISPATCH or FADF_UNKNOWN is also set.
    FADF_HAVEVARTYPE 0x0080 An array that has an IID identifying interface. When set, there will be a GUID at negative offset 16 in the safe array descriptor. Flag is set only when FADF_DISPATCH or FADF_UNKNOWN is also set.
    FADF_BSTR 0x0100 An array of BSTRs.
    FADF_UNKNOWN 0x0200 An array of IUnknown*.
    FADF_DISPATCH 0x0400 An array of IDispatch*.
    FADF_VARIANT 0x0800 An array of VARIANTs.
    FADF_RESERVED 0xF008 Bits reserved for future use. 
*/
    ULONG              cbElements; /* size of one array element */
    ULONG              cLocks;
    PVOID              pvData;
    SAFEARRAYBOUND     rgsabound[1];
} SAFEARRAY;
typedef CY             CURRENCY;
typedef short          VARIANT_BOOL;
typedef unsigned short VARTYPE;
typedef double         DATE;
typedef OLECHAR*       BSTR;
/* see http://spec.winprog.org/typeinf2/ */
typedef  enum tagVARENUM
{
  VT_EMPTY = 0x0000,
  VT_NULL = 0x0001,
  VT_I2 = 0x0002,
  VT_I4 = 0x0003,
  VT_R4 = 0x0004,
  VT_R8 = 0x0005,
  VT_CY = 0x0006,
  VT_DATE = 0x0007,
  VT_BSTR = 0x0008,
  VT_DISPATCH = 0x0009,
  VT_ERROR = 0x000A,
  VT_BOOL = 0x000B,
  VT_VARIANT = 0x000C,
  VT_UNKNOWN = 0x000D,
  VT_DECIMAL = 0x000E,
  VT_I1 = 0x0010,
  VT_UI1 = 0x0011,
  VT_UI2 = 0x0012,
  VT_UI4 = 0x0013,
  VT_I8 = 0x0014, // not for VARIANT
  VT_UI8 = 0x0015,  // not for VARIANT
  VT_INT = 0x0016,
  VT_UINT = 0x0017,
  VT_VOID = 0x0018, // not for VARIANT
  VT_HRESULT = 0x0019, // not for VARIANT
  VT_PTR = 0x001A, // not for VARIANT
  VT_SAFEARRAY = 0x001B, // not for VARIANT
  VT_CARRAY = 0x001C, // not for VARIANT
  VT_USERDEFINED = 0x001D, // not for VARIANT
  VT_LPSTR = 0x001E, // only for PROPVARIANT
  VT_LPWSTR = 0x001F, // only for PROPVARIANT
  VT_RECORD = 0x0024,
  VT_INT_PTR = 0x0025,
  VT_UINT_PTR = 0x0026,
  VT_ARRAY = 0x2000,
  VT_BYREF = 0x4000
} VARENUM;
typedef struct tagVARIANT {
  union {
    struct __tagVARIANT {
      VARTYPE vt;
      WORD    wReserved1;
      WORD    wReserved2;
      WORD    wReserved3;
      union {
        LONGLONG            llVal;
        LONG                lVal; // VT_I4  long integer
        BYTE                bVal;
        SHORT               iVal; // VT_I2  short integer
        FLOAT               fltVal; // VT_R4  4-byte float
        DOUBLE              dblVal; // VT_R8  8-byte float
        VARIANT_BOOL        boolVal;
        //_VARIANT_BOOL       bool_;
        SCODE               scode; // VT_ERROR
        CY                  cyVal; // VT_CY
        DATE                date; // VT_DATE
        BSTR                bstrVal; // VT_BSTR
        IUnknown            *punkVal; // VT_UNKNOWN
        IDispatch           *pdispVal; // VT_DISPATCH
        SAFEARRAY           *parray; // VT_ARRAY, see SafeArrayCreateVector, SafeArrayPutElement
        BYTE                *pbVal; 
        SHORT               *piVal;
        LONG                *plVal;
        LONGLONG            *pllVal;
        FLOAT               *pfltVal;
        DOUBLE              *pdblVal;
        VARIANT_BOOL        *pboolVal;
        _VARIANT_BOOL       *pbool;
        SCODE               *pscode;
        CY                  *pcyVal;
        DATE                *pdate;
        BSTR                *pbstrVal;
        IUnknown            **ppunkVal;
        IDispatch           **ppdispVal;
        SAFEARRAY           **pparray;
        VARIANT             *pvarVal;
        PVOID               byref;
        CHAR                cVal; // VT_I1
        USHORT              uiVal; // VT_UI2
        ULONG               ulVal; // VT_UI4
        ULONGLONG           ullVal; // VT_UI8
        INT                 intVal; // VT_INT
        UINT                uintVal; // VT_UINT
        DECIMAL             *pdecVal; // VT_BYREF|VT_DECIMAL
        CHAR                *pcVal;
        USHORT              *puiVal;
        ULONG               *pulVal;
        ULONGLONG           *pullVal;
        INT                 *pintVal;
        UINT                *puintVal;
        struct __tagBRECORD {
          PVOID       pvRecord;
          IRecordInfo *pRecInfo;
        } __VARIANT_NAME_4;
      } __VARIANT_NAME_3;
    } __VARIANT_NAME_2;
    DECIMAL             decVal;
  } __VARIANT_NAME_1;
} VARIANT;
#endif
namespace FFIs {
using namespace Numbers;

/*static SAFEARRAY* marshalCArray(AST::Node* source) {
	SAFEARRAY* result = SafeArrayCreateVector(VT_BSTR, 0, (unsigned int) myPaths.size() );
	return(result);
}
static AST::Node* demarshalCArray(SAFEARRAY* arr) {
	return(NULL);
}*/
static SAFEARRAY* marshalSafeArray(VARTYPE itemType, AST::Node* source) {
	// FIXME
	return(NULL);
}
static AST::Node* demarshalSafeArray(VARTYPE itemType, SAFEARRAY* arr) {
	long lBound, uBound;
	/*
	VariantInit
	HRESULT SafeArrayGetDim(SAFEARRAY FAR* psa);
	HRESULT SafeArrayGetLBound(SAFEARRAY FAR* psa, unsigned int nDim, long FAR* plLbound);
	HRESULT SafeArrayGetUBound(SAFEARRAY FAR* psa, unsigned int nDim, long FAR* plUbound);
	HRESULT SafeArrayCreate(VARTYPE vt, unsigned int cDims, SAFEARRRAYBOUND FAR* rgsabound); 
	HRESULT SafeArrayDestroy(SAFEARRAY FAR* psa); 
	HRESULT SafeArrayAccessData(SAFEARRAY FAR* psa, void HUGEP* FAR* ppvData); 
	HRESULT SafeArrayUnaccessData(SAFEARRAY FAR* psa);
	SafeArrayCreateVector(itemType, loindex, hiindex);
	long i;
	SafeArrayPutElement(parray, &i, element);
	*/
	//SafeArrayGetDim()
	IUnknown** rawArray;
	SafeArrayAccessData(arr, (void**)&rawArray);
	for(USHORT iDim = 1; iDim <= arr->cDims; ++iDim) {
		SafeArrayGetLBound(arr, 1, &lBound);
		SafeArrayGetUBound(arr, 1, &uBound);
		long numElems = uBound - lBound + 1;
	}

	/*for (long i = 0; i < numElems; ++i) {
	  IUnknown* pElem = rawArray[i];
	  DoSomething(pElem);
	}*/
	SafeArrayUnaccessData(arr);
	// FIXME fFeature
/*
typedef struct  tagSAFEARRAYBOUND {
    ULONG              cElements;
    LONG               lLbound;
} SAFEARRAYBOUND;
 
typedef struct  tagSAFEARRAY {
    USHORT             cDims;
    USHORT             fFeatures;
    ULONG              cbElements;
    ULONG              cLocks;
    PVOID              pvData;
    SAFEARRAYBOUND     rgsabound [0];
} SAFEARRAY;
*/
}
static void get_tuple_with_two_elements(AST::Node* tuple, AST::Node*& a, AST::Node*& b) {
	/* FIXME */
}
template<typename A>
static inline void encodeNumber(AST::Node* source, A& destination) {
	Numbers::NativeInt temp;
	if(!Numbers::toNativeInt(source, temp))
		throw Evaluators::EvaluationException("pack: number does not fit into slot");
	else {
		destination = (A) temp;
		if(destination != temp)
			throw Evaluators::EvaluationException("pack: number does not fit into slot");
	}
}
static const ULONGLONG max64 = ~0ULL;
static AST::Node* powersOf10[8] = {
	new Int(1), // 0
	new Int(10), // 1
	new Int(100), // 2
	new Int(1000), // 3
	new Int(10000), // 4
	new Int(100000), // 5
	new Int(1000000), // 6
	new Int(10000000), // 7
	// C++ has problems with literals up to 10**28
};
static void get_DECIMAL(AST::Node* source, DECIMAL* dest) {
	// dest->wReserved = 0; ???  we assume that the caller took care of that (ugh).
	AST::Node* c = Evaluators::divremA(source, (Int*) powersOf10[0], NULL);
	if(!c || ! cons_P(c))
		throw EvaluationException("cannot convert that to DECIMAL");
	AST::Node* root = get_cons_head(c); // the integral part.
	NativeInt result2 = 0;
	dest->scale = 0; // FIXME log10 root
	dest->sign = 0; // FIXME DECIMAL_NEG if negative, otherwise 0.
	dest->Lo64 = 0;
	dest->Hi32 = 0;
	// FIXME Float, Rational.
	if(!Numbers::toNativeInt(root, result2) || (dest->Lo64 = result2) != result2) {
		// FIXME larger numbers.
		throw Evaluators::EvaluationException("value out of range for Variant");
	}
}
AST::Node* internDECIMAL(DECIMAL* source) {
	//Given DECIMAL d, the number of decimal places is d.scale 
	//and the value is (d.sign?-1:1) * (double(d.Lo64) + double(d.Hi32) * double(1UL<<32) * double(1UL<<32)) * pow(10, d.scale)
	AST::Node* denominator;
	if (source->scale < 8) // and >= 0
		denominator = powersOf10[source->scale];
	else {
		Integer result(1);
		for(unsigned i = 0; i < source->scale; ++i)
			result *= 10;
		denominator = new Integer(result);
	}
	// scale: Valid values are from 0 to 28. So 12.345 is represented as 12345 with a scale of 3.
	Integer result = Integer(source->Hi32) * max64 + Integer(source->Hi32) + Integer(source->Lo64);
	if(source->sign & DECIMAL_NEG)
		result = -result;
	return(Numbers::makeRatio(new Integer(result), denominator));
}
void encodeVariant(AST::Node* both, VARIANT* value) {
	AST::Node* vtNum;
	AST::Node* source;
	AST::Node* a;
	AST::Node* b;
	VariantInit(value);
	get_tuple_with_two_elements(both, vtNum, source);
	encodeNumber(vtNum, value->vt);
	if(value->vt & VT_BYREF) {
		value->byref = Evaluators::get_pointer(source);
		return;
	}
	if(value->vt & VT_ARRAY) {
		VARTYPE itemType = value->vt &~ VT_ARRAY;
		value->parray = marshalSafeArray(itemType, source);
		return;
	}
	switch(value->vt) {
	case VT_BSTR:
		{
			/*The string is not terminated.  Instead, the length of the string is stored as an unsigned long (four bytes) just before the first character of the string, which is not what the pointer points to! */
			std::wstring v = FromUTF8(Evaluators::get_string(source));
			value->bstrVal = SysAllocString(v.c_str()); // later SysFreeString
		}
		break;
	case VT_BOOL:
		value->boolVal = Evaluators::get_boolean(source);
		break;
	case VT_CY:
		get_tuple_with_two_elements(source, a, b);
		encodeNumber(a, value->cyVal.Lo);
		encodeNumber(b, value->cyVal.Hi);
		break;
	case VT_DATE:
		value->date = Evaluators::get_double(source);
		break;
	/*case VT_VARIANT: // pointer to variant blah
		return();
		break;*/
	case VT_DECIMAL:
		get_DECIMAL(source, &value->decVal);
		break;
	case VT_UI1:
		encodeNumber(source, value->bVal);
		break;
	case VT_UI2:
		encodeNumber(source, value->uiVal);
		break;
	case VT_UI4:
		encodeNumber(source, value->ulVal);
		break;
	case VT_UI8:
		encodeNumber(source, value->ullVal);
		break;
	case VT_I1:
		encodeNumber(source, value->cVal);
		break;
	case VT_I2:
		encodeNumber(source, value->iVal);
		break;
	case VT_I4:
		encodeNumber(source, value->lVal);
		break;
	case VT_I8:
		encodeNumber(source, value->llVal);
		break;
	case VT_R4:
		value->fltVal = Evaluators::get_float(source);
		break;
	case VT_R8:
		value->dblVal = Evaluators::get_double(source);
		break;
	case VT_INT:
		encodeNumber(source, value->intVal);
		break;
	case VT_UINT:
		encodeNumber(source, value->uintVal);
		break;
	case VT_VOID:
		value->byref = NULL; /* just in case */
		break;
	case VT_EMPTY: /* cannot be BYREF */
		value->byref = NULL; /* just in case */
		break;
	case VT_ERROR:
		encodeNumber(source, value->scode); // HRESULT
		break;
	case VT_DISPATCH:
		value->pdispVal = unwrapIDispatch(source);
		break;
	case VT_UNKNOWN:
		value->punkVal = unwrapIUnknown(source);
		break;
	case VT_NULL: /* cannot be BYREF */
		value->byref = NULL; /* just in case */
		break;
/* not for variant
	case VT_LPSTR:
		value->pcVal = _strdup(Evaluators::get_string(source));
		break;
	case VT_USERDEFINED:
		return();
		break;
	case VT_LPWSTR:
		{
			std::wstring v = FromUTF8(Evaluators::get_string(source));
			value->puiVal = _wcsdup(v.c_str());
		}
		return();
		break;
	case VT_HRESULT:
		encodeNumber(source, value->scode); // HRESULT
		break;
	case VT_PTR: // unique PTR
		return();
		break;
	case VT_SAFEARRAY:
		value->parray = marshalSafeArray(source);
		break;
	case VT_CARRAY:
		value->parray = marshalCArray(source);
		typeDesc.lpadesc;
		break;
*/
	case VT_RECORD:
		// BRECORD FIXME
		break;
	case VT_INT_PTR:
		if(sizeof(value->lVal) == sizeof(value->byref))
			encodeNumber(source, value->lVal);
		else
			encodeNumber(source, value->llVal);
		break;
	case VT_UINT_PTR:
		if(sizeof(value->lVal) == sizeof(value->byref))
			encodeNumber(source, value->ulVal);
		else
			encodeNumber(source, value->ullVal);
		break;
	default:
		throw Evaluators::EvaluationException("pack: unknown Variant Type");
		break;
	}
}
#define MAKE_VTV(b) AST::makeCons(Numbers::internNative(value->vt), AST::makeCons(b, NULL))
AST::Node* decodeVariant(VARIANT* value) {
	if(value->vt & VT_BYREF)
		return(MAKE_VTV(AST::makeBox(value->byref, NULL/*TODO*/)));
	if(value->vt & VT_ARRAY) {
		VARTYPE itemType = value->vt &~ VT_ARRAY;
		return(MAKE_VTV(demarshalSafeArray(itemType, value->parray)));
	}
	switch(value->vt) {
	case VT_BSTR:
		return(MAKE_VTV(AST::makeStr(ToUTF8(std::wstring(value->bstrVal))))); // TODO support native UCS-4 strings? Probably not.
	case VT_BOOL:
		return(MAKE_VTV(Evaluators::internNative(value->boolVal)));
	case VT_CY:
		return(MAKE_VTV(AST::makeCons(Numbers::internNativeU((unsigned long long) value->cyVal.Lo), AST::makeCons(Numbers::internNative((Numbers::NativeInt/*FIXME*/) value->cyVal.Hi), NULL))));
	case VT_DATE:
		return(MAKE_VTV(Numbers::internNative((Numbers::NativeFloat /* TODO */) value->date)));
#if 0
	case VT_VARIANT: /* pointer to variant blah */
		return();
#endif
	case VT_DECIMAL:
		return(MAKE_VTV(internDECIMAL(&value->decVal)));
	case VT_UI1:
		return(MAKE_VTV(Numbers::internNativeU((unsigned long long) value->bVal)));
	case VT_UI2:
		return(MAKE_VTV(Numbers::internNativeU((unsigned long long) value->uiVal)));
	case VT_UI4:
		return(MAKE_VTV(Numbers::internNativeU((unsigned long long) value->ulVal)));
	case VT_UI8: /* technically not used for VARIANT */
		return(MAKE_VTV(Numbers::internNativeU((unsigned long long) value->ullVal)));
	case VT_I1:
		return(MAKE_VTV(Numbers::internNative((long long) value->cVal)));
	case VT_I2:
		return(MAKE_VTV(Numbers::internNative((long long) value->iVal)));
	case VT_I4:
		return(MAKE_VTV(Numbers::internNative((long long) value->lVal)));
	case VT_I8: /* technically not used for VARIANT */
		return(MAKE_VTV(Numbers::internNative((long long) value->llVal)));
	case VT_R4:
		return(MAKE_VTV(Numbers::internNative((Numbers::NativeFloat /* TODO */) value->fltVal)));
	case VT_R8:
		return(MAKE_VTV(Numbers::internNative((Numbers::NativeFloat /* TODO */) value->dblVal)));
	case VT_INT:
		return(MAKE_VTV(Numbers::internNative(value->intVal)));
	case VT_UINT:
		return(MAKE_VTV(Numbers::internNativeU(value->uintVal)));
	case VT_VOID:
		return(MAKE_VTV(NULL));
	case VT_EMPTY: /* cannot be BYREF */
		return(MAKE_VTV(NULL));
	case VT_ERROR:
		return(MAKE_VTV(Numbers::internNativeU((unsigned long long) value->scode))); // HRESULT
	case VT_HRESULT: /* technically not used for VARIANT */
		return(MAKE_VTV(Numbers::internNativeU((unsigned long long) value->scode)));
	case VT_DISPATCH:
		return(MAKE_VTV(wrapIDispatch(value->pdispVal)));
	case VT_UNKNOWN:
		return(MAKE_VTV(wrapIUnknown(value->punkVal)));
	case VT_NULL: /* cannot be BYREF */
		return(MAKE_VTV(NULL)); /* nil, whatever */
/*	case VT_SAFEARRAY:
		return(MAKE_VTV(demarshalSafeArray(value->parray)));
	case VT_PTR:
		return();
	case VT_CARRAY:
		return(MAKE_VTV(demarshalCArray(value->parray)));
	case VT_USERDEFINED:
		return();
	case VT_LPSTR:
		return();
	case VT_LPWSTR:
		return();
*/
	//case VT_RECORD:
		// BRECORD FIXME
	case VT_INT_PTR:
		return(MAKE_VTV(Numbers::internNative((Numbers::NativeInt) (sizeof(value->lVal) == sizeof(value->byref) ? value->lVal : value->llVal)))); // TODO proper size
	case VT_UINT_PTR:
		return(MAKE_VTV(Numbers::internNative((Numbers::NativeInt) (sizeof(value->ulVal) == sizeof(value->byref) ? value->ulVal : value->ullVal)))); // TODO proper size
	default:
		throw Evaluators::EvaluationException("unpack: unknown Variant Type");
	}
}
/*
SAFEARRAY FAR* FAR*ppsa;
unsigned int ndim = 2;
HRESULT HRESULT = SafeArrayAllocDescriptor(ndim, ppsa);
if( FAILED(hresult))
  return ERR_OutOfMemory;
(*ppsa)->rgsabound[ 0 ].lLbound = 0;
(*ppsa)->rgsabound[ 0 ].cElements = 5;
(*ppsa)->rgsabound[ 1 ].lLbound = 1;
(*ppsa)->rgsabound[ 1 ].cElements = 4;
HRESULT = SafeArrayAllocData(*ppsa);
if( FAILED(hresult)) {
  SafeArrayDestroyDescriptor(*ppsa)
  return ERR_OutOfMemory;
}
*/
 /**<p> This class represents the <code>Union</code> data type. Its usage is dictated by the discriminant
   * which acts as a "switch" to select the correct member to be serialized\deserialzed. <p>
   * 
   * Sample Usage :-
   * 
   * <br>
   * <code>
   *     JIUnion forTypeDesc = new JIUnion(Short.class); <br>
   *    JIPointer ptrToTypeDesc = new JIPointer(typeDesc); <br>
   *    JIPointer ptrToArrayDesc = new JIPointer(arrayDesc); <br>
   *    forTypeDesc.addMember(TypeDesc.VT_PTR,ptrToTypeDesc); <br>
   *    forTypeDesc.addMember(TypeDesc.VT_SAFEARRAY,ptrToTypeDesc); <br>
   *    forTypeDesc.addMember(TypeDesc.VT_CARRAY,ptrToArrayDesc); <br>
   *    forTypeDesc.addMember(TypeDesc.VT_USERDEFINED,Integer.class); <p>
   * </code>
   *
   *    The TypeDesc.VT_PTR is an <code>Integer</code> and is used as a discriminant to select ptrTypeDesc, TypeDesc.VT_CARRAY 
   *  chooses ptrArrayDesc. <br>
   */
/*
typedef struct CLSID // 16 bytes
{
DWORD Data1;
WORD Data2;
WORD Data3;
BYTE Data4[8]; // chars are one-based
} CLSID; 
*/
}; /* end namespace FFIs */
