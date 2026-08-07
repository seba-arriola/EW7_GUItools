#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#  $Id: make_ora_api.sol,v 1.16 2001/01/19 17:50:35 davidk Exp $
#  Revision history:
#
#  $Log: make_ora_api.sol,v $
#  Revision 1.16  2001/01/19 17:50:35  davidk
#  added entries for the following objects:
#        p3s2_create_snipreq
#        p3s2_delete_snipreq
#        p3s2_get_snipreq_list
#        p3s2_update_snipreq
#        p3s2_get_waveform_desc
#
#  Revision 1.15  2000/12/20 23:54:57  davidk
#  removed reference to p3s2_load_hinvarc(copied from lucky's version), and added
#  line fro p3s2_get_station_list_without_location
#
#  Revision 1.14  2000/03/30 19:04:52  davidk
#  removed modules that dealt with USNSN station list and EW station list
#  routines, and replaced them with Exernal Station list routines.
#  Added new p3s2_load_hinvarc object and added chron3 and read_arc to
#  the list of earthworm libraries used.  Changed syntax on the include
#  and on the ALL_LIB_OBJECTS param.
#
#  Revision 1.13  2000/03/15 08:46:53  davidk
#  added support for an APP_INCLUDE_FLAGS variable that is included
#  in the compiler flags issues to compile a .c into a .o.  This way
#  the client makefile has a method of specifying special compile flags.
#  Previously there was no way to specify extra compiler flags from the
#  app's (client's) makefile.  Now you can specify -DXXXX flags to define
#  preprocessor values as well as other flags.
#
#  Revision 1.12  2000/02/15 19:20:35  lucky
#  added a whole bunch of infrastructure api calls
#
#  Revision 1.11  2000/02/15 18:45:42  lucky
#  *** empty log message ***
#
#  Revision 1.10  2000/01/11 19:27:44  lucky
#  *** empty log message ***
#
#  Revision 1.9  2000/01/04 18:55:40  lucky
#  CHanged EARTHWORM_API_O definition so that it now adds to previous definition
#  instead of replacing it. This is necessary for those modules that may wish
#  to include some more "exotic" ew functionality
#
#  Revision 1.8  2000/01/04 18:53:13  lucky
#  added waveform functions and two ew functions related to the snippets
#
#  Revision 1.7  1999/12/10 00:48:44  davidk
#  added API link entries for two new strong motion objects.
#
#  Revision 1.6  1999/11/09 18:42:46  lucky
#  *** empty log message ***
#
#  Revision 1.5  1999/11/05 01:04:53  davidk
#  added p3s2_get_all_smmessages_by_scnlt object for API
#
#  Revision 1.4  1999/11/03 22:31:24  davidk
#  added an additional strong motion object to be compiled
#
#  Revision 1.3  1999/10/27 15:48:35  lucky
#  *** empty log message ***
#
#  Revision 1.2  1999/10/19 05:05:48  davidk
#  added several new statement files to the list of API objects
#
#  Revision 1.1  1999/10/15 17:51:10  lucky
#  Initial revision
#
#  Revision 1.2  1999/09/02 20:17:56  davidk
#  updated for new directory structure
#
#  Revision 1.1  1999/05/05 18:41:49  lucky
#  Initial revision
#
#
 
#       make File For ora_api - Solaris version
#
#       This is a brand spanking new makefile for PhaseIII.
#       This is the makefile for the ora_api and is called
#       or included by all Application makefiles that utilize
#       the API.  Please be careful making changes, for you
#       might break more than you bargained for.  DK 04/19/99

###############################
#EW_ORACLE_VERSION not currently set
###############################
EW_ORACLE_VERSION = 

INCLUDE_FLAGS = $(GLOBALFLAGS) $(APP_INCLUDE_FLAGS)

###############################
#ORA_API object files
###############################
BASE_OCI_LIB_O = $Ap3s2_oci_base$(OBJ)

EW_BASE_OCI_LIB_O \
	       = $Ap3s2_ew_oci_base$(OBJ)

API_LIB_O      = $Ap3db_ora_api$(OBJ) 

