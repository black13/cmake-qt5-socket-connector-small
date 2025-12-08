
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

