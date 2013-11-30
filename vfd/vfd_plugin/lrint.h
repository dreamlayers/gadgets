#if (defined (WIN32) || defined (_WIN32)) && !defined(_LRINT_H_)
#define _LRINT_H_

/* Win32 doesn't seem to have these functions.
** Therefore implement inline versions of these functions here.
*/
__inline long int
lrint (double flt)
{
  int intgr;
  _asm
  {
      fld flt
      fistp intgr
  } ;
  return intgr ;
}

__inline long int
lrintf (float flt)
{
   int intgr;
   _asm
  {
    fld flt
    fistp intgr
  } ;
  return intgr ;
}
#endif