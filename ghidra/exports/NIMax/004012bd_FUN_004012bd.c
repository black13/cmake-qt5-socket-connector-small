
uint __cdecl FUN_004012bd(uint param_1)

{
  byte bVar1;
  
  bVar1 = 0x20 - ((byte)DAT_00404014 & 0x1f) & 0x1f;
  return (param_1 >> bVar1 | param_1 << 0x20 - bVar1) ^ DAT_00404014;
}

