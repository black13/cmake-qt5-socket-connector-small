
/* Library Function - Single Match
    ___security_init_cookie
   
   Library: Visual Studio 2015 Release */

void __cdecl ___security_init_cookie(void)

{
  DWORD DVar1;
  LARGE_INTEGER local_18;
  _FILETIME local_10;
  uint local_8;
  
  local_10.dwLowDateTime = 0;
  local_10.dwHighDateTime = 0;
  if ((DAT_00404014 == 0xbb40e64e) || ((DAT_00404014 & 0xffff0000) == 0)) {
    GetSystemTimeAsFileTime(&local_10);
    local_8 = local_10.dwHighDateTime ^ local_10.dwLowDateTime;
    DVar1 = GetCurrentThreadId();
    local_8 = local_8 ^ DVar1;
    DVar1 = GetCurrentProcessId();
    local_8 = local_8 ^ DVar1;
    QueryPerformanceCounter(&local_18);
    DAT_00404014 = local_18.s.HighPart ^ local_18.s.LowPart ^ local_8 ^ (uint)&local_8;
    if (DAT_00404014 == 0xbb40e64e) {
      DAT_00404014 = 0xbb40e64f;
    }
    else if ((DAT_00404014 & 0xffff0000) == 0) {
      DAT_00404014 = DAT_00404014 | (DAT_00404014 | 0x4711) << 0x10;
    }
    DAT_00404010 = ~DAT_00404014;
  }
  else {
    DAT_00404010 = ~DAT_00404014;
  }
  return;
}

