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
//typedef short uint16_t;
#ifdef WIN32
#include <OaIdl.h> /* VARIANT */
#else
typedef struct  tagCY {
    unsigned long      Lo;
    long               Hi;
} CY;
 
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
    USHORT             fFeatures;
    ULONG              cbElements;
    ULONG              cLocks;
    PVOID              pvData;
    SAFEARRAYBOUND     rgsabound [0];
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
  VT_I8 = 0x0014,
  VT_UI8 = 0x0015,
  VT_INT = 0x0016,
  VT_UINT = 0x0017,
  VT_VOID = 0x0018,
  VT_HRESULT = 0x0019,
  VT_PTR = 0x001A,
  VT_SAFEARRAY = 0x001B,
  VT_CARRAY = 0x001C,
  VT_USERDEFINED = 0x001D,
  VT_LPSTR = 0x001E,
  VT_LPWSTR = 0x001F,
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
        _VARIANT_BOOL       bool_;
        SCODE               scode;
        CY                  cyVal;
        DATE                date;
        BSTR                bstrVal;
        IUnknown            *punkVal;
        IDispatch           *pdispVal;
        SAFEARRAY           *parray; // see SafeArrayCreateVector, SafeArrayPutElement
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
        CHAR                cVal;
        USHORT              uiVal;
        ULONG               ulVal;
        ULONGLONG           ullVal;
        INT                 intVal;
        UINT                uintVal;
        DECIMAL             *pdecVal;
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

static SAFEARRAY* marshalCArray(AST::Node* source) {
	SAFEARRAY* result = SafeArrayCreateVector(VT_BSTR, 0, (unsigned int) myPaths.size() );
	// FIXME
	return(result);
}
static AST::Node* demarshalCArray(SAFEARRAY* arr) {
	// FIXME
	return(NULL);
}
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
	SafeArrayGetLBound(psa, 1, &lBound);
	SafeArrayGetUBound(psa, 1, &uBound);
	long numElems = uBound - lBound + 1;

	IUnknown** rawArray;
	SafeArrayAccessData(psa, (void**)&rawArray);
	/*for (long i = 0; i < numElems; ++i) {
	  IUnknown* pElem = rawArray[i];
	  DoSomething(pElem);
	}*/
	SafeArrayUnaccessData(psa);
	cDims
	fFeature
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
static inline void encodeNumber(AST::Node* source, int& destination) {
	if(!Numbers::toNativeInt(source, destination))
		throw Evaluators::EvaluationException("pack: number does not fit into slot");
}
void encodeVariant(AST::Node* both, VARIANT* value) {
	bool B_ok = false;
	AST::Node* vtNum;
	AST::Node* source;
	AST::Node* a;
	AST::Node* b;
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
		/* decVal 
USHORT             wReserved;
BYTE               scale;
BYTE               sign;
ULONG              Hi32;
ULONGLONG          Lo64;*/
/* see VarDec* , VarDecCmp, VarDecDiv, VarDecMul, VarDecInt, VarDecSub */
		value->decVal;
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
	case VT_DISPATCH:
		return();
		break;
	case VT_ERROR:
		return(); // HRESULT
		break;
	case VT_HRESULT:
		return(); // HRESULT
		break;
	case VT_PTR: /* unique PTR */
		return();
		break;
	case VT_UNKNOWN:
		return();
		break;
	case VT_NULL: /* cannot be BYREF */
		value->byref = NULL; /* just in case */
		break;
	case VT_SAFEARRAY:
		value->parray = marshalSafeArray(source);
		break;
	case VT_CARRAY:
		value->parray = marshalCArray(source);
		typeDesc.lpadesc;
		break;
	case VT_USERDEFINED:
		return();
		break;
	case VT_LPSTR:
		return();
		break;
	case VT_LPWSTR:
		return();
		break;
	case VT_RECORD:
		// BRECORD
		return();
		break;
	case VT_INT_PTR:
		return(sizeof(void*) ? );
		break;
	case VT_UINT_PTR:
		return(sizeof(void*) ? );
		break;
	default:
		B_ok = false;
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
		return(MAKE_VTV(ToUTF8(std::wstring(value->bstrVal)))); // TODO support native UCS-4 strings? Probably not.
	case VT_BOOL:
		return(MAKE_VTV(AST::internNative(value->boolVal)));
	case VT_CY:
		return(MAKE_VTV(AST::makeCons(Numbers::internNativeU(value->cyVal.Lo), AST::makeCons(Numbers::internNative(value->cyVal.Hi), NULL)));
	case VT_DATE:
		return(MAKE_VTV(Numbers::internNative(value->date)));
	case VT_VARIANT: /* pointer to variant blah */
		return();
	case VT_DECIMAL:
		/*
USHORT             wReserved;
BYTE               scale;
BYTE               sign;
ULONG              Hi32;
ULONGLONG          Lo64;*/
		return();
	case VT_UI1:
		return(MAKE_VTV(Numbers::internNative(value->bVal)));
	case VT_UI2:
		return(MAKE_VTV(Numbers::internNative(value->uiVal)));
	case VT_UI4:
		return(MAKE_VTV(Numbers::internNative(value->ulVal))));
	case VT_UI8:
		return(MAKE_VTV(Numbers::internNative(value->ullVal))));
	case VT_I1:
		return(MAKE_VTV(Numbers::internNative(value->cVal)));
	case VT_I2:
		return(MAKE_VTV(Numbers::internNative(value->iVal)));
	case VT_I4:
		return(MAKE_VTV(Numbers::internNative(value->lVal))));
	case VT_I8:
		return(MAKE_VTV(Numbers::internNative(value->llVal))));
	case VT_R4:
		return(MAKE_VTV(Numbers::internNative((Numbers::NativeFloat /* FIXME */) value->fltVal))));
	case VT_R8:
		return(MAKE_VTV(Numbers::internNative(value->dblVal)));
	case VT_INT:
		return(MAKE_VTV(Numbers::internNative(value->intVal)));
	case VT_UINT:
		return(MAKE_VTV(Numbers::internNative(value->uintVal)));
	case VT_VOID:
		return(MAKE_VTV(NULL));
	case VT_EMPTY: /* cannot be BYREF */
		return(MAKE_VTV(NULL));
	case VT_DISPATCH:
		return();
	case VT_ERROR:
		return(); // HRESULT
	case VT_HRESULT:
		return(); // HRESULT
	case VT_PTR: /* unique PTR */
		return();
	case VT_UNKNOWN:
		return(MAKE_VTV(???));
	case VT_NULL: /* cannot be BYREF */
		return(MAKE_VTV(NULL)); /* nil, whatever */
	case VT_SAFEARRAY:
		return(MAKE_VTV(demarshalSafeArray(value->parray)));
	case VT_CARRAY:
		return(MAKE_VTV(demarshalCArray(value->parray)));
	case VT_USERDEFINED:
		return();
	case VT_LPSTR:
		return();
	case VT_LPWSTR:
		return();
	case VT_RECORD:
		// BRECORD
		return();
	case VT_INT_PTR:
		return(sizeof(void*) ? );
	case VT_UINT_PTR:
		return(sizeof(void*) ? );
	default:
		return();
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
