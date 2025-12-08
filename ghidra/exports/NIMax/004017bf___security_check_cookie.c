
/* Library Function - Single Match
    @__security_check_cookie@4
   
   Library: Visual Studio 2015 Release
   __fastcall __security_check_cookie,4 */

void __fastcall __security_check_cookie(int param_1)

{
  if (param_1 == DAT_00404014) {
    return;
  }
                    /* WARNING: Subroutine does not return */
  ___report_gsfailure();
}

