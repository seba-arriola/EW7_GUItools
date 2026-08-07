



/************************************
 Create Place Types
 ************************************/ 
insert into PlaceType values(1000000001, 1, 0, 'City - Generic');
insert into PlaceType values(1000000017, 1, 1, 'Megopolis');
insert into PlaceType values(1000000002, 1, 2, 'Metropolis');
insert into PlaceType values(1000000003, 1, 3, 'Large City');
insert into PlaceType values(1000000004, 1, 4, 'City');
insert into PlaceType values(1000000005, 1, 5, 'CDP');
insert into PlaceType values(1000000007, 4, 0, 'Fault - Generic');
insert into PlaceType values(1000000008, 4, 1, 'Fault - Large');
insert into PlaceType values(1000000009, 4, 2, 'Fault - Small');
insert into PlaceType values(1000000010, 5, 0, 'Structure - Generic');
insert into PlaceType values(1000000011, 5, 1, 'Bridge');
insert into PlaceType values(1000000006, 5, 2, 'Dam');
insert into PlaceType values(1000000018, 5, 3, 'Tunnel');
insert into PlaceType values(1000000012, 6, 0, 'Noise Site - Generic');
insert into PlaceType values(1000000013, 6, 1, 'Airport');
insert into PlaceType values(1000000014, 6, 2, 'Military Base');
insert into PlaceType values(1000000015, 6, 3, 'Nuke Testing Site');
insert into PlaceType values(1000000016, 6, 4, 'Quary');
insert into PlaceType values(1000000019, 6, 5, 'Mine');
insert into PlaceType values(1000000020, 6, 6, 'Oilfield');

commit;



/* from http://www.iris.washington.edu/FDSN/networks.txt */

insert into Installation values(1999999999, 0,  'UNKNOWN','UNKNOWN - WILDCARD','','','Contact','Note');
insert into Installation values(1000000001, 1,  'INST_FAIRBANKS', 'AEIC Geophysical Institute, University of Alaska, Fairbanks',
                                                                     'AK','AA','Roger Hansen (roger@giseis.alaska.edu)','Note');
insert into Installation values(1000000002, 2,  'INST_UW', 'University of Washington - Pacific Northwest Regional Seismic Network',
                                                                     'UW','','Steve Malone (steve@geophys.washington.edu)','Note');
insert into Installation values(1000000003, 3,  'INST_MENLO', 'USGS Northern California Regional Network - Menlo Park','NC','','David Oppenheimer (oppen@usgs.gov)','Note');
insert into Installation values(1000000004, 4,  'INST_CIT', 'Caltech Seismological Laboratory / USGS - Pasadena','CI','TS','Kate Hutton (kate@bombay.gps.caltech.edu)','Note');
insert into Installation values(1000000005, 5,  'INST_UTAH', 'University of Utah Regional Network','UU','WY','Walter J. Arabasz (arabasz@seis.utah.edu)','Note');
insert into Installation values(1000000006, 6,  'INST_MEMPHIS', 'CERI - University of Memphis','UM','NM ET','Mitch Withers (withers@ceri.memphis.edu)','Note');
insert into Installation values(1000000007, 7,  'INST_UNR', 'University of Nevada, Reno, Seismological Laboratory','NN','SN','Ken Smith (ken@seismo.unr.edu)','Note');
insert into Installation values(1000000008, 8,  'INST_UCB', 'University California, Berkeley, Seismological Laboratory','BK','BG BP','Lind Gee             lind@seismo.berkeley.edu','Note');
insert into Installation values(1000000009, 9,  'INST_PTWC', 'Pacific Tsunami Warning Center - NOAA, Ewa Beach','PT','','Barry Hirshorn (hirshorn@ptwc.noaa.gov)','Note');
insert into Installation values(1000000010, 10, 'INST_IDA', 'IDA - Scripps Institution of Oceanography - Univ. California, San Diego','ID','II','Jonathan Berger (jberger@ucsd.edu)','Note');

insert into Installation values(1000000011, 11, 'INST_NC', 'Central North Carolina Seismic Network - UNC Chapel Hill','UNC','SE','Chris Powell (cap@geosci.unc.edu)','Note'); 
insert into Installation values(1000000012, 12, 'INST_VT', 'Virgina Tech Regional Seismic Network','BLA','SE','Martin Chapman (chapman@vtso.geol.vt.edu)','Note'); 
insert into Installation values(1000000013, 13, 'INST_USNSN', 'US National Seismograph Network (USNSN)','US','IU','Harley Benz (benz@gldbenz3.cr.usgs.gov)','Note');
insert into Installation values(1000000014, 14, 'INST_RICKS', 'Rick`s College Network - Idaho','RC','','Edmund J. Williams (williamse@ricks.edu)','Note');
insert into Installation values(1000000015, 15, 'INST_HVO', 'Hawaiian Volcano Observatory(HVO) - USGS','HV','','Paul Okubo (pokubo@usgs.gov)','Note');
insert into Installation values(1000000016, 16, 'INST_ATWC', 'West Coast and Alaska Tsunami Warning Center','AT','','Thomas J. Sokolowski (wcatwc@wcatwc.gov)','Note');
insert into Installation values(1000000017, 17, 'INST_PGE', 'Pacific Gas and Electric','PG','','Marcia McLaren, MKM2@pge.com','Note');
insert into Installation values(1000000018, 18, 'INST_PNNL', 'Pacific Northwest National Laboratory - Battelle','HW','','Alan Rohay (alan.rohay@pnl.gov)','Note');
insert into Installation values(1000000019, 19, 'INST_PGC', 'Pacific Geoscience Centre - Geological Survey of Canada','CN','','Richard Baldwin(baldwin@pgc.nrcan.gc.ca)','Note');
insert into Installation values(1000000020, 20, 'INST_AVO', 'Alaskan Volcano Observatory','','','Tom Murray(tlmurray@usgs.gov)','Note');

