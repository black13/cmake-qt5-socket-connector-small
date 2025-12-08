
/* Library Function - Single Match
    __onexit
   
   Libraries: Visual Studio 2015 Release, Visual Studio 2017 Release */

_onexit_t __cdecl __onexit(_onexit_t _Func)

{
  int iVar1;
  byte bVar2;
  
  bVar2 = (byte)DAT_00404014 & 0x1f;
  if (((DAT_00404014 ^ DAT_004042b4) >> bVar2 | (DAT_00404014 ^ DAT_004042b4) << 0x20 - bVar2) ==
      0xffffffff) {
    iVar1 = crt_atexit();
  }
  else {
    iVar1 = register_onexit_function(&DAT_004042b4,_Func);
  }
  return (_onexit_t)(~-(uint)(iVar1 != 0) & (uint)_Func);
}

