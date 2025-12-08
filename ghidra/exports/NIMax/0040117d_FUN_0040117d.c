
AFX_MODULE_STATE * __thiscall FUN_0040117d(void *this,byte param_1)

{
  AFX_MODULE_STATE::~AFX_MODULE_STATE((AFX_MODULE_STATE *)this);
  if ((param_1 & 1) != 0) {
    if ((param_1 & 4) == 0) {
      CNoTrackObject::operator_delete(this);
    }
    else {
      guard_check_icall();
    }
  }
  return (AFX_MODULE_STATE *)this;
}

