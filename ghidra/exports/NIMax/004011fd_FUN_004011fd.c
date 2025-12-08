
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_004011fd(void)

{
  code *pcVar1;
  HMODULE hModule;
  FARPROC pFVar2;
  FARPROC pFVar3;
  FARPROC pFVar4;
  undefined4 uVar5;
  undefined *puVar6;
  
  __vcrt_InitializeCriticalSectionEx(&DAT_00404284,4000,0);
  hModule = GetModuleHandleW(L"kernel32.dll");
  if (hModule != (HMODULE)0x0) {
    pFVar2 = GetProcAddress(hModule,"InitializeConditionVariable");
    pFVar3 = GetProcAddress(hModule,"SleepConditionVariableCS");
    pFVar4 = GetProcAddress(hModule,"WakeAllConditionVariable");
    if (((pFVar2 == (FARPROC)0x0) || (pFVar3 == (FARPROC)0x0)) || (pFVar4 == (FARPROC)0x0)) {
      DAT_004042a0 = CreateEventW((LPSECURITY_ATTRIBUTES)0x0,1,0,(LPCWSTR)0x0);
      if (DAT_004042a0 == (HANDLE)0x0) goto LAB_004012b5;
    }
    else {
      DAT_004042a0 = (HANDLE)0x0;
      puVar6 = &DAT_0040429c;
      guard_check_icall();
      (*pFVar2)(puVar6);
      _DAT_004042a4 = FUN_004012bd((uint)pFVar3);
      _DAT_004042a8 = FUN_004012bd((uint)pFVar4);
    }
    _atexit((_func_4879 *)&LAB_004012da);
    return 0;
  }
LAB_004012b5:
  ___scrt_fastfail();
  pcVar1 = (code *)swi(3);
  uVar5 = (*pcVar1)();
  return uVar5;
}

