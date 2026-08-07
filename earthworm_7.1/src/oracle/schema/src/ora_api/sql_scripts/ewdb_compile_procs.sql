
create or replace procedure  Compile_Procs
(test1 number)
as

begin
  add_polesandzeroes_for_ctf();
	assoc_comp_w_chan();
	assoc_ctf();
	create_arrival();
	create_chan();
	create_chant();
	create_codaamp();
	create_coincidence();
	create_comp();
	create_compt();
	create_compt_for_scnlt();
	create_comp_from_scnl();
	create_ctf();
	create_event();
	create_installation();
	create_magnitude();
	create_mechfm();
	create_merge();
	create_origin();
	create_originpick();
	create_phase();
	create_phenomenon();
	create_pick();
	create_pick_for_mech();
	create_poleorzero();
	create_site();
	create_sitet();
	create_smmessage();
	create_smmotion();
	create_snipreq();
	create_source();
	create_tcoda();
	create_uhinfo();
	create_unassoc_pick();
	create_waveform_desc();
	delete_event();
	get_idchan_from_ext_stationid();
  get_idevent_from_externalid();
  get_idmag_from_sourceeventid();
  set_site_params();
  station_external_2_chan();
  update_chant();
  update_chants_for_timerange();
  update_ctf();
  update_origin_by_xidexternal();
  delete_waveforms_before_time();
  get_idpick();
  get_idorigin_from_source_data();
  get_idinst_from_ewinstid();
  
END;

