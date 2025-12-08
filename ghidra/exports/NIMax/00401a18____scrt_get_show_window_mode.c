
/* Library Function - Single Match
    ___scrt_get_show_window_mode
   
   Library: Visual Studio 2015 Release */

WORD ___scrt_get_show_window_mode(void)

{
  _STARTUPINFOW local_48;
  
  memset(&local_48,0,0x44);
  GetStartupInfoW(&local_48);
  if (((byte)local_48.dwFlags & 1) == 0) {
    local_48.wShowWindow = 10;
  }
  return local_48.wShowWindow;
}

