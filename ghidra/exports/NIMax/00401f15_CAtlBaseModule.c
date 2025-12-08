
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

