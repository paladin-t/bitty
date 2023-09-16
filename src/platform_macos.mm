/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#import "platform.h"
#import "text.h"
#import <SDL.h>
#import <AppKit/AppKit.h>
#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <libgen.h>
#import <mach-o/dyld.h>
#import <pthread.h>
#import <sys/stat.h>
#import <unistd.h>

/*
** {===========================================================================
** Utilities
*/

static NSString* platformTrimPath(NSString* originalPath) {
	NSString* prefixToRemove = @"file://";
	if ([originalPath hasPrefix: prefixToRemove])
		originalPath = [originalPath substringFromIndex: [prefixToRemove length]];
	originalPath = [originalPath stringByReplacingPercentEscapesUsingEncoding: NSUTF8StringEncoding];

	return originalPath;
}

/* ===========================================================================} */

/*
** {===========================================================================
** IO
*/

bool Platform::copyFile(const char* src, const char* dst) {
	NSFileManager* filemgr = [NSFileManager defaultManager];
	NSString* oldPath = [NSString stringWithCString: src encoding: NSUTF8StringEncoding];
	NSString* newPath= [NSString stringWithCString: dst encoding: NSUTF8StringEncoding];
	if ([filemgr fileExistsAtPath: newPath])
		[filemgr removeItemAtPath: newPath error: NULL];
	if ([filemgr copyItemAtPath: oldPath toPath: newPath error: NULL] == YES)
		return true;
	else
		return false;
}

bool Platform::copyDirectory(const char* src, const char* dst) {
	NSFileManager* filemgr = [NSFileManager defaultManager];
	NSString* oldPath = [NSString stringWithCString: src encoding: NSUTF8StringEncoding];
	NSString* newPath= [NSString stringWithCString: dst encoding: NSUTF8StringEncoding];
	if ([filemgr fileExistsAtPath: newPath])
		[filemgr removeItemAtPath: newPath error: NULL];
	if ([filemgr copyItemAtPath: oldPath toPath: newPath error: NULL] == YES)
		return true;
	else
		return false;
}

bool Platform::moveFile(const char* src, const char* dst) {
	NSFileManager* filemgr = [NSFileManager defaultManager];
	NSURL* oldPath = [NSURL fileURLWithPath: [NSString stringWithCString: src encoding: NSUTF8StringEncoding]];
	NSURL* newPath= [NSURL fileURLWithPath: [NSString stringWithCString: dst encoding: NSUTF8StringEncoding]];
	[filemgr moveItemAtURL: oldPath toURL: newPath error: nil];

	return true;
}

bool Platform::moveDirectory(const char* src, const char* dst) {
	NSFileManager* filemgr = [NSFileManager defaultManager];
	NSURL* oldPath = [NSURL fileURLWithPath: [NSString stringWithCString: src encoding: NSUTF8StringEncoding]];
	NSURL* newPath= [NSURL fileURLWithPath: [NSString stringWithCString: dst encoding: NSUTF8StringEncoding]];
	[filemgr moveItemAtURL: oldPath toURL: newPath error: nil];

	return true;
}

