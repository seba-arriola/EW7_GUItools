#ifndef _STATION_H_
#define _STATION_H_

#include "str.h"

#define MAXSTA 10000
#define MIN_STATION_QUAL_THRESHOLD .001

typedef struct _SCNL
{
	char szSta[10];
	char szComp[10];
	char szNet[10];
	char szLoc[10];
} SCNL;


typedef struct 
{
	SCNL		Name;
	double		dLat;
	double		dLon;
	double		dElev;
	double		dQual;
	CStr		sDesc;
} STATION;

class CStation  {
public:

// Attributes
	int		nSta;		// Number of stations loaded
	STATION	Sta[MAXSTA];

// Methods
	CStation();
	virtual ~CStation();
	bool Load();
	bool LoadHypoEllipse(const char *name);
	bool LoadHypoInverse(const char *name);
	const STATION * Get(const SCNL *name);
	bool Put(const STATION * pStation);


protected:
  void SortStationList();
};


#endif