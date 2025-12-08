
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

