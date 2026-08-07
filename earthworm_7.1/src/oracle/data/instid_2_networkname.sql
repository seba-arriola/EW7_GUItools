variable Temp_idSource number;
begin

  Get_idSource(:Temp_idSource,'014024002');
  update source set sHumanReadable='Pacific Northwest Seismic Network (UW) - Automatic'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'014024003');
  update source set sHumanReadable='Northern California Seismic Network (NC) - Automatic'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'014024005');
  update source set sHumanReadable='University of Utah Seismograph Stations (UU) - Automatic' 
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'014024015');
  update source set sHumanReadable='Hawaiian Volcano Observatory Seismic Network (HV) - Automatic'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'014024020');
  update source set sHumanReadable='Alaska Volcano Observatory Network - Anchorage (AK) - Automatic'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'014024021');
  update source set sHumanReadable='Montana Seismic Network (MB) - Automatic'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'014018021');
  update source set sHumanReadable='Montana Seismic Network (MB) - Automatic'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'014024006');
  update source set sHumanReadable='New Madrid Seismic Network, CERI, Memphis (NM) - Automatic'
    where idSource=:Temp_idSource;




  Get_idSource(:Temp_idSource,'REV_014024002');
  update source set sHumanReadable='Pacific Northwest Seismic Network (UW) - Reviewed'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_014024003');
  update source set sHumanReadable='Northern California Seismic Network (NC) - Reviewed'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_014024005');
  update source set sHumanReadable='University of Utah Seismograph Stations (UU) - Reviewed' 
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_014024015');
  update source set sHumanReadable='Hawaiian Volcano Observatory Seismic Network (HV) - Reviewed'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_014024020');
  update source set sHumanReadable='Alaska Volcano Observatory Network - Anchorage (AK) - Reviewed'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_014024021');
  update source set sHumanReadable='Montana Seismic Network (MB) - Reviewed'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_014018021');
  update source set sHumanReadable='Montana Seismic Network (MB) - Reviewed'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_014024006');
  update source set sHumanReadable='New Madrid Seismic Network, CERI, Memphis (NM) - Reviewed'
    where idSource=:Temp_idSource;



  Get_idSource(:Temp_idSource,'USNSN');
  update source set sHumanReadable='USNSN - Automatic'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_USNSN');
  update source set sHumanReadable='USNSN - Reviewed'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'DEWEY');
  update source set sHumanReadable='Dewey Project'
    where idSource=:Temp_idSource;

  Get_idSource(:Temp_idSource,'REV_DEWEY');
  update source set sHumanReadable='Dewey Project (Reviewed)'
    where idSource=:Temp_idSource;

end;
/
commit;


