(*
* LICENSE
*
* Copyright (c) 2010 Vladimir Bochkarev.
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the author be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This license may not be removed or altered from any source distribution.
*
* --------------------------------------------------------------------------
* Vladimir Bochkarev <boxa@mail.ru>.
*)

library FastMMx64;

uses
  Windows, FastMM4;

{x$R *.res}

{$SETPEFLAGS IMAGE_FILE_LARGE_ADDRESS_AWARE or IMAGE_FILE_DEBUG_STRIPPED or
  IMAGE_FILE_LINE_NUMS_STRIPPED or IMAGE_FILE_LOCAL_SYMS_STRIPPED}

{$IMAGEBASE $00D20000}

{$Include FastMM4Options.inc}

function FastMM_malloc(ASize: Integer): Pointer; cdecl;
begin
  Result := GetMemory(ASize);
end;

procedure FastMM_free(APointer: Pointer); cdecl;
begin
  FreeMem(APointer);
end;

exports
  FastMM_malloc,
  FastMM_free;

begin
  IsMultiThread := True;
  DisableThreadLibraryCalls(HInstance); // not needed for current MaNGOS, but not superfluous in future :)
end.
