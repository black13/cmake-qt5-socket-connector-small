
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

