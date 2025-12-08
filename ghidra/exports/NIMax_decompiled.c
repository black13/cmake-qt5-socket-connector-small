/*
Ghidra single-file decompilation export
Program: NIMax.exe
Language: x86/little/32/default
*/

/* Function FUN_00401050 @ 00401050 */

uint __fastcall FUN_00401050(LPCRITICAL_SECTION param_1)

{
  BOOL BVar1;
  uint uVar2;
  
  BVar1 = InitializeCriticalSectionEx(param_1,0,0);
  if (BVar1 == 0) {
    uVar2 = GetLastError();
    if (0 < (int)uVar2) {
      return uVar2 & 0xffff | 0x80070000;
    }
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}



/* Function FUN_00401080 @ 00401080 */

undefined4 FUN_00401080(void)

{
  return 0;
}



/* Function FUN_00401090 @ 00401090 */

int FUN_00401090(HMODULE param_1)

{
  int iVar1;
  AFX_MODULE_STATE *pAVar2;
  
  FUN_004010e0(param_1);
  NiMaxInit();
  iVar1 = NiMaxRun((char *)0x0,(ImxApp *)0x0);
  NiMaxCleanup();
  pAVar2 = (AFX_MODULE_STATE *)FUN_004011b0();
  AfxSetModuleState(pAVar2);
  return iVar1;
}



/* Function guard_check_icall @ 004010d0 */

/* guard_check_icall */

void __cdecl guard_check_icall(void)

{
  return;
}



/* Function FUN_004010e0 @ 004010e0 */

void FUN_004010e0(HMODULE param_1)

{
  DWORD DVar1;
  char *pcVar2;
  
  DVar1 = GetModuleFileNameA(param_1,&DAT_004040a0,0x104);
  if (DVar1 == 0) {
    DAT_004040a0 = 0;
    return;
  }
  pcVar2 = strrchr(&DAT_004040a0,0x5c);
  if (pcVar2 != (char *)0x0) {
    *pcVar2 = '\0';
  }
  return;
}



/* Function operator_delete @ 0040111e */

void __cdecl operator_delete(void *param_1)

{
                    /* WARNING: Could not recover jumptable at 0x0040111e. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  operator_delete(param_1);
  return;
}



/* Function AfxSetModuleState @ 00401124 */

AFX_MODULE_STATE * AfxSetModuleState(AFX_MODULE_STATE *param_1)

{
  AFX_MODULE_STATE *pAVar1;
  
                    /* WARNING: Could not recover jumptable at 0x00401124. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  pAVar1 = AfxSetModuleState(param_1);
  return pAVar1;
}



/* Function FUN_0040117d @ 0040117d */

AFX_MODULE_STATE * __thiscall FUN_0040117d(void *this,byte param_1)

{
  AFX_MODULE_STATE::~AFX_MODULE_STATE((AFX_MODULE_STATE *)this);
  if ((param_1 & 1) != 0) {
    if ((param_1 & 4) == 0) {
      CNoTrackObject::operator_delete(this);
    }
    else {
      guard_check_icall();
    }
  }
  return (AFX_MODULE_STATE *)this;
}



/* Function FUN_004011b0 @ 004011b0 */

undefined * FUN_004011b0(void)

{
  return &DAT_004041e0;
}



/* Function FUN_004011fd @ 004011fd */

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



/* Function FUN_004012bd @ 004012bd */

uint __cdecl FUN_004012bd(uint param_1)

{
  byte bVar1;
  
  bVar1 = 0x20 - ((byte)DAT_00404014 & 0x1f) & 0x1f;
  return (param_1 >> bVar1 | param_1 << 0x20 - bVar1) ^ DAT_00404014;
}



/* Function find_pe_section @ 004012f6 */

/* Library Function - Single Match
    struct _IMAGE_SECTION_HEADER * __cdecl find_pe_section(unsigned char * const,unsigned int)
   
   Library: Visual Studio 2015 Release */

_IMAGE_SECTION_HEADER * __cdecl find_pe_section(uchar *param_1,uint param_2)

{
  int iVar1;
  _IMAGE_SECTION_HEADER *p_Var2;
  _IMAGE_SECTION_HEADER *p_Var3;
  
  iVar1 = *(int *)(param_1 + 0x3c);
  p_Var2 = (_IMAGE_SECTION_HEADER *)
           (param_1 + (uint)*(ushort *)(param_1 + iVar1 + 0x14) + iVar1 + 0x18);
  p_Var3 = p_Var2 + (uint)*(ushort *)(param_1 + iVar1 + 6) * 0x28;
  while( true ) {
    if (p_Var2 == p_Var3) {
      return (_IMAGE_SECTION_HEADER *)0x0;
    }
    if ((*(uint *)(p_Var2 + 0xc) <= param_2) &&
       (param_2 < (uint)(*(int *)(p_Var2 + 8) + *(int *)(p_Var2 + 0xc)))) break;
    p_Var2 = p_Var2 + 0x28;
  }
  return p_Var2;
}



/* Function ___scrt_acquire_startup_lock @ 0040133a */

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



/* Function ___scrt_initialize_crt @ 0040136f */

/* Library Function - Single Match
    ___scrt_initialize_crt
   
   Library: Visual Studio 2015 Release */

int __cdecl ___scrt_initialize_crt(int param_1)

{
  char cVar1;
  uint3 extraout_var;
  uint3 uVar2;
  undefined3 extraout_var_00;
  uint3 extraout_var_01;
  
  if (param_1 == 0) {
    DAT_004042cc = 1;
  }
  ___isa_available_init();
  cVar1 = FUN_0040208a();
  uVar2 = extraout_var;
  if (cVar1 != '\0') {
    cVar1 = FUN_0040208a();
    if (cVar1 != '\0') {
      return CONCAT31(extraout_var_00,1);
    }
    FUN_0040208a();
    uVar2 = extraout_var_01;
  }
  return (uint)uVar2 << 8;
}



/* Function ___scrt_initialize_onexit_tables @ 004013a8 */

/* Library Function - Single Match
    ___scrt_initialize_onexit_tables
   
   Library: Visual Studio 2015 Release */

uint __cdecl ___scrt_initialize_onexit_tables(int param_1)

{
  code *pcVar1;
  byte bVar2;
  bool bVar3;
  undefined3 extraout_var;
  uint uVar4;
  int iVar5;
  
  if ((param_1 != 0) && (param_1 != 1)) {
    ___scrt_fastfail();
    pcVar1 = (code *)swi(3);
    uVar4 = (*pcVar1)();
    return uVar4;
  }
  bVar3 = ___scrt_is_ucrt_dll_in_use();
  if ((CONCAT31(extraout_var,bVar3) == 0) || (param_1 != 0)) {
    bVar2 = 0x20 - ((byte)DAT_00404014 & 0x1f) & 0x1f;
    DAT_004042b4 = (0xffffffffU >> bVar2 | -1 << 0x20 - bVar2) ^ DAT_00404014;
    uVar4 = CONCAT31((int3)(DAT_004042b4 >> 8),1);
    DAT_004042b8 = DAT_004042b4;
    DAT_004042bc = DAT_004042b4;
    DAT_004042c0 = DAT_004042b4;
    DAT_004042c4 = DAT_004042b4;
    DAT_004042c8 = DAT_004042b4;
  }
  else {
    uVar4 = initialize_onexit_table(&DAT_004042b4);
    if (uVar4 == 0) {
      iVar5 = initialize_onexit_table(&DAT_004042c0);
      uVar4 = CONCAT31((int3)((uint)-iVar5 >> 8),'\x01' - (iVar5 != 0));
    }
    else {
      uVar4 = uVar4 & 0xffffff00;
    }
  }
  return uVar4;
}



/* Function ___scrt_is_nonwritable_in_current_image @ 0040143f */

/* WARNING: Function: __SEH_prolog4 replaced with injection: SEH_prolog4 */
/* WARNING: Function: __SEH_epilog4 replaced with injection: EH_epilog3 */
/* Library Function - Single Match
    ___scrt_is_nonwritable_in_current_image
   
   Library: Visual Studio 2015 Release */

uint __cdecl ___scrt_is_nonwritable_in_current_image(int param_1)

{
  _IMAGE_SECTION_HEADER *p_Var1;
  
  p_Var1 = (_IMAGE_SECTION_HEADER *)0x5a4d;
  if ((((IMAGE_DOS_HEADER_00400000.e_magic == (char  [2])0x5a4d) &&
       (p_Var1 = (_IMAGE_SECTION_HEADER *)IMAGE_DOS_HEADER_00400000.e_lfanew,
       *(int *)(IMAGE_DOS_HEADER_00400000.e_lfanew + 0x400000) == 0x4550)) &&
      (*(short *)((int)IMAGE_DOS_HEADER_00400000.e_res_4_ + (IMAGE_DOS_HEADER_00400000.e_lfanew - 4)
                 ) == 0x10b)) &&
     ((p_Var1 = find_pe_section((uchar *)&IMAGE_DOS_HEADER_00400000,param_1 - 0x400000),
      p_Var1 != (_IMAGE_SECTION_HEADER *)0x0 && (-1 < *(int *)(p_Var1 + 0x24))))) {
    return CONCAT31((int3)((uint)p_Var1 >> 8),1);
  }
  return (uint)p_Var1 & 0xffffff00;
}



/* Function ___scrt_release_startup_lock @ 004014c9 */

/* Library Function - Single Match
    ___scrt_release_startup_lock
   
   Library: Visual Studio 2015 Release */

int __cdecl ___scrt_release_startup_lock(char param_1)

{
  int iVar1;
  bool bVar2;
  undefined3 extraout_var;
  int iVar3;
  
  bVar2 = ___scrt_is_ucrt_dll_in_use();
  iVar1 = DAT_004042b0;
  iVar3 = CONCAT31(extraout_var,bVar2);
  if ((iVar3 != 0) && (param_1 == '\0')) {
    LOCK();
    DAT_004042b0 = 0;
    UNLOCK();
    iVar3 = iVar1;
  }
  return iVar3;
}



/* Function ___scrt_uninitialize_crt @ 004014e6 */

/* Library Function - Single Match
    ___scrt_uninitialize_crt
   
   Library: Visual Studio 2015 Release */

undefined1 __cdecl ___scrt_uninitialize_crt(undefined4 param_1,char param_2)

{
  if ((DAT_004042cc == '\0') || (param_2 == '\0')) {
    FUN_0040208a();
    FUN_0040208a();
  }
  return 1;
}



/* Function __onexit @ 0040150e */

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



/* Function _atexit @ 00401549 */

/* Library Function - Single Match
    _atexit
   
   Libraries: Visual Studio 2015 Release, Visual Studio 2017 Release */

int __cdecl _atexit(_func_4879 *param_1)

{
  _onexit_t p_Var1;
  
  p_Var1 = __onexit((_onexit_t)param_1);
  return (p_Var1 != (_onexit_t)0x0) - 1;
}



/* Function FUN_0040155e @ 0040155e */

void __cdecl FUN_0040155e(void *param_1)

{
  operator_delete(param_1);
  return;
}



/* Function FUN_0040156c @ 0040156c */

undefined4 * __thiscall FUN_0040156c(void *this,byte param_1)

{
  *(undefined ***)this = type_info::vftable;
  if ((param_1 & 1) != 0) {
    FUN_0040155e(this);
  }
  return (undefined4 *)this;
}



/* Function __scrt_common_main_seh @ 0040164d */

/* WARNING: Function: __SEH_prolog4 replaced with injection: SEH_prolog4 */
/* WARNING: Function: __SEH_epilog4 replaced with injection: EH_epilog3 */
/* Library Function - Single Match
    int __cdecl __scrt_common_main_seh(void)
   
   Library: Visual Studio 2015 Release */

int __cdecl __scrt_common_main_seh(void)

{
  code *pcVar1;
  bool bVar2;
  WORD WVar3;
  undefined4 uVar4;
  int iVar5;
  int *piVar6;
  uint uVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  
  uVar4 = ___scrt_initialize_crt(1);
  if ((char)uVar4 != '\0') goto LAB_0040166c;
  do {
    ___scrt_fastfail();
LAB_0040166c:
    bVar2 = false;
    uVar4 = ___scrt_acquire_startup_lock();
  } while (DAT_004042ac == 1);
  if (DAT_004042ac == 0) {
    DAT_004042ac = 1;
    iVar5 = initterm_e(&DAT_00403134,&DAT_00403144);
    if (iVar5 != 0) {
      return 0xff;
    }
    initterm(&DAT_00403120,&DAT_00403130);
    DAT_004042ac = 2;
  }
  else {
    bVar2 = true;
  }
  ___scrt_release_startup_lock((char)uVar4);
  piVar6 = (int *)FUN_00401d90();
  if ((*piVar6 != 0) &&
     (uVar4 = ___scrt_is_nonwritable_in_current_image((int)piVar6), (char)uVar4 != '\0')) {
    uVar9 = 0;
    uVar8 = 2;
    uVar4 = 0;
    pcVar1 = (code *)*piVar6;
    guard_check_icall();
    (*pcVar1)(uVar4,uVar8,uVar9);
  }
  piVar6 = (int *)FUN_00401d96();
  if ((*piVar6 != 0) &&
     (uVar4 = ___scrt_is_nonwritable_in_current_image((int)piVar6), (char)uVar4 != '\0')) {
    register_thread_local_exe_atexit_callback(*piVar6);
  }
  WVar3 = ___scrt_get_show_window_mode();
  get_narrow_winmain_command_line(WVar3);
  iVar5 = FUN_00401090((HMODULE)&IMAGE_DOS_HEADER_00400000);
  uVar7 = ___scrt_is_managed_app();
  if ((char)uVar7 != '\0') {
    if (!bVar2) {
      _cexit();
    }
    ___scrt_uninitialize_crt(1,'\0');
    return iVar5;
  }
                    /* WARNING: Subroutine does not return */
  exit(iVar5);
}



/* Function entry @ 004017b5 */

void entry(void)

{
  ___security_init_cookie();
  __scrt_common_main_seh();
  return;
}



/* Function __security_check_cookie @ 004017bf */

/* Library Function - Single Match
    @__security_check_cookie@4
   
   Library: Visual Studio 2015 Release
   __fastcall __security_check_cookie,4 */

void __fastcall __security_check_cookie(int param_1)

{
  if (param_1 == DAT_00404014) {
    return;
  }
                    /* WARNING: Subroutine does not return */
  ___report_gsfailure();
}



/* Function __EH_epilog3 @ 004017d0 */

/* WARNING: This is an inlined function */
/* Library Function - Single Match
    __EH_epilog3
   
   Libraries: Visual Studio 2015, Visual Studio 2017, Visual Studio 2019 */

void __EH_epilog3(void)

{
  undefined4 *unaff_EBP;
  undefined4 unaff_retaddr;
  
  ExceptionList = (void *)unaff_EBP[-3];
  *unaff_EBP = unaff_retaddr;
  return;
}



/* Function __EH_prolog3 @ 004017e5 */

/* WARNING: This is an inlined function */
/* WARNING: Unable to track spacebase fully for stack */
/* WARNING: Variable defined which should be unmapped: param_1 */
/* Library Function - Single Match
    __EH_prolog3
   
   Libraries: Visual Studio 2015, Visual Studio 2017, Visual Studio 2019 */

void __cdecl __EH_prolog3(int param_1)

{
  int iVar1;
  undefined4 unaff_EBX;
  undefined4 unaff_ESI;
  undefined4 unaff_EDI;
  undefined4 unaff_retaddr;
  uint auStack_1c [5];
  undefined1 local_8 [8];
  
  iVar1 = -param_1;
  *(undefined4 *)((int)auStack_1c + iVar1 + 0x10) = unaff_EBX;
  *(undefined4 *)((int)auStack_1c + iVar1 + 0xc) = unaff_ESI;
  *(undefined4 *)((int)auStack_1c + iVar1 + 8) = unaff_EDI;
  *(uint *)((int)auStack_1c + iVar1 + 4) = DAT_00404014 ^ (uint)&param_1;
  *(undefined4 *)((int)auStack_1c + iVar1) = unaff_retaddr;
  ExceptionList = local_8;
  return;
}



/* Function __SEH_prolog4 @ 00401820 */

/* WARNING: This is an inlined function */
/* WARNING: Unable to track spacebase fully for stack */
/* WARNING: Variable defined which should be unmapped: param_2 */
/* Library Function - Single Match
    __SEH_prolog4
   
   Libraries: Visual Studio 2015, Visual Studio 2017, Visual Studio 2019 */

void __cdecl __SEH_prolog4(undefined4 param_1,int param_2)

{
  int iVar1;
  undefined4 unaff_EBX;
  undefined4 unaff_ESI;
  undefined4 unaff_EDI;
  undefined4 unaff_retaddr;
  uint auStack_1c [5];
  undefined1 local_8 [8];
  
  iVar1 = -param_2;
  *(undefined4 *)((int)auStack_1c + iVar1 + 0x10) = unaff_EBX;
  *(undefined4 *)((int)auStack_1c + iVar1 + 0xc) = unaff_ESI;
  *(undefined4 *)((int)auStack_1c + iVar1 + 8) = unaff_EDI;
  *(uint *)((int)auStack_1c + iVar1 + 4) = DAT_00404014 ^ (uint)&param_2;
  *(undefined4 *)((int)auStack_1c + iVar1) = unaff_retaddr;
  ExceptionList = local_8;
  return;
}



/* Function __SEH_epilog4 @ 00401866 */

/* WARNING: This is an inlined function */
/* Library Function - Single Match
    __SEH_epilog4
   
   Libraries: Visual Studio 2015, Visual Studio 2017, Visual Studio 2019 */

void __SEH_epilog4(void)

{
  undefined4 *unaff_EBP;
  undefined4 unaff_retaddr;
  
  ExceptionList = (void *)unaff_EBP[-4];
  *unaff_EBP = unaff_retaddr;
  return;
}



/* Function FUN_004018da @ 004018da */

void __cdecl
FUN_004018da(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  except_handler4_common(&DAT_00404014,__security_check_cookie,param_1,param_2,param_3,param_4);
  return;
}



/* Function ___scrt_fastfail @ 004018fd */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* Library Function - Single Match
    ___scrt_fastfail
   
   Library: Visual Studio 2015 Release */

void ___scrt_fastfail(void)

{
  code *pcVar1;
  BOOL BVar2;
  LONG LVar3;
  undefined4 local_328 [39];
  EXCEPTION_RECORD local_5c;
  _EXCEPTION_POINTERS local_c;
  
  BVar2 = IsProcessorFeaturePresent(0x17);
  if (BVar2 != 0) {
    pcVar1 = (code *)swi(0x29);
    (*pcVar1)();
  }
  _DAT_004042d4 = 0;
  memset(local_328,0,0x2cc);
  local_328[0] = 0x10001;
  memset(&local_5c,0,0x50);
  local_5c.ExceptionCode = 0x40000015;
  local_5c.ExceptionFlags = 1;
  BVar2 = IsDebuggerPresent();
  local_c.ExceptionRecord = &local_5c;
  local_c.ContextRecord = (PCONTEXT)local_328;
  SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)0x0);
  LVar3 = UnhandledExceptionFilter(&local_c);
  if (LVar3 == 0) {
    _DAT_004042d4 = _DAT_004042d4 & -(uint)(BVar2 == 1);
  }
  return;
}



/* Function ___scrt_get_show_window_mode @ 00401a18 */

/* Library Function - Single Match
    ___scrt_get_show_window_mode
   
   Library: Visual Studio 2015 Release */

WORD ___scrt_get_show_window_mode(void)

{
  _STARTUPINFOW local_48;
  
  memset(&local_48,0,0x44);
  GetStartupInfoW(&local_48);
  if (((byte)local_48.dwFlags & 1) == 0) {
    local_48.wShowWindow = 10;
  }
  return local_48.wShowWindow;
}



/* Function ___scrt_is_managed_app @ 00401a4b */

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



/* Function FUN_00401a8f @ 00401a8f */

void FUN_00401a8f(void)

{
  SetUnhandledExceptionFilter(FUN_00401a9b);
  return;
}



/* Function FUN_00401a9b @ 00401a9b */

undefined4 FUN_00401a9b(int *param_1)

{
  int *piVar1;
  int iVar2;
  
  piVar1 = (int *)*param_1;
  if (((*piVar1 == -0x1f928c9d) && (piVar1[4] == 3)) &&
     ((iVar2 = piVar1[5], iVar2 == 0x19930520 ||
      (((iVar2 == 0x19930521 || (iVar2 == 0x19930522)) || (iVar2 == 0x1994000)))))) {
                    /* WARNING: Subroutine does not return */
    terminate();
  }
  return 0;
}



/* Function guard_check_icall @ 00401adc */

/* WARNING: Switch with 1 destination removed at 0x00401adc */

void __cdecl guard_check_icall(void)

{
  return;
}



/* Function ___isa_available_init @ 00401ae2 */

/* WARNING: Removing unreachable block (ram,0x00401b23) */
/* WARNING: Removing unreachable block (ram,0x00401bd3) */
/* WARNING: Removing unreachable block (ram,0x00401b5d) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* Library Function - Single Match
    ___isa_available_init
   
   Library: Visual Studio 2015 Release */

undefined4 ___isa_available_init(void)

{
  int *piVar1;
  uint *puVar2;
  int iVar3;
  uint uVar4;
  BOOL BVar5;
  uint uVar6;
  uint uVar7;
  uint in_XCR0;
  uint local_14;
  
  _DAT_004042d8 = 0;
  DAT_00404018 = DAT_00404018 | 1;
  BVar5 = IsProcessorFeaturePresent(10);
  uVar4 = DAT_00404018;
  if (BVar5 != 0) {
    local_14 = 0;
    DAT_00404018 = DAT_00404018 | 2;
    _DAT_004042d8 = 1;
    piVar1 = (int *)cpuid_basic_info(0);
    puVar2 = (uint *)cpuid_Version_info(1);
    uVar7 = puVar2[3];
    if (((piVar1[2] == 0x49656e69 && piVar1[3] == 0x6c65746e) && piVar1[1] == 0x756e6547) &&
       (((((uVar6 = *puVar2 & 0xfff3ff0, uVar6 == 0x106c0 || (uVar6 == 0x20660)) ||
          (uVar6 == 0x20670)) || ((uVar6 == 0x30650 || (uVar6 == 0x30660)))) || (uVar6 == 0x30670)))
       ) {
      DAT_004042dc = DAT_004042dc | 1;
    }
    if (6 < *piVar1) {
      iVar3 = cpuid_Extended_Feature_Enumeration_info(7);
      local_14 = *(uint *)(iVar3 + 4);
      if ((local_14 & 0x200) != 0) {
        DAT_004042dc = DAT_004042dc | 2;
      }
    }
    if ((uVar7 & 0x100000) != 0) {
      DAT_00404018 = uVar4 | 6;
      _DAT_004042d8 = 2;
      if ((((uVar7 & 0x8000000) != 0) && ((uVar7 & 0x10000000) != 0)) && ((in_XCR0 & 6) == 6)) {
        DAT_00404018 = uVar4 | 0xe;
        _DAT_004042d8 = 3;
        if ((local_14 & 0x20) != 0) {
          DAT_00404018 = uVar4 | 0x2e;
          _DAT_004042d8 = 5;
        }
      }
    }
  }
  return 0;
}



/* Function FUN_00401c7c @ 00401c7c */

undefined4 FUN_00401c7c(void)

{
  return 1;
}



/* Function ___scrt_is_ucrt_dll_in_use @ 00401c80 */

/* Library Function - Single Match
    ___scrt_is_ucrt_dll_in_use
   
   Library: Visual Studio 2015 Release */

bool ___scrt_is_ucrt_dll_in_use(void)

{
  return DAT_00404020 != 0;
}



/* Function ___security_init_cookie @ 00401c8c */

/* Library Function - Single Match
    ___security_init_cookie
   
   Library: Visual Studio 2015 Release */

void __cdecl ___security_init_cookie(void)

{
  DWORD DVar1;
  LARGE_INTEGER local_18;
  _FILETIME local_10;
  uint local_8;
  
  local_10.dwLowDateTime = 0;
  local_10.dwHighDateTime = 0;
  if ((DAT_00404014 == 0xbb40e64e) || ((DAT_00404014 & 0xffff0000) == 0)) {
    GetSystemTimeAsFileTime(&local_10);
    local_8 = local_10.dwHighDateTime ^ local_10.dwLowDateTime;
    DVar1 = GetCurrentThreadId();
    local_8 = local_8 ^ DVar1;
    DVar1 = GetCurrentProcessId();
    local_8 = local_8 ^ DVar1;
    QueryPerformanceCounter(&local_18);
    DAT_00404014 = local_18.s.HighPart ^ local_18.s.LowPart ^ local_8 ^ (uint)&local_8;
    if (DAT_00404014 == 0xbb40e64e) {
      DAT_00404014 = 0xbb40e64f;
    }
    else if ((DAT_00404014 & 0xffff0000) == 0) {
      DAT_00404014 = DAT_00404014 | (DAT_00404014 | 0x4711) << 0x10;
    }
    DAT_00404010 = ~DAT_00404014;
  }
  else {
    DAT_00404010 = ~DAT_00404014;
  }
  return;
}



/* Function FUN_00401d28 @ 00401d28 */

undefined4 FUN_00401d28(void)

{
  return 0x4000;
}



/* Function FUN_00401d2e @ 00401d2e */

void FUN_00401d2e(void)

{
  InitializeSListHead((PSLIST_HEADER)&DAT_004042e0);
  return;
}



/* Function FUN_00401d3a @ 00401d3a */

void FUN_00401d3a(void)

{
  code *pcVar1;
  errno_t eVar2;
  
  eVar2 = _controlfp_s((uint *)0x0,0x10000,0x30000);
  if (eVar2 == 0) {
    return;
  }
  ___scrt_fastfail();
  pcVar1 = (code *)swi(3);
  (*pcVar1)();
  return;
}



/* Function FUN_00401d5b @ 00401d5b */

undefined * FUN_00401d5b(void)

{
  return &DAT_004042e8;
}



/* Function FUN_00401d61 @ 00401d61 */

undefined * FUN_00401d61(void)

{
  return &DAT_004042f0;
}



/* Function FUN_00401d67 @ 00401d67 */

void FUN_00401d67(void)

{
  uint *puVar1;
  
  puVar1 = (uint *)FUN_00401d5b();
  *puVar1 = *puVar1 | 4;
  puVar1[1] = puVar1[1];
  puVar1 = (uint *)FUN_00401d61();
  *puVar1 = *puVar1 | 2;
  puVar1[1] = puVar1[1];
  return;
}



/* Function FUN_00401d84 @ 00401d84 */

bool FUN_00401d84(void)

{
  return DAT_0040401c == 0;
}



/* Function FUN_00401d90 @ 00401d90 */

undefined * FUN_00401d90(void)

{
  return &DAT_00404658;
}



/* Function FUN_00401d96 @ 00401d96 */

undefined * FUN_00401d96(void)

{
  return &DAT_00404654;
}



/* Function FUN_00401d9c @ 00401d9c */

/* WARNING: Removing unreachable block (ram,0x00401dac) */
/* WARNING: Removing unreachable block (ram,0x00401dad) */
/* WARNING: Removing unreachable block (ram,0x00401db3) */
/* WARNING: Removing unreachable block (ram,0x00401dbc) */
/* WARNING: Removing unreachable block (ram,0x00401dc3) */

void FUN_00401d9c(void)

{
  return;
}



/* Function FUN_00401dc7 @ 00401dc7 */

/* WARNING: Removing unreachable block (ram,0x00401dd7) */
/* WARNING: Removing unreachable block (ram,0x00401dd8) */
/* WARNING: Removing unreachable block (ram,0x00401dde) */
/* WARNING: Removing unreachable block (ram,0x00401de7) */
/* WARNING: Removing unreachable block (ram,0x00401dee) */

void FUN_00401dc7(void)

{
  return;
}



/* Function ___raise_securityfailure @ 00401df2 */

/* Library Function - Single Match
    ___raise_securityfailure
   
   Library: Visual Studio 2015 Release */

void __cdecl ___raise_securityfailure(_EXCEPTION_POINTERS *param_1)

{
  HANDLE hProcess;
  UINT uExitCode;
  
  SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)0x0);
  UnhandledExceptionFilter(param_1);
  uExitCode = 0xc0000409;
  hProcess = GetCurrentProcess();
  TerminateProcess(hProcess,uExitCode);
  return;
}



