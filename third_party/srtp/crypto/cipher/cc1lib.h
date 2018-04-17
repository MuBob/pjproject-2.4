#ifndef _CC1LIB_H_
#define _CC1LIB_H_

typedef void* HANDLE;
#ifdef __cplusplus
extern "C" {
#endif


#ifdef _CC_1_DLL
#define CC_1_DLL_API __declspec(dllexport)
#else
#define CC_1_DLL_API __declspec(dllimport)
#endif


HANDLE CC_1_DLL_API CC_1_CreateContext(unsigned long id, unsigned char Rstar[72]);
void CC_1_DLL_API CC_1_DestroyContext(HANDLE hContext);
void CC_1_DLL_API CC_1_SesPackageNumber(HANDLE hContext, unsigned long packNum);
void CC_1_DLL_API CC_1_Encrypt(HANDLE hContext, unsigned char *pBuf, int len);
#ifdef __cplusplus
}
#endif

#endif
