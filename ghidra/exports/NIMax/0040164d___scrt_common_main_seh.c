
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