/* Function ___report_gsfailure @ 00401e1a */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* Library Function - Single Match
    ___report_gsfailure
   
   Library: Visual Studio 2015 Release */

void __cdecl ___report_gsfailure(void)

{
  code *pcVar1;
  uint uVar2;
  undefined4 extraout_ECX;
  undefined4 extraout_ECX_00;
  undefined4 uVar3;
  undefined4 extraout_EDX;
  undefined4 unaff_EBX;
  undefined4 unaff_EBP;
  undefined4 unaff_ESI;
  undefined4 unaff_EDI;
  undefined2 in_ES;
  undefined2 in_CS;
  undefined2 in_SS;
  undefined2 in_DS;
  undefined2 in_FS;
  undefined2 in_GS;
  byte bVar4;
  byte bVar5;
  byte in_AF;
  byte bVar6;
  byte bVar7;
  byte in_TF;
  byte in_IF;
  byte bVar8;
  byte in_NT;
  byte in_AC;
  byte in_VIF;
  byte in_VIP;
  byte in_ID;
  undefined8 uVar9;
  undefined4 unaff_retaddr;
  
  uVar2 = IsProcessorFeaturePresent(0x17);
  uVar9 = CONCAT44(extraout_EDX,uVar2);
  bVar4 = 0;
  bVar8 = 0;
  bVar7 = (int)uVar2 < 0;
  bVar6 = uVar2 == 0;
  bVar5 = (POPCOUNT(uVar2 & 0xff) & 1U) == 0;
  uVar3 = extraout_ECX;
  if (!(bool)bVar6) {
    pcVar1 = (code *)swi(0x29);
    uVar9 = (*pcVar1)();
    uVar3 = extraout_ECX_00;
  }
  _DAT_004043f0 = (undefined4)((ulonglong)uVar9 >> 0x20);
  _DAT_004043f8 = (undefined4)uVar9;
  _DAT_00404408 =
       (uint)(in_NT & 1) * 0x4000 | (uint)(bVar8 & 1) * 0x800 | (uint)(in_IF & 1) * 0x200 |
       (uint)(in_TF & 1) * 0x100 | (uint)(bVar7 & 1) * 0x80 | (uint)(bVar6 & 1) * 0x40 |
       (uint)(in_AF & 1) * 0x10 | (uint)(bVar5 & 1) * 4 | (uint)(bVar4 & 1) |
       (uint)(in_ID & 1) * 0x200000 | (uint)(in_VIP & 1) * 0x100000 | (uint)(in_VIF & 1) * 0x80000 |
       (uint)(in_AC & 1) * 0x40000;
  _DAT_0040440c = &stack0x00000004;
  _DAT_00404348 = 0x10001;
  _DAT_004042f8 = 0xc0000409;
  _DAT_004042fc = 1;
  _DAT_00404308 = 1;
  DAT_0040430c = 2;
  _DAT_00404304 = unaff_retaddr;
  _DAT_004043d4 = in_GS;
  _DAT_004043d8 = in_FS;
  _DAT_004043dc = in_ES;
  _DAT_004043e0 = in_DS;
  _DAT_004043e4 = unaff_EDI;
  _DAT_004043e8 = unaff_ESI;
  _DAT_004043ec = unaff_EBX;
  _DAT_004043f4 = uVar3;
  _DAT_004043fc = unaff_EBP;
  DAT_00404400 = unaff_retaddr;
  _DAT_00404404 = in_CS;
  _DAT_00404410 = in_SS;
  ___raise_securityfailure((_EXCEPTION_POINTERS *)&PTR_DAT_004031e4);
  return;
}



