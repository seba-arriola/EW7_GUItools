variable Temp_idSource number;
begin

  Get_idSource(:Temp_idSource,'014024002');
  update source set sHumanReadable='UW - Automatic'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'014024003');
  update source set sHumanReadable='NC - Automatic'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'014024005');
  update source set sHumanReadable='UU - Automatic' 
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'014024015');
  update source set sHumanReadable='HV - Automatic'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'014024020');
  update source set sHumanReadable='AK - Automatic'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'014024021');
  update source set sHumanReadable='MB - Automatic'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'014018021');
  update source set sHumanReadable='MB - Automatic'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'014024006');
  update source set sHumanReadable='NM - Automatic'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'209142013 ');
  update source set sHumanReadable='GLASS - Automatic'
    where idSource=:Temp_idSource;




  Get_idSource(:Temp_idSource,'REV_014024002');
  update source set sHumanReadable='UW - Reviewed'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_014024003');
  update source set sHumanReadable='NC - Reviewed'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_014024005');
  update source set sHumanReadable='UU - Reviewed' 
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_014024015');
  update source set sHumanReadable='HV - Reviewed'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_014024020');
  update source set sHumanReadable='AK - Reviewed'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_014024021');
  update source set sHumanReadable='MB - Reviewed'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_014018021');
  update source set sHumanReadable='MB - Reviewed'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_014024006');
  update source set sHumanReadable='NM - Reviewed'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_209142013 ');
  update source set sHumanReadable='GLASS - Reviewed'
    where idSource=:Temp_idSource;



  Get_idSource(:Temp_idSource,'USNSN');
  update source set sHumanReadable='USNSN - Automatic'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_USNSN');
  update source set sHumanReadable='USNSN - Reviewed'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'DEWEY');
  update source set sHumanReadable='Dewey'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_DEWEY');
  update source set sHumanReadable='Dewey - Reviewed'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'UH');
  update source set sHumanReadable='Urban Hazards'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_UH');
  update source set sHumanReadable='Urban Hazards - Reviewed'
    where idSource=:Temp_idSource;

end;
/
commit;