insert into Installation values(1000000021, 21, 'INST_BUTTE', 'Montana Regional Seismic Network','MB','','Michael Stickney (mike@mbmgsun.mtech.edu)','Note');
insert into Installation values(1000000022, 22, 'INST_LAMONT', 'Lamont-Doherty Earth Observatory of Columbia University','LD','','Douglas H. Johnson (dhj@ldeo.columbia.edu)','Note');
insert into Installation values(1000000023, 23, 'INST_SC_CHA', 'South Carolina Seismic Network- University of South Carolina/Charleston Southern University','','','Dr. Pradeep Talwani (talwani@prithvi.seis.sc.edu)','Note');
insert into Installation values(1000000024, 24, 'INST_STLOUIS', 'St. Louis University','SLU','NM','Robert B. Herrmann (rbh@eas.slu.edu)','Note');
insert into Installation values(1000000025, 25, 'INST_NMT', 'New Mexico Tech Seismic Network','SC','ER','Rick Aster (aster@dutchman.nmt.edu)','Note');
insert into Installation values(1000000026, 26, 'INST_CVO', 'Cascades Volcano Observatory - USGS (CVO)','','','Elliot T. Endo(etendo@usgs.gov)','Note');
insert into Installation values(1000000028, 28, 'INST_PRSN', 'Puerto Rico Seismic Network','PRSN','MPR','Christa G. von Hillebrandt-Andrade (christa@rmsismo.upr.clu.edu)','Note');
insert into Installation values(1000000029, 29, 'INST_TEMP1', 'Temp','','','','Note');
insert into Installation values(1000000030, 30, 'INST_SOAPP', 'Southeastern Appalachian Cooperative Seismic Network','SE','','Martin Chapman (chapman@vtso.geol.vt.edu)','Note');

insert into Installation values(1000000031, 31, 'INST_UO', 'University of Oregon Regional Network','UO','','Douglas Toomey (drt@newberry.uoregon.edu)','Note');
insert into Installation values(1000000035, 35, 'INST_INEL', 'Idaho National Engineering and Environmental Laboratory','INL','','Suzette Payne (msj1@inel.gov)','Note');
insert into Installation values(1000000037, 37, 'INST_MIT', 'MIT  - New England Seismic Network','MIT','NE','Charles Doll, Jr. (doll@erl.mit.edu) ','Note');
insert into Installation values(1000000039, 39, 'INST_IRISDMC', 'IRIS DMC','IU','II','Time Ahern','Note');
insert into Installation values(1000000042, 42, 'INST_CDWR', 'California Department of Water Resources','CDWR','','Dave Kessler (kessler@top-flite.water.ca.gov)','Note');

/* don't know anything about these inst_ids */
/*
insert into Installation values(1000000038, 38, 'INST_GSC', 'FAIRBANKS','','','Contact','Note');
insert into Installation values(1000000027, 27, 'INST_SC_COL', 'FAIRBANKS','','','Contact','Note'); 
insert into Installation values(1000000032, 32, 'INST_USBR', 'FAIRBANKS','','','Contact','Note');
insert into Installation values(1000000033, 33, 'INST_UTIG', 'FAIRBANKS','','','Contact','Note');
insert into Installation values(1000000034, 34, 'INST_PSU', 'FAIRBANKS','','','Contact','Note');
insert into Installation values(1000000036, 36, 'INST_SISMALP', 'FAIRBANKS','','','Contact','Note');
insert into Installation values(1000000040, 40, 'INST_BOZEMAN', 'FAIRBANKS','','','Contact','Note');
insert into Installation values(1000000041, 41, 'INST_UTK', 'FAIRBANKS','','','Contact','Note');
insert into Installation values(1000000043, 43, 'INST_WO', 'FAIRBANKS','','','Contact','Note');
insert into Installation values(1000000044, 44, 'INST_NAZ', 'FAIRBANKS','','','Contact','Note');
insert into Installation values(1000000045, 45, 'INST_MVO', 'FAIRBANKS','','','Contact','Note');

****************************************************/