/* Function CAtlBaseModule @ 00401f15 */

/* Library Function - Single Match
    public: __thiscall ATL::CAtlBaseModule::CAtlBaseModule(void)
   
   Libraries: Visual Studio 2015 Release, Visual Studio 2017 Release, Visual Studio 2019 Release */

CAtlBaseModule * __thiscall ATL::CAtlBaseModule::CAtlBaseModule(CAtlBaseModule *this)

{
  uint uVar1;
  BOOL BVar2;
  
  _ATL_BASE_MODULE70::_ATL_BASE_MODULE70((_ATL_BASE_MODULE70 *)this);
  *(undefined4 *)this = 0x38;
  *(undefined4 *)(this + 8) = 0x400000;
  *(undefined4 *)(this + 4) = 0x400000;
  *(undefined4 *)(this + 0xc) = 0xe00;
  *(undefined **)(this + 0x10) = &DAT_004031ec;
  uVar1 = FUN_00401050((LPCRITICAL_SECTION)(this + 0x14));
  if ((int)uVar1 < 0) {
    BVar2 = IsDebuggerPresent();
    if (BVar2 != 0) {
      OutputDebugStringW(L"ERROR : Unable to initialize critical section in CAtlBaseModule\n");
    }
    DAT_00404650 = 1;
  }
  return this;
}



