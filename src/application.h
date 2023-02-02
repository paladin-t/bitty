/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "bitty.h"

/*
** {===========================================================================
** Application
*/

class Application* createApplication(class Workspace* workspace, int argc, const char* argv[]);
void destroyApplication(class Application* app);

bool updateApplication(class Application* app);

/* ===========================================================================} */

#endif /* __APPLICATION_H__ */
