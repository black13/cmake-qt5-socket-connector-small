
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

