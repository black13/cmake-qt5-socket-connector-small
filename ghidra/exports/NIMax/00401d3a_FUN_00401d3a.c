
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