/* Function _ATL_BASE_MODULE70 @ 00401f68 */

/* Library Function - Single Match
    public: __thiscall ATL::_ATL_BASE_MODULE70::_ATL_BASE_MODULE70(void)
   
   Libraries: Visual Studio 2015 Release, Visual Studio 2017 Release, Visual Studio 2019 Release */

_ATL_BASE_MODULE70 * __thiscall
ATL::_ATL_BASE_MODULE70::_ATL_BASE_MODULE70(_ATL_BASE_MODULE70 *this)

{
  memset(this + 0x14,0,0x18);
  *(undefined4 *)(this + 0x2c) = 0;
  *(undefined4 *)(this + 0x30) = 0;
  *(undefined4 *)(this + 0x34) = 0;
  return this;
}



/* Function ~CAtlBaseModule @ 00401f8b */

/* Library Function - Single Match
    public: __thiscall ATL::CAtlBaseModule::~CAtlBaseModule(void)
   
   Libraries: Visual Studio 2012 Release, Visual Studio 2015 Release, Visual Studio 2017 Release,
   Visual Studio 2019 Release */

void __thiscall ATL::CAtlBaseModule::~CAtlBaseModule(CAtlBaseModule *this)

{
  DeleteCriticalSection((LPCRITICAL_SECTION)(this + 0x14));
  CSimpleArray<struct_HINSTANCE__*,class_ATL::CSimpleArrayEqualHelper<struct_HINSTANCE__*>_>::
  RemoveAll((CSimpleArray<struct_HINSTANCE__*,class_ATL::CSimpleArrayEqualHelper<struct_HINSTANCE__*>_>
             *)(this + 0x2c));
  return;
}



