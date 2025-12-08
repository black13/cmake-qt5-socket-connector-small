
/* Library Function - Single Match
    ___scrt_is_managed_app
   
   Library: Visual Studio 2015 Release */

uint ___scrt_is_managed_app(void)

{
  HMODULE pHVar1;
  HMODULE pHVar2;
  
  pHVar1 = GetModuleHandleW((LPCWSTR)0x0);
  pHVar2 = pHVar1;
  if ((((pHVar1 != (HMODULE)0x0) && (pHVar2 = (HMODULE)0x5a4d, (short)pHVar1->unused == 0x5a4d)) &&
      (pHVar2 = (HMODULE)((int)&pHVar1->unused + pHVar1[0xf].unused), pHVar2->unused == 0x4550)) &&
     (((short)pHVar2[6].unused == 0x10b && (0xe < (uint)pHVar2[0x1d].unused)))) {
    return CONCAT31((int3)((uint)pHVar2 >> 8),pHVar2[0x3a].unused != 0);
  }
  return (uint)pHVar2 & 0xffffff00;
}

