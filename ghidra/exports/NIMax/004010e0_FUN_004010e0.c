
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

