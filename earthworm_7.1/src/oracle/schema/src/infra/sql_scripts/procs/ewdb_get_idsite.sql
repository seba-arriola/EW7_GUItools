/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE Get_idSite
(
 OUT_RetCode OUT number,
 OUT_idSite OUT number,
 IN_sSta varchar,
 IN_sNet varchar,
 IN_sComment varchar
)
as

  /**********************************
    Return Codes:
      0:   Success;  idSite found and returned
      1:   Success;  matching Site not found, new Site created
     -1:   Failure;  unknown error
     -2:   Failure;  Create_Site() failed.
   **********************************/

Temp               number;

begin
  /**********************************/
  /* Get the ID for the Site        */
  /**********************************/
  select idSite into OUT_idSite from Site 
    where sSta = IN_sSta
      and sNet = IN_sNet;

  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    Create_Site(Temp,OUT_idSite,IN_sSta,IN_sNet,IN_sComment);
    if(OUT_idSite > 0) then
      OUT_RetCode := 1;
    else
      OUT_RetCode := -2;
    end if;


  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Get_idSite ' || IN_sSta, Temp,0);
    OUT_idSite := -1;
    OUT_RetCode := -1;
END;
