#include "io/ps_memcard.h"

#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>

#include "log/ps_log.h"

void LoadMCMANModules()
{
    int ret;
	/*ret = SifLoadModule("cdrom0:SIO2MAN", 0, NULL);
	if (ret < 0) {
		ERRORLOG("Failed to load module: SIO2MAN");
	} */

	ret = SifLoadModule("cdrom0:\\MCMAN.IRX", 0, NULL);
	if (ret < 0) {
		ERRORLOG("Failed to load module: MCMAN");
	}

	ret = SifLoadModule("cdrom0:\\MCSERV.IRX", 0, NULL);
	if (ret < 0) {
		ERRORLOG("Failed to load module: MCSERV");
	}
}