bool Platform::removeFile(const char* src, bool toTrash) {
	NSFileManager* filemgr = [NSFileManager defaultManager];
	if (toTrash) {
		NSURL* oldPath = [NSURL fileURLWithPath: [NSString stringWithCString: src encoding: NSUTF8StringEncoding]];
		if (@available(macOS 10.8, *)) {
			if ([filemgr trashItemAtURL: oldPath resultingItemURL: NULL error: NULL] == YES)
				return true;
			else
				return false;
		} else {
			/*dispatch_semaphore_t sema = dispatch_semaphore_create(0);
			__block bool ret = false;
			NSArray* files = [NSArray arrayWithObject: oldPath];
			[[NSWorkspace sharedWorkspace] recycleURLs: files completionHandler:
				^(NSDictionary* newURLs, NSError* error) {
					if (error != nil) {
						NSLog(@"%@", error);
						dispatch_semaphore_signal(sema);

						return;
					}
					for (NSString* file in newURLs) {
						NSLog(@"File %@ recycled to %@", file, [newURLs objectForKey: file]);
					}

					ret = true;
					dispatch_semaphore_signal(sema);
				}
			];
			dispatch_semaphore_wait(sema, DISPATCH_TIME_FOREVER);

			return ret;*/

			NSString* nsstr = [NSString stringWithCString: src encoding: NSUTF8StringEncoding];
			if ([filemgr removeItemAtPath: nsstr error: NULL] == YES)
				return true;
			else
				return false;
		}
	} else {
		NSString* oldPath = [NSString stringWithCString: src encoding: NSUTF8StringEncoding];
		if ([filemgr removeItemAtPath: oldPath error: NULL] == YES)
			return true;
		else
			return false;
	}
}

bool Platform::removeDirectory(const char* src, bool toTrash) {
	NSFileManager* filemgr = [NSFileManager defaultManager];
	if (toTrash) {
		NSURL* oldPath = [NSURL fileURLWithPath: [NSString stringWithCString: src encoding: NSUTF8StringEncoding]];
		if (@available(macOS 10.8, *)) {
			if ([filemgr trashItemAtURL: oldPath resultingItemURL: NULL error: NULL] == YES)
				return true;
			else
				return false;
		} else {
			/*dispatch_semaphore_t sema = dispatch_semaphore_create(0);
			__block bool ret = false;
			NSArray* files = [NSArray arrayWithObject: oldPath];
			[[NSWorkspace sharedWorkspace] recycleURLs: files completionHandler:
			 	^(NSDictionary* newURLs, NSError* error) {
					if (error != nil) {
						NSLog(@"%@", error);
						dispatch_semaphore_signal(sema);

						return;
					}
					for (NSString* file in newURLs) {
						NSLog(@"File %@ recycled to %@", file, [newURLs objectForKey: file]);
					}

					ret = true;
					dispatch_semaphore_signal(sema);
				}
			];
			dispatch_semaphore_wait(sema, DISPATCH_TIME_FOREVER);

			return ret;*/

			NSString* nsstr = [NSString stringWithCString: src encoding: NSUTF8StringEncoding];
			if ([filemgr removeItemAtPath: nsstr error: NULL] == YES)
				return true;
			else
				return false;
		}
	} else {
		NSString* oldPath = [NSString stringWithCString: src encoding: NSUTF8StringEncoding];
		if ([filemgr removeItemAtPath: oldPath error: NULL] == YES)
			return true;
		else
			return false;
	}
}

bool Platform::makeDirectory(const char* path) {
	return !mkdir(path, 0777);
}

void Platform::accreditDirectory(const char*) {
	// Do nothing.
}

bool Platform::equal(const char* lpath, const char* rpath) {
	const std::string lstr = lpath;
	const std::string rstr = rpath;

	return lstr == rstr;
}

bool Platform::isParentOf(const char* lpath, const char* rpath) {
	const std::string lstr = lpath;
	const std::string rstr = rpath;

	if (lstr == rstr)
		return true;
	if (lstr.length() < rstr.length()) {
		if (rstr.find(lstr, 0) == 0) {
			const std::string tail = rstr.substr(lstr.length());
			if (tail.front() == '/' || tail.front() == '\\')
				return true;
		}
	}

	return false;
}

std::string Platform::absoluteOf(const std::string &path) {
	NSURL* nspath = [NSURL fileURLWithPath: [NSString stringWithCString: path.c_str() encoding: NSUTF8StringEncoding]];
	std::string ret = [platformTrimPath([nspath absoluteString]) cStringUsingEncoding: NSUTF8StringEncoding];
	if (!path.empty() && !ret.empty()) {
		if ((path.back() == '\\' || path.back() == '/') && (ret.back() != '\\' && ret.back() != '/'))
			ret.push_back('/');
		else if ((path.back() != '\\' && path.back() != '/') && (ret.back() == '\\' || ret.back() == '/'))
			ret.pop_back();
	}

	return ret;
}

