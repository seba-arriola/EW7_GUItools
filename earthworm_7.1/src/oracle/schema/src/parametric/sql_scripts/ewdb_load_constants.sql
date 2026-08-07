/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

/* db_population_scripts.sql */
/*
Insert the table identifiers into ewdb_tablelist
********************************************/
insert into ewdb_tablelist values(1 , 'CodaAmp');
insert into ewdb_tablelist values(2 , 'Pick');
insert into ewdb_tablelist values(3 , 'TCoda');
insert into ewdb_tablelist values(4 , 'Origin');
insert into ewdb_tablelist values(5 , 'Magnitude');
insert into ewdb_tablelist values(6 , 'MechFM');
insert into ewdb_tablelist values(7 , 'PeakAmp');
insert into ewdb_tablelist values(8 , 'CodaDur');
insert into ewdb_tablelist values(9 , 'Chan');
insert into ewdb_tablelist values(10, 'Source');
insert into ewdb_tablelist values(12, 'Ray');
insert into ewdb_tablelist values(13, 'Event');
insert into ewdb_tablelist values(14, 'Prefer');
insert into ewdb_tablelist values(15, 'Bind');
insert into ewdb_tablelist values(16, 'Comments');
insert into ewdb_tablelist values(17, 'OriginPick');
insert into ewdb_tablelist values(18, 'MechFMPick');
insert into ewdb_tablelist values(19, 'MagLink');
insert into ewdb_tablelist values(20, 'Origin_EW');
insert into ewdb_tablelist values(21, 'Link_EW');
insert into ewdb_tablelist values(22, 'Pick_EW');
insert into ewdb_tablelist values(23, 'Snippet_EW');
insert into ewdb_tablelist values(24, 'Magnitude_EW');
insert into ewdb_tablelist values(25, 'ExternalEvent');
insert into ewdb_tablelist values(26, 'Waveform');
insert into ewdb_tablelist values(27, 'WaveformDesc');
insert into ewdb_tablelist values(28, 'Site');
insert into ewdb_tablelist values(29, 'Comp');
insert into ewdb_tablelist values(30, 'SiteT');
insert into ewdb_tablelist values(31, 'CompT');
insert into ewdb_tablelist values(32, 'ChanT');
insert into ewdb_tablelist values(33, 'ChanCTF');
insert into ewdb_tablelist values(34, 'CookedTF');
insert into ewdb_tablelist values(35, 'PolesOrZeroes');
insert into ewdb_tablelist values(40, 'SMBox');
insert into ewdb_tablelist values(41, 'SMMessage');
insert into ewdb_tablelist values(42, 'SMMotion');
insert into ewdb_tablelist values(43, 'SMVendor');

/*
Now Insert the centrally assigned nodes
********************************************/
insert into ewdbnode(ewdbnodeid,ewdbnodename,idcomment,iismynodeid)
  values (1,'DK Test Node',0,1);

/*
Now Insert the Magnitude Types into MagType 
********************************************/
insert into magtype values(0,'??','Undefined',7);
insert into magtype values(1,'ML','Local_Peak-Peak',7);
insert into magtype values(2,'Mw','Moment',7);
insert into magtype values(3,'Mb','Body Wave',7);
insert into magtype values(4,'Ms','Surface Wave',7);
insert into magtype values(5,'Mwp','Scalar Moment',7);
insert into magtype values(6,'Md','Duration',8);
insert into magtype values(7,'ML','Local_Zero-Peak',7);
insert into magtype values(8,'mblg','mblg', 7);

commit;