API_SQL_LIB_O  = $Ap3s2_get_event_list$(OBJ) $Ap3s2_create_event$(OBJ) \
                 $Ap3s2_get_station_list$(OBJ) $Ap3s2_create_origin$(OBJ) \
		 $Ap3s2_create_magnitude$(OBJ) $Ap3s2_create_arrival$(OBJ) \
		 $Ap3s2_get_idchan_from_station_external$(OBJ) \
		 $Ap3s2_create_bodyamp$(OBJ) $Ap3s2_create_sta_mag$(OBJ) \
		 $Ap3s2_create_durcoda$(OBJ) $Ap3s2_create_tcoda$(OBJ) \
		 $Ap3s2_get_event_info$(OBJ) $Ap3s2_get_origin$(OBJ) \
		 $Ap3s2_get_component_info$(OBJ) $Ap3s2_get_stamags$(OBJ) \
		 $Ap3s2_get_arrivals$(OBJ) $Ap3s2_get_magnitude$(OBJ) \
		 $Ap3s2_get_preferred_summary_info$(OBJ)  \
		 $Ap3s2_create_or_alter_external_station$(OBJ) \
		 $Ap3s2_create_smmessage$(OBJ) \
		 $Ap3s2_get_idsmbox_from_serialnum$(OBJ) \
		 $Ap3s2_create_smmotion$(OBJ) \
		 $Ap3s2_get_smbox_info$(OBJ) \
		 $Ap3s2_get_all_smmessages_by_time$(OBJ) \
		 $Ap3s2_get_smchannels_info$(OBJ) \
		 $Ap3s2_get_all_smmessages_by_time_and_location$(OBJ) \
		 $Ap3s2_get_sminfo$(OBJ) \
		 $Ap3s2_get_idchans_from_scnlt$(OBJ) \
		 $Ap3s2_get_all_smmessages_by_scnlt$(OBJ) \
		 $Ap3s2_get_sminfo_for_channel$(OBJ) \
		 $Ap3s2_get_all_smchans$(OBJ) \
		 $Ap3s2_create_compt_for_scnlt$(OBJ) \
		 $Ap3s2_create_snippet$(OBJ) \
		 $Ap3s2_create_waveform_desc$(OBJ) \
		 $Ap3s2_get_snippet$(OBJ) \
		 $Ap3s2_get_waveform_list$(OBJ) \
		 $Ap3s2_get_waveform_list_w_compt$(OBJ) \
		 $Ap3s2_cooked_infra$(OBJ) \
		 $Ap3s2_set_comp_params$(OBJ) \
		 $Ap3s2_assoc_chan_w_comp$(OBJ) \
		 $Ap3s2_create_channel$(OBJ) \
		 $Ap3s2_create_component$(OBJ) \
		 $Ap3s2_create_trans_func$(OBJ) \
		 $Ap3s2_get_chanctf_for_channel$(OBJ) \
		 $Ap3s2_get_idchant$(OBJ) \
		 $Ap3s2_get_trans_function$(OBJ) \
		 $Ap3s2_get_trans_function_desc$(OBJ) \
		 $Ap3s2_set_chan_params$(OBJ) \
		 $Ap3s2_set_trans_func_fct$(OBJ) \
		 $Ap3s2_update_comment$(OBJ) \
		 $Ap3s2_delete_event_data$(OBJ) \
                 $Ap3s2_get_station_list_without_location$(OBJ) \
		 $Ap3s2_create_snipreq$(OBJ) \
		 $Ap3s2_delete_snipreq$(OBJ) \
		 $Ap3s2_get_snipreq_list$(OBJ) \
		 $Ap3s2_update_snipreq$(OBJ) \
		 $Ap3s2_get_waveform_desc$(OBJ) 

EARTHWORM_API_O    += $Llogit_mt$(OBJ) $Lsleep_ew$(OBJ) $Lthreads_ew$(OBJ) \
                      $Ltime_ew$(OBJ) $Lsema_ew$(OBJ) $Lkom$(OBJ) $Lread_arc$(OBJ) \
                      $Lchron3$(OBJ)

ALL_ORA_API_LIBS = $(BASE_OCI_LIB_O) $(EW_BASE_OCI_LIB_O) $(API_LIB_O) \
                   $(API_SQL_LIB_O)  $(EARTHWORM_API_O)

###############################
#All object files for this app.
###############################
ALL_APP_OBJECTS =  $(APP_OBJECTS)

ALL_LIB_OBJECTS = $(ALL_ORA_API_LIBS) \
                   $(ALL_CLIENT_LIBS)


MAKE_DEFAULT :
	make -f makefile.sol $(APP)

$(APP): $(ALL_APP_OBJECTS)
	make -f makefile.sol build EXE=$B$(APP) OBJS="$(ALL_APP_OBJECTS)" APP_OTHER="$(APP_OTHER)" LIB_OBJS="$(ALL_LIB_OBJECTS)"

ORA_API_LIB: $(ALL_LIB_OBJECTS)
	cp *$(OBJ) $(P3_LIB)
	

include $A../src/makefile.sol

