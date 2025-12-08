
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

