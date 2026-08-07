#include "module.h"
#include <ITravelTime.h>

struct IGlint;
struct ISolve;
class CMod : public CModule {
// attributes
public:
	IGlint		*pGlint;
	ITravelTime	*pTT;
	ISolve		*pSolve;
  int       iNumLocatorIterations;

// Methods
public:
	CMod();
	~CMod();
	bool Action(IMessage *msg);
	int Locate(char *ent, char *mask);
};