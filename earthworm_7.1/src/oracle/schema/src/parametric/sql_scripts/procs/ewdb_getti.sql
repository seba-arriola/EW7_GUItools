/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


CREATE OR REPLACE FUNCTION GetTI
(IN_sTableName varchar) RETURN number
as

/***************
 Would have liked to put some debugging stuff in
 here, but since it is a function called in oracle
 sql statements, the pl/sql compiler will not let
 me do it.
 davidk 000114
***************************/
  tiTemp number;
  Temp   number;
begin
  select idTable into tiTemp from ewdb_tablelist
    where sTableName=IN_sTableName;
  return(tiTemp);
EXCEPTION
  WHEN OTHERS THEN
    return(-1);
END;