std::string Platform::executableFile(void) {
	uint32_t bufsize = BITTY_MAX_PATH;
	char buf[BITTY_MAX_PATH];
	int result = _NSGetExecutablePath(buf, &bufsize);
	if (result == 0) {
		NSURL* fileURL = [NSURL fileURLWithPath: [NSString stringWithCString: buf encoding: NSUTF8StringEncoding]];
		/*NSURL* folderURL = [fileURL URLByDeletingLastPathComponent];
		std::string ret = [platformTrimPath([folderURL absoluteString]) cStringUsingEncoding: NSUTF8StringEncoding];
		if (!ret.empty() && ret.back() != '/')
			ret.push_back('/');*/
		const std::string ret = [platformTrimPath([fileURL absoluteString]) cStringUsingEncoding: NSUTF8StringEncoding];

		return ret;
	}

	return "";
}

std::string Platform::documentDirectory(void) {
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString* documentsDirectory = [paths objectAtIndex: 0];
	std::string ret = [documentsDirectory cStringUsingEncoding: NSUTF8StringEncoding];
	if (!ret.empty() && ret.back() != '/')
		ret.push_back('/');

	return ret;
}

std::string Platform::savedGamesDirectory(void) {
	return writableDirectory();
}

std::string Platform::currentDirectory(void) {
	NSFileManager* filemgr = [NSFileManager defaultManager];
	NSString* currentpath = [filemgr currentDirectoryPath];
	std::string ret = [currentpath cStringUsingEncoding: NSUTF8StringEncoding];

	return ret;
}

void Platform::currentDirectory(const char* dir) {
	chdir(dir);
}

/* ===========================================================================} */

/*
** {===========================================================================
** Surfing and browsing
*/

void Platform::surf(const char* url) {
	NSString* nsstr = [NSString stringWithCString: url encoding: NSUTF8StringEncoding];
	NSURL* nsurl = [NSURL URLWithString: nsstr];
	[[NSWorkspace sharedWorkspace] openURL: nsurl];
}

void Platform::browse(const char* dir) {
	NSString* path = [NSString stringWithCString: dir encoding: NSUTF8StringEncoding];
	[[NSWorkspace sharedWorkspace] selectFile: path inFileViewerRootedAtPath: @""];
}

/* ===========================================================================} */

/*
** {===========================================================================
** OS
*/

const char* Platform::os(void) {
	return "MacOS";
}

void Platform::threadName(const char* threadName) {
	pthread_setname_np(threadName);
}

std::string Platform::execute(const char* cmd) {
	const int ret_ = system(cmd);
	const std::string ret = Text::toString(ret_);

	return ret;
}

bool Platform::checkProgram(const char* /* prog */) {
	return false;
}

void Platform::redirectIoToConsole(void) {
	BITTY_MISSING
}

/* ===========================================================================} */

/*
** {===========================================================================
** GUI
*/

void Platform::msgbox(const char* text, const char* caption) {
	NSString* nstxt = [NSString stringWithCString: text encoding: NSUTF8StringEncoding];
	NSString* nscpt= [NSString stringWithCString: caption encoding: NSUTF8StringEncoding];

	NSAlert* alert = [[NSAlert alloc] init];
	[alert setMessageText: nscpt];
	[alert setInformativeText: nstxt];
	if ([alert runModal] == NSAlertFirstButtonReturn) {
		// Do nothing.
	}
}

void Platform::openInput(void) {
	// Do nothing.
}

void Platform::closeInput(void) {
	// Do nothing.
}

void Platform::inputScreenPosition(int x, int y) {
	SDL_Rect rect{ x, y, 100, 20 };
	SDL_SetTextInputRect(&rect);
}

/* ===========================================================================} */
