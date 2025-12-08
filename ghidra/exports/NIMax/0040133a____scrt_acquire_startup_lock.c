
/* Library Function - Single Match
    ___scrt_acquire_startup_lock
   
   Library: Visual Studio 2015 Release */

int ___scrt_acquire_startup_lock(void)

{
  int iVar1;
  bool bVar2;
  uint3 extraout_var;
  int iVar3;
  uint3 uVar4;
  
  bVar2 = ___scrt_is_ucrt_dll_in_use();
  if (CONCAT31(extraout_var,bVar2) == 0) {
    return (uint)extraout_var << 8;
  }
  while( true ) {
    iVar3 = 0;
    LOCK();
    iVar1 = *(int *)((int)Self + 4);
    if (DAT_004042b0 != 0) {
      iVar3 = DAT_004042b0;
      iVar1 = DAT_004042b0;
    }
    DAT_004042b0 = iVar1;
    UNLOCK();
    uVar4 = (uint3)((uint)iVar3 >> 8);
    if (iVar3 == 0) break;
    if (*(int *)((int)Self + 4) == iVar3) {
      return CONCAT31(uVar4,1);
    }
  }
  return (uint)uVar4 << 8;
}

