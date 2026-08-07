#!/bin/csh -f

echo
stalist_hinv2ora aksta.hinv $1 $2 $3
stalist_hinv2ora butte_sta.hinv $1 $2 $3 
stalist_hinv2ora ceri_sta.hinv $1 $2 $3
stalist_hinv2ora ncal_sta.hinv $1 $2 $3
stalist_hinv2ora utah_sta.hinv $1 $2 $3
stalist_hinv2ora uw_sta.hinv  $1 $2 $3
stalist_hinv2ora hvo_sta.hinv $1 $2 $3
stalist_usnsn2ora usnsn.sta neic2scn.d $1 $2 $3
