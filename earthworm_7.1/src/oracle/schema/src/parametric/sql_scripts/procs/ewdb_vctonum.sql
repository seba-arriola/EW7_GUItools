/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


CREATE OR REPLACE FUNCTION ewdb_VCtoNum(IN_string varchar) RETURN Number 
AS
  TempNum  Number;
BEGIN
  TempNum := TO_NUMBER(IN_string);
  if TempNum = 0 then
    if ltrim(IN_string) IS NULL then
      TempNum := NULL;
    end if;
  end if;
  return(TempNum);

EXCEPTION
  WHEN OTHERS THEN
    return(NULL);
END ewdb_VCtoNum;


