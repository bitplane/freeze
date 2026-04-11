#define INCL_NOPM
#define INCL_DOSNLS
#define INCL_DOSERRORS
#include <os2.h>

#include <stdlib.h>


int IsFileNameValid(char *name)
{
  HFILE hf;
  ULONG uAction;

  if (_osmode == DOS_MODE)
    return FALSE;

  switch( DosOpen(name, &hf, &uAction, 0, 0, FILE_OPEN,
                  OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE, 0) )
  {
  case ERROR_INVALID_NAME:
  case ERROR_FILENAME_EXCED_RANGE:
    return FALSE;
  case NO_ERROR:
    DosClose(hf);
  default:
    return TRUE;
  }
}