/* Function RemoveAll @ 00401fa1 */

/* Library Function - Single Match
    public: void __thiscall ATL::CSimpleArray<struct HINSTANCE__ *,class
   ATL::CSimpleArrayEqualHelper<struct HINSTANCE__ *> >::RemoveAll(void)
   
   Libraries: Visual Studio 2012 Release, Visual Studio 2015 Release, Visual Studio 2017 Release,
   Visual Studio 2019 Release */

void __thiscall
ATL::CSimpleArray<struct_HINSTANCE__*,class_ATL::CSimpleArrayEqualHelper<struct_HINSTANCE__*>_>::
RemoveAll(CSimpleArray<struct_HINSTANCE__*,class_ATL::CSimpleArrayEqualHelper<struct_HINSTANCE__*>_>
          *this)

{
  if (*(int *)this != 0) {
    free(*(void **)this);
    *(undefined4 *)this = 0;
  }
  *(undefined4 *)(this + 4) = 0;
  *(undefined4 *)(this + 8) = 0;
  return;
}



/* Function memset @ 00401fbe */

void * __cdecl memset(void *_Dst,int _Val,size_t _Size)

{
  void *pvVar1;
  
                    /* WARNING: Could not recover jumptable at 0x00401fbe. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  pvVar1 = memset(_Dst,_Val,_Size);
  return pvVar1;
}



/* Function __CxxFrameHandler3 @ 00401fc4 */

void __CxxFrameHandler3(void)

{
                    /* WARNING: Could not recover jumptable at 0x00401fc4. Too many branches */
                    /* WARNING: Subroutine does not return */
                    /* WARNING: Treating indirect jump as call */
  __CxxFrameHandler3();
  return;
}



/* Function __vcrt_InitializeCriticalSectionEx @ 00401fca */

void __vcrt_InitializeCriticalSectionEx(void)

{
                    /* WARNING: Could not recover jumptable at 0x00401fca. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  __vcrt_InitializeCriticalSectionEx();
  return;
}



/* Function except_handler4_common @ 00401fd0 */

void __cdecl except_handler4_common(void)

{
                    /* WARNING: Could not recover jumptable at 0x00401fd0. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  except_handler4_common();
  return;
}



/* Function free @ 00401fd6 */

void __cdecl free(void *_Memory)

{
                    /* WARNING: Could not recover jumptable at 0x00401fd6. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  free(_Memory);
  return;
}



/* Function _configure_narrow_argv @ 00401fdc */

void _configure_narrow_argv(void)

{
                    /* WARNING: Could not recover jumptable at 0x00401fdc. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _configure_narrow_argv();
  return;
}



/* Function _initialize_narrow_environment @ 00401fe2 */

void _initialize_narrow_environment(void)

{
                    /* WARNING: Could not recover jumptable at 0x00401fe2. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _initialize_narrow_environment();
  return;
}



/* Function initialize_onexit_table @ 00401fe8 */

void __cdecl initialize_onexit_table(void)

{
                    /* WARNING: Could not recover jumptable at 0x00401fe8. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  initialize_onexit_table();
  return;
}



/* Function register_onexit_function @ 00401fee */

void __cdecl register_onexit_function(void)

{
                    /* WARNING: Could not recover jumptable at 0x00401fee. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  register_onexit_function();
  return;
}



/* Function crt_atexit @ 00401ff4 */

void __cdecl crt_atexit(void)

{
                    /* WARNING: Could not recover jumptable at 0x00401ff4. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  crt_atexit();
  return;
}



/* Function _cexit @ 00401ffa */

void __cdecl _cexit(void)

{
                    /* WARNING: Could not recover jumptable at 0x00401ffa. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _cexit();
  return;
}



/* Function _seh_filter_exe @ 00402000 */

void _seh_filter_exe(void)

{
                    /* WARNING: Could not recover jumptable at 0x00402000. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _seh_filter_exe();
  return;
}



/* Function _set_app_type @ 00402006 */

void _set_app_type(void)

{
                    /* WARNING: Could not recover jumptable at 0x00402006. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _set_app_type();
  return;
}



/* Function __setusermatherr @ 0040200c */

void __setusermatherr(void)

{
                    /* WARNING: Could not recover jumptable at 0x0040200c. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  __setusermatherr();
  return;
}



/* Function get_narrow_winmain_command_line @ 00402012 */

void __cdecl get_narrow_winmain_command_line(void)

{
                    /* WARNING: Could not recover jumptable at 0x00402012. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  get_narrow_winmain_command_line();
  return;
}



/* Function initterm @ 00402018 */

void __cdecl initterm(void)

{
                    /* WARNING: Could not recover jumptable at 0x00402018. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  initterm();
  return;
}



/* Function initterm_e @ 0040201e */

void __cdecl initterm_e(void)

{
                    /* WARNING: Could not recover jumptable at 0x0040201e. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  initterm_e();
  return;
}



/* Function exit @ 00402024 */

void __cdecl exit(int _Code)

{
                    /* WARNING: Could not recover jumptable at 0x00402024. Too many branches */
                    /* WARNING: Subroutine does not return */
                    /* WARNING: Treating indirect jump as call */
  exit(_Code);
  return;
}



/* Function _exit @ 0040202a */

void __cdecl _exit(int _Code)

{
                    /* WARNING: Could not recover jumptable at 0x0040202a. Too many branches */
                    /* WARNING: Subroutine does not return */
                    /* WARNING: Treating indirect jump as call */
  _exit(_Code);
  return;
}



/* Function _set_fmode @ 00402030 */

errno_t __cdecl _set_fmode(int _Mode)

{
  errno_t eVar1;
  
                    /* WARNING: Could not recover jumptable at 0x00402030. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  eVar1 = _set_fmode(_Mode);
  return eVar1;
}



/* Function _c_exit @ 00402036 */

void __cdecl _c_exit(void)

{
                    /* WARNING: Could not recover jumptable at 0x00402036. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _c_exit();
  return;
}



/* Function register_thread_local_exe_atexit_callback @ 0040203c */

void __cdecl register_thread_local_exe_atexit_callback(void)

{
                    /* WARNING: Could not recover jumptable at 0x0040203c. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  register_thread_local_exe_atexit_callback();
  return;
}



/* Function _configthreadlocale @ 00402042 */

int __cdecl _configthreadlocale(int _Flag)

{
  int iVar1;
  
                    /* WARNING: Could not recover jumptable at 0x00402042. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  iVar1 = _configthreadlocale(_Flag);
  return iVar1;
}



/* Function _set_new_mode @ 00402048 */

void _set_new_mode(void)

{
                    /* WARNING: Could not recover jumptable at 0x00402048. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  _set_new_mode();
  return;
}



/* Function __p__commode @ 0040204e */

void __p__commode(void)

{
                    /* WARNING: Could not recover jumptable at 0x0040204e. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  __p__commode();
  return;
}



/* Function terminate @ 00402054 */

void terminate(void)

{
                    /* WARNING: Could not recover jumptable at 0x00402054. Too many branches */
                    /* WARNING: Subroutine does not return */
                    /* WARNING: Treating indirect jump as call */
  terminate();
  return;
}



/* Function _controlfp_s @ 0040205a */

errno_t __cdecl _controlfp_s(uint *_CurrentState,uint _NewValue,uint _Mask)

{
  errno_t eVar1;
  
                    /* WARNING: Could not recover jumptable at 0x0040205a. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  eVar1 = _controlfp_s(_CurrentState,_NewValue,_Mask);
  return eVar1;
}



/* Function IsProcessorFeaturePresent @ 00402060 */

BOOL IsProcessorFeaturePresent(DWORD ProcessorFeature)

{
  BOOL BVar1;
  
                    /* WARNING: Could not recover jumptable at 0x00402060. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  BVar1 = IsProcessorFeaturePresent(ProcessorFeature);
  return BVar1;
}



/* Function operator_delete @ 00402066 */

void CNoTrackObject::operator_delete(void *param_1)

{
                    /* WARNING: Could not recover jumptable at 0x00402066. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  operator_delete(param_1);
  return;
}



/* Function AFX_MODULE_STATE @ 0040206c */

void __thiscall
AFX_MODULE_STATE::AFX_MODULE_STATE
          (AFX_MODULE_STATE *this,int param_1,_func_long_HWND___ptr_uint_uint_long *param_2,
          ulong param_3,int param_4)

{
                    /* WARNING: Could not recover jumptable at 0x0040206c. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  AFX_MODULE_STATE(this,param_1,param_2,param_3,param_4);
  return;
}



/* Function ~AFX_MODULE_STATE @ 00402072 */

void __thiscall AFX_MODULE_STATE::~AFX_MODULE_STATE(AFX_MODULE_STATE *this)

{
                    /* WARNING: Could not recover jumptable at 0x00402072. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  ~AFX_MODULE_STATE(this);
  return;
}



/* Function AFX_MAINTAIN_STATE2 @ 00402078 */

void __thiscall
AFX_MAINTAIN_STATE2::AFX_MAINTAIN_STATE2(AFX_MAINTAIN_STATE2 *this,AFX_MODULE_STATE *param_1)

{
                    /* WARNING: Could not recover jumptable at 0x00402078. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  AFX_MAINTAIN_STATE2(this,param_1);
  return;
}



/* Function ~AFX_MAINTAIN_STATE2 @ 0040207e */

void __thiscall AFX_MAINTAIN_STATE2::~AFX_MAINTAIN_STATE2(AFX_MAINTAIN_STATE2 *this)

{
                    /* WARNING: Could not recover jumptable at 0x0040207e. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  ~AFX_MAINTAIN_STATE2(this);
  return;
}



/* Function AfxWndProc @ 00402084 */

long AfxWndProc(HWND__ *param_1,uint param_2,uint param_3,long param_4)

{
  long lVar1;
  
                    /* WARNING: Could not recover jumptable at 0x00402084. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  lVar1 = AfxWndProc(param_1,param_2,param_3,param_4);
  return lVar1;
}



/* Function FUN_0040208a @ 0040208a */

undefined1 FUN_0040208a(void)

{
  return 1;
}



/* Function Unwind@00402090 @ 00402090 */

void Unwind_00402090(void)

{
  int unaff_EBP;
  
  AFX_MAINTAIN_STATE2::~AFX_MAINTAIN_STATE2((AFX_MAINTAIN_STATE2 *)(unaff_EBP + -0x14));
  return;
}



/* Function FUN_004020ca @ 004020ca */

void FUN_004020ca(void)

{
  ATL::CAtlBaseModule::~CAtlBaseModule((CAtlBaseModule *)&DAT_00404614);
  return;
}



